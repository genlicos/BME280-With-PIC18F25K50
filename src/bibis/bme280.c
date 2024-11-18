/******************************************************************************
 * Biblioteca: Driver BME280 (bme280.c)
 * Autor: Elison Nogueira
 * Data: 16/11/2024
 * Vers�o: 1.0
 * Plataforma: PIC18F25K50 (Microchip)
 * Compilador: mikroC Pro for PIC v7.6.0
 *
 * Descri��o:
 * Driver para sensor BME280 da Bosch que realiza leituras de temperatura,
 * umidade e press�o atmosf�rica via interface I2C. O driver suporta:
 * - Modo normal, for�ado e sleep
 * - Configura��o de oversampling para temp/press�o/umidade
 * - Filtro digital configur�vel
 * - Tempo de standby ajust�vel
 *
 * Depend�ncias:
 * - Biblioteca I2C do mikroC PRO for PIC
 *
 * Limita��es:
 * - Apenas comunica��o I2C (n�o suporta SPI)
 * - Precis�o de temperatura: 0.01�C
 * - Precis�o de umidade: 0.008%
 * - Precis�o de press�o: 0.18Pa
 ******************************************************************************/

#include "bme280.h"

// Vari�veis para armazenamento das leituras e calibra��o
long adc_T, adc_P, adc_H, t_fine;                                               // Dados brutos do ADC
calib_bme280 BME280_calib;                                                      // Dados de calibra��o
unsigned char ADD_BME280;                                                       // Endere�o I2C do BME280

// Escrita de um byte no registrador do BME280 via I2C
void I2C_Write8(unsigned short reg_addr, unsigned short _data) {
    I2C_Start();                                                                // In�cio comunica��o I2C
    I2C_Write(ADD_BME280);                                                      // Endere�o do BME280
    I2C_Write(reg_addr);                                                        // Registrador a ser acessado
    I2C_Write(_data);                                                           // Dado a ser gravado
    I2C_Stop();                                                                 // Fim comunica��o I2C
}

// Leitura de um byte do registrador do BME280 via I2C
unsigned short I2C_Read8(unsigned short reg_addr) {
    unsigned short ret;                                                         // Armazena byte lido

    I2C_Start();                                                                // In�cio comunica��o I2C
    I2C_Write(ADD_BME280);                                                      // Endere�o do BME280
    I2C_Write(reg_addr);                                                        // Registrador a ser lido
    I2C_Restart();                                                              // Reinicia comunica��o
    I2C_Write(ADD_BME280 | 1);                                                  // Modo leitura
    ret = I2C_Read(0);                                                          // L� byte
    I2C_Stop();                                                                 // Fim comunica��o I2C

    return ret;                                                                 // Retorna byte lido
}

// Leitura de dois bytes (palavra) do registrador do BME280 via I2C
unsigned int I2C_Read16(unsigned short reg_addr) {
    union {
        unsigned short b[2];                                                    // Array de bytes
        unsigned int w;                                                         // Palavra de 16 bits
    } ret;

    I2C_Start();                                                                // In�cio comunica��o I2C
    I2C_Write(ADD_BME280);                                                      // Endere�o do BME280
    I2C_Write(reg_addr);                                                        // Registrador a ser lido
    I2C_Restart();                                                              // Reinicia comunica��o
    I2C_Write(ADD_BME280 | 1);                                                  // Modo leitura
    ret.b[0] = I2C_Read(1);                                                     // L� byte menos significativo
    ret.b[1] = I2C_Read(0);                                                     // L� byte mais significativo
    I2C_Stop();                                                                 // Fim comunica��o I2C

    return(ret.w);                                                              // Retorna palavra lida
}

