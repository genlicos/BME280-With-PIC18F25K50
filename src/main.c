/******************************************************************************
 * Projeto: Sistema de Monitoramento Ambiental com BME280 - Display Rotativo
 * Autor: Elison Nogueira
 * Data de Criação: 16/11/2024
 * Versão: 1.0
 * Plataforma: PIC18F25K50 (Microchip)
 * Compilador: mikroC Pro for PIC v7.6.0
 *
 * Descrição:
 * Este projeto implementa um sistema de monitoramento ambiental que utiliza o
 * sensor BME280 para leitura de temperatura, umidade e pressão atmosférica. Os
 * dados são exibidos de forma rotativa em um display LCD 16x2 via I2C,
 * alternando entre as leituras a cada 2 segundos.
 *
 * Hardware Necessário:
 * - Microcontrolador PIC18F25K50 (16Mhz de clock)
 * - Sensor BME280 (I2C)
 * - Display LCD 16x2 (I2C)
 * - Fonte de alimentação 5V
 * - Fonte de alimentação 3V3 para o sensor
 * - Logic Level Converter de 5V -> 3V3 (Para o SDA e SCL do BME280)
 *
 * Funcionalidades:
 * - Leitura de temperatura, umidade e pressão via BME280
 * - Exibição rotativa dos dados em display LCD
 * - Atualização automática a cada 2 segundos
 *
 * Referências:
 * - Datasheet BME280
 * - Datasheet PIC18F25K50
 * - Documentação do MikroC Pro for PIC
 *
 * Configurações dos Registradores iniciais:
 * - CONFIG1L : $300000 : 0x0000
 * - CONFIG1H : $300001 : 0x0003
 * - CONFIG2L : $300002 : 0x005F
 * - CONFIG2H : $300003 : 0x003C
 * - CONFIG3H : $300005 : 0x00D3
 * - CONFIG4L : $300006 : 0x0081
 * - CONFIG5L : $300008 : 0x000F
 * - CONFIG5H : $300009 : 0x00C0
 * - CONFIG6L : $30000A : 0x000F
 * - CONFIG6H : $30000B : 0x00E0
 * - CONFIG7L : $30000C : 0x000F
 * - CONFIG7H : $30000D : 0x0040
  *****************************************************************************/

// Incluindo bibliotecas
#include "bibis/bme280.h"
#include "bibis/lcd_i2c.h"

// Variáveis globais
signed long temperatura;                                                        // Armazena temperatura em centésimos de grau
unsigned long pressao, umidade;                                                 // Armazena pressão em Pa e umidade em 1024 passos
char texto[16];                                                                 // Buffer para strings no LCD
unsigned char estado_display = 0;                                               // Controla qual leitura será exibida

// Enumeração para controle do estado de exibição
enum ESTADOS_DISPLAY {
    MOSTRA_TEMPERATURA = 0,
    MOSTRA_UMIDADE = 1,
    MOSTRA_PRESSAO = 2
};

void inicializar_sistema() {
    char txt[17];

    // Inicializa comunicação I2C
    I2C1_Init(100000);
    delay_ms(100);

    // Inicializa LCD
    I2C_LCD_Init();

    // Mensagem inicial
    // Testa comunicação I2C
    ADD_BME280 = BME280_TestConnection();
    if(ADD_BME280 == 0) {
        I2C_Lcd_Out(1, 1, "Erro I2C!");
        I2C_Lcd_Out(2, 1, "Sensor n/ found");
        while(1);
    } else {
        sprintf(txt, "Add BME280: 0x%02X", ADD_BME280);
        I2C_Lcd_Out(1, 1, txt);
        sprintf(txt, "Iniciando...");
        I2C_Lcd_Out(2, 1, txt);
        Delay_ms(2000);
    }

    // Inicializa BME280
    if(!BME280_Begin(MODE_NORMAL, SAMPLING_X1, SAMPLING_X1, SAMPLING_X1, FILTER_OFF, STANDBY_0_5)) {
        I2C_LCD_Out(1, 1, "Erro BME280!");
        while(1);  // Trava execução em caso de erro
    }
}

void ler_sensor() {
    // Realiza todas as leituras do sensor
    ReadTemperature(&temperatura);
    ReadHumidity(&umidade);
    ReadPressure(&pressao);
}

void exibir_temperatura() {
    I2C_LCD_Cmd(_LCD_CLEAR);
    I2C_LCD_Out(1, 1, "Temperatura:");

    // Formata e exibe a temperatura
    if(temperatura < 0) {
        sprintf(texto, "-%d.%02d C", abs(temperatura/100), abs(temperatura%100));
    } else {
        sprintf(texto, "%d.%02d C", temperatura/100, temperatura%100);
    }
    I2C_LCD_Out(2, 1, texto);
}

void exibir_umidade() {
    I2C_LCD_Cmd(_LCD_CLEAR);
    I2C_LCD_Out(1, 1, "Umidade:");

    // Formata e exibe a umidade
    sprintf(texto, "%d.%d %%", umidade/1024, ((umidade*100)/1024)%100);
    I2C_LCD_Out(2, 1, texto);
}

void exibir_pressao() {
    I2C_LCD_Cmd(_LCD_CLEAR);
    I2C_LCD_Out(1, 1, "Pressao:");

    // Formata e exibe a pressão
    sprintf(texto, "%d.%d hPa", pressao/100, pressao%100);
    I2C_LCD_Out(2, 1, texto);
}

void atualizar_display() {
    // Seleciona qual informação exibir baseado no estado atual
    switch(estado_display) {
        case MOSTRA_TEMPERATURA:
            exibir_temperatura();
            break;

        case MOSTRA_UMIDADE:
            exibir_umidade();
            break;

        case MOSTRA_PRESSAO:
            exibir_pressao();
            break;
    }

    // Avança para o próximo estado
    estado_display = (estado_display + 1) % 3;
}

void main() {
    // Inicializa sistema
    inicializar_sistema();

    // Loop principal
    while(1) {
        // Faz a leitura do sensor
        ler_sensor();

        // Atualiza o display com a leitura atual
        atualizar_display();

        // Aguarda 2 segundos antes da próxima atualização
        delay_ms(2000);
    }
}