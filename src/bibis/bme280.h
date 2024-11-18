/******************************************************************************
 * Biblioteca: Driver BME280 (bme280.h)
 * Autor: Elison Nogueira
 * Data: 16/11/2024
 * Versão: 1.0
 * Plataforma: PIC18F25K50 (Microchip)
 * Compilador: mikroC Pro for PIC v7.6.0
 *
 * Descrição:
 * Driver para sensor BME280 da Bosch que realiza leituras de temperatura,
 * umidade e pressão atmosférica via interface I2C. O driver suporta:
 * - Modo normal, forçado e sleep
 * - Configuração de oversampling para temp/pressão/umidade
 * - Filtro digital configurável
 * - Tempo de standby ajustável
 *
 * Dependências:
 * - Biblioteca I2C do mikroC PRO for PIC
 *
 * Limitações:
 * - Apenas comunicação I2C (não suporta SPI)
 * - Precisão de temperatura: 0.01°C
 * - Precisão de umidade: 0.008%
 * - Precisão de pressão: 0.18Pa
 *****************************************************************************/

// Endereços I2C possíveis do BME280
#define BME280_ADDR_LOW  0xEC                                                   // SDO/CSB conectado ao GND
#define BME280_ADDR_HIGH 0xEE                                                   // SDO/CSB conectado ao VDD

#define BME280_CHIP_ID        0x60                                              // ID do chip BME280

// Registradores de calibração de temperatura
#define BME280_REG_DIG_T1     0x88                                              // Registrador T1
#define BME280_REG_DIG_T2     0x8A                                              // Registrador T2
#define BME280_REG_DIG_T3     0x8C                                              // Registrador T3

// Registradores de calibração de pressão
#define BME280_REG_DIG_P1     0x8E                                              // Registrador P1
#define BME280_REG_DIG_P2     0x90                                              // Registrador P2
#define BME280_REG_DIG_P3     0x92                                              // Registrador P3
#define BME280_REG_DIG_P4     0x94                                              // Registrador P4
#define BME280_REG_DIG_P5     0x96                                              // Registrador P5
#define BME280_REG_DIG_P6     0x98                                              // Registrador P6
#define BME280_REG_DIG_P7     0x9A                                              // Registrador P7
#define BME280_REG_DIG_P8     0x9C                                              // Registrador P8
#define BME280_REG_DIG_P9     0x9E                                              // Registrador P9

// Registradores de calibração de umidade
#define BME280_REG_DIG_H1     0xA1                                              // Registrador H1
#define BME280_REG_DIG_H2     0xE1                                              // Registrador H2
#define BME280_REG_DIG_H3     0xE3                                              // Registrador H3
#define BME280_REG_DIG_H4     0xE4                                              // Registrador H4
#define BME280_REG_DIG_H5     0xE5                                              // Registrador H5
#define BME280_REG_DIG_H6     0xE7                                              // Registrador H6

// Registradores de controle
#define BME280_REG_CHIPID     0xD0                                              // Registro do ID do chip
#define BME280_REG_SOFTRESET  0xE0                                              // Registro de reset
#define BME280_REG_CTRLHUM    0xF2                                              // Controle de umidade
#define BME280_REG_STATUS     0xF3                                              // Status
#define BME280_REG_CONTROL    0xF4                                              // Controle principal
#define BME280_REG_CONFIG     0xF5                                              // Configuração
#define BME280_REG_PRESS_MSB  0xF7                                              // MSB da pressão


// Variáveis externas para armazenamento das leituras ADC
extern long adc_T, adc_P, adc_H, t_fine;
// Variável externas para armazenamento do endereçamento do BME280
extern unsigned char ADD_BME280;

// Enumeração para modos de operação do BME280
typedef enum {
    MODE_SLEEP  = 0x00,                                                         // Modo sleep (baixo consumo)
    MODE_FORCED = 0x01,                                                         // Modo forçado (uma leitura)
    MODE_NORMAL = 0x03                                                          // Modo normal (leituras contínuas)
} bme280_mode;