// Testa conex�o com o sensor buscando endere�o I2C v�lido
unsigned char BME280_TestConnection(void) {
    unsigned char i;
    unsigned char found = 0;

    for(i = 0; i < 2; i++) {                                                    // Testa os dois endere�os poss�veis
        I2C1_Start();                                                           // Inicia comunica��o
        if(i == 0) {
            if(I2C1_Wr(BME280_ADDR_LOW) == 0) found = BME280_ADDR_LOW;          // Testa endere�o 0x76
        } else {
            if(I2C1_Wr(BME280_ADDR_HIGH) == 0) found = BME280_ADDR_HIGH;        // Testa endere�o 0x77
        }
        I2C1_Stop();                                                            // Finaliza comunica��o

        if(found) break;                                                        // Endere�o encontrado
    }

    return found;                                                               // Retorna endere�o encontrado
}

// Configura os par�metros de opera��o do BME280
void BME280_Configure(bme280_mode mode, bme280_sampling T_sampling,
                     bme280_sampling H_sampling, bme280_sampling P_sampling,
                     bme280_filter filter, standby_time standby) {

    unsigned short _ctrl_hum, _ctrl_meas, _config;                              // Registradores de controle

    _ctrl_hum = H_sampling;                                                     // Configura amostragem umidade
    _config = ((standby << 5) | (filter << 2)) & 0xFC;                          // Configura standby e filtro
    _ctrl_meas = (T_sampling << 5) | (P_sampling << 2) | mode;                  // Configura amostragem T/P e modo

    I2C_Write8(BME280_REG_CTRLHUM, _ctrl_hum);                                  // Grava config umidade
    I2C_Write8(BME280_REG_CONFIG, _config);                                     // Grava config geral
    I2C_Write8(BME280_REG_CONTROL, _ctrl_meas);                                 // Grava config medi��o
}

// Inicializa o sensor BME280 com os par�metros fornecidos
unsigned short BME280_begin(bme280_mode mode, bme280_sampling T_sampling,
                          bme280_sampling H_sampling, bme280_sampling P_sampling,
                          bme280_filter filter, standby_time standby) {

    if(I2C_Read8(BME280_REG_CHIPID) != BME280_CHIP_ID)                          // Verifica ID do sensor
        return 0;                                                               // Retorna erro se ID inv�lido

    I2C_Write8(BME280_REG_SOFTRESET, 0xB6);                                     // Executa reset do sensor
    delay_ms(100);                                                              // Aguarda reset completar

    while((I2C_Read8(BME280_REG_STATUS) & 0x01) == 0x01)                        // Aguarda dados de calibra��o
        delay_ms(100);

    // L� coeficientes de calibra��o para temperatura
    BME280_calib.dig_T1 = I2C_Read16(BME280_REG_DIG_T1);                        // Coeficiente T1
    BME280_calib.dig_T2 = I2C_Read16(BME280_REG_DIG_T2);                        // Coeficiente T2
    BME280_calib.dig_T3 = I2C_Read16(BME280_REG_DIG_T3);                        // Coeficiente T3

    // L� coeficientes de calibra��o para press�o
    BME280_calib.dig_P1 = I2C_Read16(BME280_REG_DIG_P1);                        // Coeficiente P1
    BME280_calib.dig_P2 = I2C_Read16(BME280_REG_DIG_P2);                        // Coeficiente P2
    BME280_calib.dig_P3 = I2C_Read16(BME280_REG_DIG_P3);                        // Coeficiente P3
    BME280_calib.dig_P4 = I2C_Read16(BME280_REG_DIG_P4);                        // Coeficiente P4
    BME280_calib.dig_P5 = I2C_Read16(BME280_REG_DIG_P5);                        // Coeficiente P5
    BME280_calib.dig_P6 = I2C_Read16(BME280_REG_DIG_P6);                        // Coeficiente P6
    BME280_calib.dig_P7 = I2C_Read16(BME280_REG_DIG_P7);                        // Coeficiente P7
    BME280_calib.dig_P8 = I2C_Read16(BME280_REG_DIG_P8);                        // Coeficiente P8
    BME280_calib.dig_P9 = I2C_Read16(BME280_REG_DIG_P9);                        // Coeficiente P9

    // L� coeficientes de calibra��o para umidade
    BME280_calib.dig_H1 = I2C_Read8(BME280_REG_DIG_H1);                         // Coeficiente H1
    BME280_calib.dig_H2 = I2C_Read16(BME280_REG_DIG_H2);                        // Coeficiente H2
    BME280_calib.dig_H3 = I2C_Read8(BME280_REG_DIG_H3);                         // Coeficiente H3

    // L� coeficientes H4 e H5 que possuem bits compartilhados
    BME280_calib.dig_H4 = ((unsigned int)I2C_Read8(BME280_REG_DIG_H4) << 4) |
                          (I2C_Read8(BME280_REG_DIG_H4 + 1) & 0x0F);            // Coeficiente H4
    if (BME280_calib.dig_H4 & 0x0800)                                           // Ajusta sinal se negativo
        BME280_calib.dig_H4 |= 0xF000;

    BME280_calib.dig_H5 = ((unsigned int)I2C_Read8(BME280_REG_DIG_H5 + 1) << 4) |
                          (I2C_Read8(BME280_REG_DIG_H5) >> 4);                  // Coeficiente H5
    if (BME280_calib.dig_H5 & 0x0800)                                           // Ajusta sinal se negativo
        BME280_calib.dig_H5 |= 0xF000;

    BME280_calib.dig_H6 = I2C_Read8(BME280_REG_DIG_H6);                         // Coeficiente H6

    BME280_Configure(mode, T_sampling, H_sampling, P_sampling, filter, standby);// Configura par�metros

    return 1;                                                                   // Retorna sucesso
}

