#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <math.h> 

// Inclui o driver nativo de CAN (TWAI) do ESP32
#include "driver/twai.h"

// Pinos do Ecrã GC9A01
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  4

// Pinos escolhidos para a rede CAN no ESP32
#define CAN_TX_PIN GPIO_NUM_21
#define CAN_RX_PIN GPIO_NUM_22

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

int rpm_atual = 0;
int pedal_atual = 0;
int rpm_anterior = -1;
int pedal_anterior = -1;

void desenharFundoEstatico();
void atualizarPainel(int rpm, int pedal);
void desenharPonteiro(int valor, uint16_t cor);
void lerRedeCAN();

void setup() {
  Serial.begin(115200);
  
  // Inicializa o Ecrã
  tft.begin();
  tft.fillScreen(GC9A01A_BLACK);
  desenharFundoEstatico();

  // ==========================================
  // INICIALIZAÇÃO DO CAN NATIVO DO ESP32 (TWAI)
  // ==========================================
  // Configuração dos Pinos
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  
  // Configuração da Velocidade (Ajusta para a velocidade do teu carro, ex: 500kbps)
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  
  // Filtros (Aceita todas as mensagens)
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // Instala e arranca o driver
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    Serial.println("Driver CAN instalado");
  }
  if (twai_start() == ESP_OK) {
    Serial.println("Driver CAN iniciado");
  }
}

void loop() {
  // Ouve a rede continuamente sem bloquear o painel
  lerRedeCAN();
}

// ==========================================
// LEITURA DO BARRAMENTO CAN (USANDO ESP32 TWAI)
// ==========================================
void lerRedeCAN() {
  twai_message_t message;

  // Verifica se há alguma mensagem nova (o '0' significa que não bloqueia à espera)
  if (twai_receive(&message, 0) == ESP_OK) {
    
    // Processa os IDs exatamente como antes
    
    // EXAMPLO 1: Capturando RPM (Substitui o ID 0x1A2 pelo do teu carro)
    if (message.identifier == 0x1A2) { 
      // Exemplo: Junta 2 bytes (Byte 2 e 3 da mensagem)
      rpm_atual = (message.data[2] << 8) | message.data[3]; 
    }
    
    // EXEMPLO 2: Capturando Pedal (Substitui o ID 0x2B4)
    if (message.identifier == 0x2B4) { 
      pedal_atual = map(message.data[4], 0, 255, 0, 100); 
    }

    // Só redesenha se os valores mudaram (mantém o ponteiro suave)
    if (rpm_atual != rpm_anterior || pedal_atual != pedal_anterior) {
      atualizarPainel(rpm_atual, pedal_atual);
    }
  }
}

// ==========================================
// FUNÇÕES DE DESENHO (Visual Branco/Vermelho Desportivo)
// ==========================================

void desenharFundoEstatico() {
  for(int i = 0; i < 360; i += 10) {
    float rad = i * PI / 180.0;
    tft.drawPixel(120 + 115 * cos(rad), 120 + 115 * sin(rad), GC9A01A_DARKGREY);
  }

  for (int i = 0; i <= 8000; i += 500) {
    float angulo_graus = map(i, 0, 8000, 135, 405);
    float angulo_rad = angulo_graus * PI / 180.0; 
    
    int x_ext = 120 + 110 * cos(angulo_rad); 
    int y_ext = 120 + 110 * sin(angulo_rad);
    int x_int = 120 + 95 * cos(angulo_rad); 
    int y_int = 120 + 95 * sin(angulo_rad);
    
    uint16_t cor_marca = (i >= 6000) ? GC9A01A_RED : GC9A01A_WHITE;
    
    if (i % 1000 == 0) {
      tft.drawLine(x_int, y_int, x_ext, y_ext, cor_marca);
      tft.drawLine(x_int+1, y_int, x_ext+1, y_ext, cor_marca); 
    } else {
      tft.drawPixel(x_ext, y_ext, cor_marca);
    }
  }
  
  tft.setTextSize(1);
  tft.setTextColor(GC9A01A_WHITE);
  
  tft.setCursor(111, 65); 
  tft.print("RPM");
  
  tft.setCursor(99, 130); 
  tft.print("PEDAL %");
}

void atualizarPainel(int rpm, int pedal) {
  if (rpm != rpm_anterior) {
    if (rpm_anterior >= 0) {
      desenharPonteiro(rpm_anterior, GC9A01A_BLACK);
    }
    
    uint16_t corPonteiro = (rpm >= 6000) ? GC9A01A_RED : GC9A01A_WHITE;
    desenharPonteiro(rpm, corPonteiro);

    tft.setTextColor(GC9A01A_WHITE, GC9A01A_BLACK);
    tft.setTextSize(3);
    
    char rpmStr[10];
    sprintf(rpmStr, "%04d", rpm); 
    tft.setCursor(84, 80); 
    tft.print(rpmStr);
    
    rpm_anterior = rpm;
  }
  
  if (pedal != pedal_anterior) {
    tft.setTextColor(GC9A01A_WHITE, GC9A01A_BLACK);
    tft.setTextSize(3);
    
    char pedalStr[10];
    sprintf(pedalStr, "%03d", pedal); 
    tft.setCursor(93, 145);
    tft.print(pedalStr);
    
    pedal_anterior = pedal;
  }
}

void desenharPonteiro(int valor, uint16_t cor) {
  if (valor < 0) valor = 0;
  if (valor > 8000) valor = 8000;
  
  float angulo_graus = map(valor, 0, 8000, 135, 405);
  float angulo_rad = angulo_graus * PI / 180.0;
  
  int x_centro = 120 + 70 * cos(angulo_rad);
  int y_centro = 120 + 70 * sin(angulo_rad);
  
  int x_ponta = 120 + 105 * cos(angulo_rad);
  int y_ponta = 120 + 105 * sin(angulo_rad);
  
  tft.drawLine(x_centro, y_centro, x_ponta, y_ponta, cor);
  
  int x_ponta2 = 120 + 105 * cos(angulo_rad + 0.02);
  int y_ponta2 = 120 + 105 * sin(angulo_rad + 0.02);
  tft.drawLine(x_centro, y_centro, x_ponta2, y_ponta2, cor);
}