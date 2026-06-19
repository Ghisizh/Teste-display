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

#define BOTAO_TELA 19

int telaAtual = 0;

int temperaturaMotor = 85; // valor inicial

bool ultimoEstadoBotao = HIGH;

void desenharFundoEstatico();
void atualizarPainel(int rpm, int pedal);
void desenharPonteiro(int valor, uint16_t cor);
void lerRedeCAN();
void mostrarSplashScreen();
void desenharLogo();
void verificarBotao();
void desenharTelaTemperatura();
void atualizarTelaTemperatura();

void setup() {
  Serial.begin(115200);
  pinMode(BOTAO_TELA, INPUT_PULLUP);
// Inicializa o Ecrã
tft.begin();

mostrarSplashScreen();

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

  verificarBotao();

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
    if (message.identifier == 0xFC) { 
      // Exemplo: Junta 2 bytes (Byte 2 e 3 da mensagem)
      rpm_atual = ((message.data[0] << 8) | message.data[1])/4; 
    }
    
    // EXEMPLO 2: Capturando Pedal (Substitui o ID 0x2B4)
    if (message.identifier == 0x1F0) { 
      pedal_atual = map(((message.data[0] << 8) | message.data[1]), 0, 8000, 0, 100); 
    }

    // Só redesenha se os valores mudaram (mantém o ponteiro suave)
    if (rpm_atual != rpm_anterior || pedal_atual != pedal_anterior) {
      atualizarPainel(rpm_atual, pedal_atual);
    }

    if (message.identifier == 0x123) {
      temperaturaMotor = message.data[0] - 40;
    }

  }

if (telaAtual == 0) {

  if (rpm_atual != rpm_anterior ||
      pedal_atual != pedal_anterior) {

    atualizarPainel(rpm_atual, pedal_atual);
  }
}

if (telaAtual == 1) {

  atualizarTelaTemperatura();
}

}

// ==========================================
// FUNÇÕES DE DESENHO (Visual Branco/Vermelho Desportivo)
// ==========================================

void desenharFundoEstatico() {

  tft.fillScreen(GC9A01A_BLACK);

  // Arco colorido do RPM
  for (int rpm = 0; rpm <= 8000; rpm += 20) {

    float angulo = map(rpm, 0, 8000, 135, 405);
    float rad = angulo * PI / 180.0;

    uint16_t cor;

    if (rpm < 4000)
      cor = GC9A01A_GREEN;

    else if (rpm < 6000)
      cor = GC9A01A_YELLOW;

    else
      cor = GC9A01A_RED;

    for (int esp = 0; esp < 12; esp++) {

      int x = 120 + (100 + esp) * cos(rad);
      int y = 120 + (100 + esp) * sin(rad);

      tft.drawPixel(x, y, cor);
    }
  }

  // Marcas da escala
  for (int i = 0; i <= 8000; i += 500) {

    float angulo = map(i, 0, 8000, 135, 405);
    float rad = angulo * PI / 180.0;

    int x1 = 120 + 90 * cos(rad);
    int y1 = 120 + 90 * sin(rad);

    int x2 = 120 + 115 * cos(rad);
    int y2 = 120 + 115 * sin(rad);

    uint16_t cor;

    if (i < 4000)
      cor = GC9A01A_GREEN;
    else if (i < 6000)
      cor = GC9A01A_YELLOW;
    else
      cor = GC9A01A_RED;

    tft.drawLine(x1, y1, x2, y2, cor);

    if (i % 1000 == 0) {

      char txt[2];
      sprintf(txt, "%d", i / 1000);

      int xt = 120 + 75 * cos(rad);
      int yt = 120 + 75 * sin(rad);

      tft.setTextColor(GC9A01A_WHITE);
      tft.setTextSize(2);
      tft.setCursor(xt - 5, yt - 5);
      tft.print(txt);
    }
  }

  // Centro
  tft.fillCircle(120,120,40,GC9A01A_BLACK);

  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(2);

  tft.setCursor(95,55);
  tft.print("RPM");

  tft.setCursor(70,145);
  tft.print("PEDAL %");
}