// For�a uma nova medi��o quando em modo for�ado
unsigned short BME280_ForcedMeasurement() {
    unsigned short ctrl_meas_reg = I2C_Read8(BME280_REG_CONTROL);               // L� registrador controle

    if ((ctrl_meas_reg & 0x03) != 0x00)                                         // Verifica se est� em sleep
        return 0;                                                               // Retorna erro se n�o

    I2C_Write8(BME280_REG_CONTROL, ctrl_meas_reg | 1);                          // For�a uma medi��o
    while (I2C_Read8(BME280_REG_STATUS) & 0x08)                                 // Aguarda medi��o completar
        delay_ms(1);

    return 1;                                                                   // Retorna sucesso
}

// Atualiza leituras brutas de press�o, temperatura e umidade
void BME280_Update() {
    union {
        unsigned short b[4];                                                    // Array de bytes
        unsigned long dw;                                                       // Palavra 32 bits
    } ret;
    ret.b[3] = 0x00;                                                            // Limpa byte mais significativo

    I2C_Start();                                                                // Inicia comunica��o
    I2C_Write(ADD_BME280);                                                      // Endere�o do sensor
    I2C_Write(BME280_REG_PRESS_MSB);                                            // Registrador inicial
    I2C_Restart();                                                              // Reinicia para leitura
    I2C_Write(ADD_BME280 | 1);                                                  // Modo leitura

    // L� press�o (20 bits)
    ret.b[2] = I2C_Read(1);                                                     // Byte mais significativo
    ret.b[1] = I2C_Read(1);                                                     // Byte do meio
    ret.b[0] = I2C_Read(1);                                                     // Byte menos significativo
    adc_P = (ret.dw >> 4) & 0xFFFFF;                                            // Extrai 20 bits press�o

    // L� temperatura (20 bits)
    ret.b[2] = I2C_Read(1);                                                     // Byte mais significativo
    ret.b[1] = I2C_Read(1);                                                     // Byte do meio
    ret.b[0] = I2C_Read(1);                                                     // Byte menos significativo
    adc_T = (ret.dw >> 4) & 0xFFFFF;                                            // Extrai 20 bits temperatura

    // L� umidade (16 bits)
    ret.b[2] = 0x00;                                                            // Limpa byte n�o usado
    ret.b[1] = I2C_Read(1);                                                     // Byte mais significativo
    ret.b[0] = I2C_Read(0);                                                     // Byte menos significativo
    I2C_Stop();                                                                 // Finaliza comunica��o

    adc_H = ret.dw & 0xFFFF;                                                    // Extrai 16 bits umidade
}