// Enumeração para configuração de oversampling
typedef enum {
    SAMPLING_SKIPPED = 0x00,                                                    // Desabilita leitura
    SAMPLING_X1      = 0x01,                                                    // Oversampling x1
    SAMPLING_X2      = 0x02,                                                    // Oversampling x2
    SAMPLING_X4      = 0x03,                                                    // Oversampling x4
    SAMPLING_X8      = 0x04,                                                    // Oversampling x8
    SAMPLING_X16     = 0x05                                                     // Oversampling x16
} bme280_sampling;

// Enumeração para configuração do filtro digital
typedef enum {
    FILTER_OFF = 0x00,                                                          // Filtro desligado
    FILTER_2   = 0x01,                                                          // Coeficiente 2
    FILTER_4   = 0x02,                                                          // Coeficiente 4
    FILTER_8   = 0x03,                                                          // Coeficiente 8
    FILTER_16  = 0x04                                                           // Coeficiente 16
} bme280_filter;

// Enumeração para tempo de standby
typedef enum {
    STANDBY_0_5   =  0x00,                                                      // 0.5ms
    STANDBY_62_5  =  0x01,                                                      // 62.5ms
    STANDBY_125   =  0x02,                                                      // 125ms
    STANDBY_250   =  0x03,                                                      // 250ms
    STANDBY_500   =  0x04,                                                      // 500ms
    STANDBY_1000  =  0x05,                                                      // 1000ms
    STANDBY_10    =  0x06,                                                      // 10ms
    STANDBY_20    =  0x07                                                       // 20ms
} standby_time;

// Estrutura para armazenar dados de calibração
typedef struct {
    unsigned int dig_T1;                                                        // Calibração T1
    int  dig_T2;                                                                // Calibração T2
    int  dig_T3;                                                                // Calibração T3
    unsigned int dig_P1;                                                        // Calibração P1
    int  dig_P2;                                                                // Calibração P2
    int  dig_P3;                                                                // Calibração P3
    int  dig_P4;                                                                // Calibração P4
    int  dig_P5;                                                                // Calibração P5
    int  dig_P6;                                                                // Calibração P6
    int  dig_P7;                                                                // Calibração P7
    int  dig_P8;                                                                // Calibração P8
    int  dig_P9;                                                                // Calibração P9
    unsigned short  dig_H1;                                                     // Calibração H1
    int  dig_H2;                                                                // Calibração H2
    unsigned short  dig_H3;                                                     // Calibração H3
    int  dig_H4;                                                                // Calibração H4
    int  dig_H5;                                                                // Calibração H5
    short   dig_H6;                                                             // Calibração H6
} calib_bme280;

// Protótipos das funções
void I2C_Write8(unsigned short reg_addr, unsigned short _data);                 // Escreve 1 byte via I2C
unsigned short I2C_Read8(unsigned short reg_addr);                              // Lê 1 byte via I2C
unsigned int I2C_Read16(unsigned short reg_addr);                               // Lê 2 bytes via I2C
unsigned char BME280_TestConnection(void);                                      // Testa a conexão com sensor na inicialização
void BME280_Configure(bme280_mode mode, bme280_sampling T_sampling,             // Configura parâmetros do sensor
                     bme280_sampling H_sampling, bme280_sampling P_sampling,
                     bme280_filter filter, standby_time standby);
unsigned short BME280_begin(bme280_mode mode,                                   // Inicializa o sensor
                          bme280_sampling T_sampling,
                          bme280_sampling H_sampling,
                          bme280_sampling P_sampling,
                          bme280_filter filter,
                          standby_time standby);
unsigned short BME280_ForcedMeasurement();                                      // Realiza medição forçada
void BME280_Update();                                                           // Atualiza leituras do ADC
unsigned short ReadTemperature(long *temp);                                     // Lê temperatura
unsigned short ReadHumidity(unsigned long *humi);                               // Lê umidade
unsigned short ReadPressure(unsigned long *pres);                               // Lê pressão