void atualizarPainel(int rpm, int pedal) {
  if (rpm != rpm_anterior) {
    if (rpm_anterior >= 0) {
      desenharPonteiro(rpm_anterior, GC9A01A_BLACK);
    }
    
    uint16_t corPonteiro = (rpm >= 6000) ? GC9A01A_RED : GC9A01A_WHITE;
    desenharPonteiro(rpm, corPonteiro);


    uint16_t corRPM;

if(rpm < 4000)
    corRPM = GC9A01A_GREEN;
else if(rpm < 6000)
    corRPM = GC9A01A_YELLOW;
else
    corRPM = GC9A01A_RED;

tft.setTextColor(corRPM, GC9A01A_BLACK);

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

  valor = constrain(valor, 0, 8000);

  float angulo = map(valor, 0, 8000, 135, 405);
  float rad = angulo * PI / 180.0;

  int x1 = 120;
  int y1 = 120;

  int x2 = 120 + 105 * cos(rad);
  int y2 = 120 + 105 * sin(rad);

  tft.drawLine(x1, y1, x2, y2, cor);
  tft.drawLine(x1+1, y1, x2+1, y2, cor);
  tft.drawLine(x1-1, y1, x2-1, y2, cor);

  tft.fillCircle(120,120,6,cor);
}

// ==========================================
// SPLASH SCREEN
// ==========================================

void desenharLogo() {

  uint16_t verde = GC9A01A_GREEN;
  uint16_t vermelho = GC9A01A_RED;

  int s = 18;
  int gap = 4;

  // posição da logo
  int x0 = 82;
  int y0 = 45;

  // círculo vermelho
  tft.fillCircle(
    x0 + s/2,
    y0 + s/2,
    s/2,
    vermelho
  );

  // linha superior
  tft.fillRoundRect(x0 + (s+gap), y0, s, s, 3, verde);
  tft.fillRoundRect(x0 + 2*(s+gap), y0, s, s, 3, verde);

  // linha 2
  tft.fillRoundRect(x0, y0 + (s+gap), s, s, 3, verde);
  tft.fillRoundRect(x0 + (s+gap), y0 + (s+gap), s, s, 3, verde);

  // linha 3
  tft.fillRoundRect(x0, y0 + 2*(s+gap), s, s, 3, verde);
  tft.fillRoundRect(x0 + (s+gap), y0 + 2*(s+gap), s, s, 3, verde);
  tft.fillRoundRect(x0 + 2*(s+gap), y0 + 2*(s+gap), s, s, 3, verde);

  // linha 4
  tft.fillRoundRect(x0, y0 + 3*(s+gap), s, s, 3, verde);
  tft.fillRoundRect(x0 + (s+gap), y0 + 3*(s+gap), s, s, 3, verde);
}

void mostrarSplashScreen() {

  tft.fillScreen(GC9A01A_BLACK);

  desenharLogo();

  delay(3000);

  tft.fillScreen(GC9A01A_BLACK);
}

void verificarBotao() {

  bool estadoAtual = digitalRead(BOTAO_TELA);

  // Detecta apenas o clique
  if (ultimoEstadoBotao == HIGH && estadoAtual == LOW) {

    telaAtual++;

    if (telaAtual > 1)
      telaAtual = 0;

    tft.fillScreen(GC9A01A_BLACK);

    if (telaAtual == 0) {
      desenharFundoEstatico();
      rpm_anterior = -1;
      pedal_anterior = -1;
    }

    if (telaAtual == 1) {
      desenharTelaTemperatura();
    }

  }

  ultimoEstadoBotao = estadoAtual;
}

void desenharTelaTemperatura() {

  tft.fillScreen(GC9A01A_BLACK);

  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(2);

  tft.setCursor(30,40);
  tft.print("TEMPERATURA");

  tft.drawCircle(120,130,70,GC9A01A_WHITE);
}

void atualizarTelaTemperatura() {

  tft.fillRect(50,100,140,60,GC9A01A_BLACK);

  uint16_t cor;

  if(temperaturaMotor < 95)
    cor = GC9A01A_GREEN;
  else if(temperaturaMotor < 105)
    cor = GC9A01A_YELLOW;
  else
    cor = GC9A01A_RED;

  tft.setTextColor(cor);
  tft.setTextSize(5);

  tft.setCursor(70,105);
  tft.print(temperaturaMotor);

  tft.setTextSize(2);
  tft.print("C");
}