// L� temperatura em cent�simos de grau Celsius
unsigned short ReadTemperature(long *temp) {
    long var1, var2;                                                            // Vari�veis auxiliares c�lculo

    BME280_Update();                                                            // Atualiza leituras

    // Calcula temperatura usando coeficientes de calibra��o
    var1 = ((((adc_T / 8) - ((long)BME280_calib.dig_T1 * 2))) *
           ((long)BME280_calib.dig_T2)) / 2048;

    var2 = (((((adc_T / 16) - ((long)BME280_calib.dig_T1)) *
           ((adc_T / 16) - ((long)BME280_calib.dig_T1))) / 4096) *
           ((long)BME280_calib.dig_T3)) / 16384;

    t_fine = var1 + var2;                                                       // Temperatura calibrada
    *temp = (t_fine * 5 + 128) / 256;                                           // Converte para cent�simos �C

    return 1;                                                                   // Retorna sucesso
}

// L� umidade relativa em passos de 1024 (47445 = 46.333%)
unsigned short ReadHumidity(unsigned long *humi) {
    long v_x1_u32r;                                                             // Vari�vel auxiliar c�lculo
    unsigned long H;                                                            // Umidade calculada

    // C�lculo complexo usando coeficientes de calibra��o
    v_x1_u32r = (t_fine - ((long)76800));
    v_x1_u32r = (((((adc_H * 16384) - (((long)BME280_calib.dig_H4) * 1048576) -
                (((long)BME280_calib.dig_H5) * v_x1_u32r)) + ((long)16384)) / 32768) *
                (((((((v_x1_u32r * ((long)BME280_calib.dig_H6)) / 1024) *
                (((v_x1_u32r * ((long)BME280_calib.dig_H3)) / 2048) + ((long)32768))) / 1024) +
                ((long)2097152)) * ((long)BME280_calib.dig_H2) + 8192) / 16384));

    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r / 32768) * (v_x1_u32r / 32768)) / 128) *
                ((long)BME280_calib.dig_H1)) / 16));

    // Limita resultado entre 0 e 419430400
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

    H = (unsigned long)(v_x1_u32r / 4096);                                      // Converte para formato final
    *humi = H;                                                                  // Atualiza ponteiro

    return 1;                                                                   // Retorna sucesso
}

// L� press�o em Pascal (96386 = 963.86 hPa)
unsigned short ReadPressure(unsigned long *pres) {
    long var1, var2;                                                            // Vari�veis auxiliares
    unsigned long p;                                                            // Press�o calculada

    // Calcula press�o usando coeficientes de calibra��o
    var1 = (((long)t_fine) / 2) - (long)64000;
    var2 = (((var1/4) * (var1/4)) / 2048) * ((long)BME280_calib.dig_P6);
    var2 = var2 + ((var1 * ((long)BME280_calib.dig_P5)) * 2);
    var2 = (var2/4) + (((long)BME280_calib.dig_P4) * 65536);
    var1 = ((((long)BME280_calib.dig_P3 * (((var1/4) * (var1/4)) / 8192)) / 8) +
           ((((long)BME280_calib.dig_P2) * var1)/2)) / 262144;
    var1 =((((32768 + var1)) * ((long)BME280_calib.dig_P1)) / 32768);

    if (var1 == 0)                                                              // Evita divis�o por zero
        return 0;

    p = (((unsigned long)(((long)1048576) - adc_P) - (var2 / 4096))) * 3125;

    if (p < 0x80000000)
        p = (p * 2) / ((unsigned long)var1);
    else
        p = (p / (unsigned long)var1) * 2;

    var1 = (((long)BME280_calib.dig_P9) * ((long)(((p/8) * (p/8)) / 8192))) / 4096;
    var2 = (((long)(p/4)) * ((long)BME280_calib.dig_P8)) / 8192;

    p = (unsigned long)((long)p + ((var1 + var2 + (long)BME280_calib.dig_P7) / 16));

    *pres = p;                                                                  // Atualiza ponteiro

    return 1;                                                                   // Retorna sucesso
}