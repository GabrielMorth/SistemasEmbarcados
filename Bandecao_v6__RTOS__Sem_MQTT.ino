// Inclusão das bibliotecas
#include <WiFi.h>
#include <PubSubClient.h>
#include "HX711.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <ArduinoOTA.h>


// Definição de parâmetros e associação de valores
#define ON 1
#define OFF 0
#define motor 5

Preferences preferences;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP); // estabelece conexão WiFi

WiFiClient espClient;
PubSubClient client(espClient); // estabelece conexão WiFi

LiquidCrystal_I2C lcd(0x27, 16, 2); // determina LCD utilizado

HX711 scale;
HX711 scale2;

// Referencia as funções e variáveis usadas na main
void ligarMotor();
void desligarMotor();
void sendDataBalanca();
void agendarHorario(void);
void setup_balancas();
int obterAcao(int estado, int evento);
int obterProximoEstado(int estado, int evento);
int executarAcao(int codigoAcao);
int obterEvento();
void botao1_setup(int pin);
void botao2_setup(int pin);
void botao3_setup(int pin);
void botao4_setup(int pin);
void botaomodo_setup(int pin);
void botao1_update();
void botao2_update();
void botao3_update();
void botao4_update();
void botaomodo_update();
void setPesoReservatorio();
void setPesoTigela();
String payload_to_string();
void printa_peso(double peso, double peso2);
void relogio_incrementa(void);
void setup_wifi();
void setup_mqtt();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();

int horario1 = -1;
int minuto1 = -1;
int horario2 = -1;
int minuto2 = -1;
bool ja_agendou = false;
bool relogio_mudou_segundos = false;
int relogio_segundos = 0;
int relogio_minutos1 = 0;
int relogio_horas1 = 0;
int relogio_minutos2 = 0;
int relogio_horas2 = 0;
static int tick = 0;
int horarioAnt;
int minutoAnt;
int p1_ant = 0;
int p2_ant = 0;

// Define os estados e eventos
#define EXIBICAO 0
#define AGENDAMENTO 1
#define BOTAO1 0
#define BOTAO2 1
#define BOTAO3 2
#define BOTAO4 3
#define BOTAO_MODO 4
#define MOTOR_CELULAR 5
#define HORARIO_AGENDADO 6
#define PROGRAMACAO_CELULAR 7
#define NENHUM_EVENTO 8

#define A01 1
#define A02 2
#define A03 3
#define A04 4
#define A05 5
#define A06 6
#define A07 7
#define A08 8
#define A09 9
#define NENHUMA_ACAO 0

#define NUM_ESTADOS 2
#define NUM_EVENTOS 9
#define MAX_EVENTO 50

unsigned long EventoInstante[MAX_EVENTO];
int EventoTipo[MAX_EVENTO], EventoDado[MAX_EVENTO];
int numeroEventos = 0;
const unsigned long kDebounceDelay = 1000;
bool botao1_botao_apertado;
bool botao2_botao_apertado;
bool botao3_botao_apertado;
bool botao4_botao_apertado;
bool botaomodo_botao_apertado;

int botao1_pin = 13;
int botao2_pin = 12;
int botao3_pin = 14;
int botao4_pin = 27;
int botaomodo_pin = 26;

unsigned long botao1_onTime;
unsigned long botao2_onTime;
unsigned long botao3_onTime;
unsigned long botao4_onTime;
unsigned long botaomodo_onTime;

double peso = 0, peso2 = 0;
double reading, reading2;

const int LOADCELL_DOUT_PIN = 32;
const int LOADCELL_SCK_PIN = 33;
const int LOADCELL_DOUT_PIN2 = 2;
const int LOADCELL_SCK_PIN2 = 4;

double CALIBRATION_FACTOR = 0;
double CALIBRATION_FACTOR2 = 0;
double tareReservatorio = 0;
double tareTigela = 0;

bool eventoCelularRecebido = false;

// Referencia as funções necessárias para máquina de estados
int acao_matrizTransicaoEstados[NUM_ESTADOS][NUM_EVENTOS];
int proximo_estado_matrizTransicaoEstados[NUM_ESTADOS][NUM_EVENTOS];

// Referencia a função que inicia a máquina de estados 
void iniciaMaquinaEstados();

// Referencia as variáveis usadas e as funções utilizadas na máquina de estados
int estado;
int eventoInterno;

// define parâmetros iniciais
int h1 = 0;
int h2 = 0;
int m1 = 0;
int m2 = 9;
int codigoEvento;
int codigoAcao;
int comando_pro_motor = OFF;
int estadoMotor = OFF;

TaskHandle_t Task1, Task2, Task3, Task4;

void setup() {
  Serial.begin(115200);
  setup_wifi();
  setup_mqtt();

  lcd.init(); // Inicializa LCD
  lcd.clear();
  lcd.backlight();

  setup_balancas();

  pinMode(botao1_pin, INPUT_PULLUP);
  pinMode(botao2_pin, INPUT_PULLUP);
  pinMode(botao3_pin, INPUT_PULLUP);
  pinMode(botao4_pin, INPUT_PULLUP);
  pinMode(botaomodo_pin, INPUT_PULLUP);

  botao1_setup(13);
  botao2_setup(12);
  botao3_setup(14);
  botao4_setup(27);
  botaomodo_setup(26);

  pinMode(motor, OUTPUT); // Inicializa motor
  digitalWrite(motor, HIGH);

  delay(500); // Espera 0,5 segundos

  iniciaMaquinaEstados();

  estado = EXIBICAO;
  eventoInterno = NENHUM_EVENTO;

  xTaskCreatePinnedToCore(Task1code, "Task1", 10000, NULL, 1, &Task1, 0);
  xTaskCreatePinnedToCore(Task2code, "Task2", 10000, NULL, 1, &Task2, 1);
  xTaskCreatePinnedToCore(Task3code, "Task3", 10000, NULL, 1, &Task3, 0);
  xTaskCreatePinnedToCore(Task4code, "Task4", 10000, NULL, 1, &Task4, 1);
}

void Task1code(void * parameter) {
  for (;;) {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    delay(10);
  }
}

void Task2code(void * parameter) {
  for (;;) {
    botao1_update();
    botao2_update();
    botao3_update();
    botao4_update();
    botaomodo_update();
    delay(10);
  }
}

void Task3code(void * parameter) {
  for (;;) {
    setPesoReservatorio(round(scale.get_units()));
    setPesoTigela(round(scale2.get_units()));

    if (reading >= peso * 1.1 || reading <= peso * 0.9) {
      peso = reading;
    }
    if (reading2 >= peso2 * 1.1 || reading2 <= peso2 * 0.9) {
      peso2 = reading2;
    }
    if (estado == EXIBICAO) {
      printa_peso(peso, peso2);
    }
    delay(10);
  }
}

void Task4code(void * parameter) {
  for (;;) {
    relogio_incrementa();
    if (eventoInterno == NENHUM_EVENTO) {
      codigoEvento = obterEvento();
    } else {
      codigoEvento = eventoInterno;
    }
    if (codigoEvento != NENHUM_EVENTO) {
      codigoAcao = obterAcao(estado, codigoEvento);
      estado = obterProximoEstado(estado, codigoEvento);
      eventoInterno = executarAcao(codigoAcao);
      printf("Estado: %d, Evento: %d, Acao: %d, Prox estado: %d, Evento interno: %d\n", estado, codigoEvento, codigoAcao, obterProximoEstado(estado, codigoEvento), eventoInterno);
    }
    delay(10);
  }
}

// Implementação das outras funções necessárias para o seu código
// Coloque todas as funções auxiliares e lógicas aqui.

void loop() {
  // O loop principal é agora gerenciado pelas tasks, então pode estar vazio.
}



void iniciaMaquinaEstados() {
  int i;
  int j;
  for (i = 0; i < NUM_ESTADOS; i++) {
    for (j = 0; j < NUM_EVENTOS; j++) {
      // cria matrizes genéricas com as dimensões corretas a fim de se preencher em seguida com os estados e ações
      acao_matrizTransicaoEstados[i][j] = NENHUMA_ACAO;
      proximo_estado_matrizTransicaoEstados[i][j] = i; 
    }
  }

  // Configuração das transições e ações
  proximo_estado_matrizTransicaoEstados[EXIBICAO][MOTOR_CELULAR] = EXIBICAO;
  acao_matrizTransicaoEstados[EXIBICAO][MOTOR_CELULAR] = A07;

  proximo_estado_matrizTransicaoEstados[EXIBICAO][HORARIO_AGENDADO] = EXIBICAO;
  acao_matrizTransicaoEstados[EXIBICAO][HORARIO_AGENDADO] = A09; // LIGA O MOTOR POR 5 SEUGUNDOS

  proximo_estado_matrizTransicaoEstados[EXIBICAO][PROGRAMACAO_CELULAR] = EXIBICAO;
  acao_matrizTransicaoEstados[EXIBICAO][PROGRAMACAO_CELULAR] = A02;

  proximo_estado_matrizTransicaoEstados[EXIBICAO][BOTAO1] = EXIBICAO;
  acao_matrizTransicaoEstados[EXIBICAO][BOTAO1] = A09;

  proximo_estado_matrizTransicaoEstados[EXIBICAO][BOTAO_MODO] = AGENDAMENTO;
  acao_matrizTransicaoEstados[EXIBICAO][BOTAO_MODO] = A01;

  proximo_estado_matrizTransicaoEstados[AGENDAMENTO][BOTAO_MODO] = EXIBICAO;
  acao_matrizTransicaoEstados[AGENDAMENTO][BOTAO_MODO] = A02;

  proximo_estado_matrizTransicaoEstados[AGENDAMENTO][BOTAO1] = AGENDAMENTO;
  acao_matrizTransicaoEstados[AGENDAMENTO][BOTAO1] = A03;

  proximo_estado_matrizTransicaoEstados[AGENDAMENTO][BOTAO2] = AGENDAMENTO;
  acao_matrizTransicaoEstados[AGENDAMENTO][BOTAO2] = A04;

  proximo_estado_matrizTransicaoEstados[AGENDAMENTO][BOTAO3] = AGENDAMENTO;
  acao_matrizTransicaoEstados[AGENDAMENTO][BOTAO3] = A05;

  proximo_estado_matrizTransicaoEstados[AGENDAMENTO][BOTAO4] = AGENDAMENTO;
  acao_matrizTransicaoEstados[AGENDAMENTO][BOTAO4] = A06;

  proximo_estado_matrizTransicaoEstados[EXIBICAO][BOTAO2] = EXIBICAO;


  proximo_estado_matrizTransicaoEstados[EXIBICAO][BOTAO3] = EXIBICAO;

  proximo_estado_matrizTransicaoEstados[EXIBICAO][BOTAO4] = EXIBICAO;
}

// Funções para obter evento, ação, próximo estado e executar ação
int obterEvento() {
  int evento = NENHUM_EVENTO;
  bool condicao = false;

  // Verifica os estados dos botões e outros eventos
  if (digitalRead(botao1_pin) == LOW) {
    condicao = true;
    Serial.print("BOTAO 1 PRESSIONADO\n");
    evento = BOTAO1; // LIGA O MOTOR POR 5 SEGUNDOS
  } else if (digitalRead(botao2_pin) == LOW) {
    condicao = true;
    Serial.print("BOTAO 2 PRESSIONADO\n");
    evento = BOTAO2; // FAZ A AÇÃO 4 - MUDA O H2
  } else if (digitalRead(botao3_pin) == LOW) {
    condicao = true;
    evento = BOTAO3;
    Serial.print("BOTAO 3 PRESSIONADO\n");
  } else if (digitalRead(botao4_pin) == LOW) {
    condicao = true;
    evento = BOTAO4;
    Serial.print("BOTAO 4 PRESSIONADO\n");
  } else if (digitalRead(botaomodo_pin) == LOW) {
    condicao = true;
    evento = BOTAO_MODO;
    delay(200);
    Serial.print("BOTAO MODO PRESSIONADO\n");
  } else if (horario1 == relogio_horas1 && minuto1 == relogio_minutos1 && horario2 == relogio_horas2 && minuto2 == relogio_minutos2) {
    condicao = true;
    evento = HORARIO_AGENDADO;
  } else if (eventoCelularRecebido) {
    condicao = true;
    evento = MOTOR_CELULAR;
    eventoCelularRecebido = false;  // Reseta a flag após o evento ser processado
    Serial.println("ENTROU AQUI ");
    Serial.print("comando_pro_motor: ");
    Serial.println(comando_pro_motor);
  }

  switch (condicao) {
    case true:
      break;
    default:
      // Adicione outras verificações de eventos, se necessário
      break;
  }

  return evento;
}

// Obtem a ação de acordo com o estado e evento fornecidos
int obterAcao(int estado, int evento) {
  return acao_matrizTransicaoEstados[estado][evento];
}

// Obtem o próximo estado de acordo com o estado e evento fornecidos
int obterProximoEstado(int estado, int evento) {
  return proximo_estado_matrizTransicaoEstados[estado][evento];
}

int executarAcao(int codigoAcao) {
  int retval;
  retval = NENHUM_EVENTO;
  if (codigoAcao == NENHUMA_ACAO) return retval;
  switch (codigoAcao) {
    case A01:
      Serial.print("Caso A01 - Mudar modo de exibição para agendamento ou agendamento para exibição\n");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.setCursor(0,1);
      lcd.print("Agendamento");
      if((10*h1+h2)>=24){
      h1=0;
      h2=0;
      }
      if((10*m1+m2)>=60){
        m1=0;
        m2=0;
      }
      if(h2>9){
        h2=0;
      }
      if(m2>9){
        m2=0;
      }
      lcd.setCursor(5,0);
      lcd.print(h1);
      lcd.print(h2);
      lcd.print(":");
      lcd.print(m1);
      lcd.print(m2);
      return NENHUM_EVENTO;
      break;
    case A02:
      // Implementação da ação A02
      lcd.clear();
      Serial.print("Caso A02 - Salvar horario de agendamento\n");
      agendarHorario();
      lcd.print("Horario Salvo");
      delay(500);
      relogio_horas1 = 0;
      relogio_minutos1 = 0;
      relogio_horas2 = 0;
      relogio_minutos2 = 0;
      relogio_segundos = 0; 
      return NENHUM_EVENTO;
      break;
    case A03:
      lcd.clear();
      Serial.print("Caso A03 - Muda primeiro dígito da hora\n");
      // Implementação da ação A03
      h1 = (h1 + 1) % 3; // Ajuste conforme necessário para o formato de hora
      lcd.setCursor(0, 0);
      lcd.setCursor(0,1);
      lcd.print("Agendamento");
      if((10*h1+h2)>=24){
      h1=0;
      h2=0;
      }
      if((10*m1+m2)>=60){
        m1=0;
        m2=0;
      }
      if(h2>9){
        h2=0;
      }
      if(m2>9){
        m2=0;
      }
      lcd.setCursor(5,0);
      lcd.print(h1);
      lcd.print(h2);
      lcd.print(":");
      lcd.print(m1);
      lcd.print(m2);      
      return NENHUM_EVENTO;
      break;
    case A04:
      lcd.clear();
      Serial.print("Caso A04 - Muda segundo dígito da hora\n");
      // Implementação da ação A04
      h2 = (h2 + 1) % 10;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.setCursor(0,1);
      lcd.print("Agendamento");
      if((10*h1+h2)>=24){
      h1=0;
      h2=0;
      }
      if((10*m1+m2)>=60){
        m1=0;
        m2=0;
      }
      if(h2>9){
        h2=0;
      }
      if(m2>9){
        m2=0;
      }
      lcd.setCursor(5,0);
      lcd.print(h1);
      lcd.print(h2);
      lcd.print(":");
      lcd.print(m1);
      lcd.print(m2);      
      return NENHUM_EVENTO;
      break;
    case A05:
      Serial.print("Caso A05 - Muda primeiro dígito dos minutos\n");
      // Implementação da ação A05
      m1 = (m1 + 1) % 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.setCursor(0,1);
      lcd.print("Agendamento");
      if((10*h1+h2)>=24){
      h1=0;
      h2=0;
      }
      if((10*m1+m2)>=60){
        m1=0;
        m2=0;
      }
      if(h2>9){
        h2=0;
      }
      if(m2>9){
        m2=0;
      }
      lcd.setCursor(5,0);
      lcd.print(h1);
      lcd.print(h2);
      lcd.print(":");
      lcd.print(m1);
      lcd.print(m2);
      return NENHUM_EVENTO;
      break;
    case A06:
      Serial.print("Caso A05 - Muda segundo dígito dos minutos\n");
      // Implementação da ação A06
      m2 = (m2 + 1) % 10;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.setCursor(0,1);
      lcd.print("Agendamento");
      if((10*h1+h2)>=24){
      h1=0;
      h2=0;
      }
      if((10*m1+m2)>=60){
        m1=0;
        m2=0;
      }
      if(h2>9){
        h2=0;
      }
      if(m2>9){
        m2=0;
      }
      lcd.setCursor(5,0);
      lcd.print(h1);
      lcd.print(h2);
      lcd.print(":");
      lcd.print(m1);
      lcd.print(m2);
      return NENHUM_EVENTO;
      break;
    case A07:
      // Implementação da ação A07
      if (comando_pro_motor == ON){
        lcd.clear();
        lcd.print("Motor ON");
        ligarMotor();
        delay(3000);
        lcd.clear();
      } else {
        if (comando_pro_motor == OFF){
          desligarMotor();
          lcd.clear();
          lcd.print("Motor OFF");
          delay(500);
          lcd.clear();
        }
      }
      return NENHUM_EVENTO;
      break;
    case A08:
      // Implementação da ação A08
      lcd.clear();
      return NENHUM_EVENTO;
      break;
    case A09:
      // Serial.print("Caso A09 - Liga/desliga motor\n");
      // Implementação da ação A09
      lcd.clear();
      lcd.print("Motor ON");
      ligarMotor();
      delay(3000); // Exemplo de duração
      desligarMotor();
      lcd.clear();
      lcd.print("Motor OFF");
      delay(500);
      lcd.clear();
      relogio_horas1 = 0;
      relogio_minutos1 = 0;
      relogio_horas2 = 0;
      relogio_minutos2 = 0;
      relogio_segundos = 0; 
      return NENHUM_EVENTO;
      break;
    default:
      return NENHUM_EVENTO;
      break;
  }
  return retval;
}

void ligarMotor() {
  estadoMotor = ON;
}

void desligarMotor() {
  estadoMotor = OFF;
}

void agendarHorario(void) {
  horario1 = h1;
  minuto1 = m1;
  horario2 = h2;
  minuto2 = m2;
}



void acrescentaEvento(unsigned long instante, int tipo, int dado){
  int i = 0;
  if (numeroEventos == MAX_EVENTO) return;
    for (int i = 0 ; i < numeroEventos ; i ++)
      if (EventoInstante[i] < instante) {
        for (int j = numeroEventos ; j > i - 1 ; j --) {
          EventoInstante[j] = EventoInstante[j - 1];
          EventoTipo[j] = EventoTipo[j - 1];
          EventoDado[j] = EventoDado[j - 1];
        }
      break;
    }
    EventoInstante[i] = instante;
    EventoTipo[i] = tipo; EventoDado[i] = dado; numeroEventos ++;
}

void botao1_setup(int pin) {
  botao1_botao_apertado = false;
  botao1_onTime = 0;
  botao1_pin = pin;
};

void botao2_setup(int pin) {
  botao2_botao_apertado = false;
  botao2_onTime = 0;
  botao2_pin = pin;
};

void botao3_setup(int pin) {
  botao3_botao_apertado = false;
  botao3_onTime = 0;
  botao3_pin = pin;
};

void botao4_setup(int pin) {
  botao4_botao_apertado = false;
  botao4_onTime = 0;
  botao4_pin = pin;
};

void botaomodo_setup(int pin) {
  botaomodo_botao_apertado = false;
  botaomodo_onTime = 0;
  botaomodo_pin = pin;
};

// Função de atualização para botao1
void botao1_update(void) {
  if (digitalRead(botao1_pin) == LOW) {
    if (botao1_botao_apertado == false) {
      acrescentaEvento(millis(), BOTAO1, 0);
      botao1_onTime = millis();
    }
    botao1_botao_apertado = true;
  } else {
    if (botao1_onTime > 0 && millis() - botao1_onTime > kDebounceDelay) {
      botao1_botao_apertado = false;
      botao1_onTime = 0;
    }
  }
}

// Função de atualização para botao2
void botao2_update(void) {
  if (digitalRead(botao2_pin) == LOW) {
    if (botao2_botao_apertado == false) {
      acrescentaEvento(millis(), BOTAO2, 0);
      botao2_onTime = millis();
    }
    botao2_botao_apertado = true;
  } else {
    if (botao2_onTime > 0 && millis() - botao2_onTime > kDebounceDelay) {
      botao2_botao_apertado = false;
      botao2_onTime = 0;
    }
  }
}

// Função de atualização para botao3
void botao3_update(void) {
  if (digitalRead(botao3_pin) == LOW) {
    if (botao3_botao_apertado == false) {
      acrescentaEvento(millis(), BOTAO3, 0);
      botao3_onTime = millis();
    }
    botao3_botao_apertado = true;
  } else {
    if (botao3_onTime > 0 && millis() - botao3_onTime > kDebounceDelay) {
      botao3_botao_apertado = false;
      botao3_onTime = 0;
    }
  }
}

// Função de atualização para botao4
void botao4_update(void) {
  if (digitalRead(botao4_pin) == LOW) {
    if (botao4_botao_apertado == false) {
      acrescentaEvento(millis(), BOTAO4, 0);
      botao4_onTime = millis();
    }
    botao4_botao_apertado = true;
  } else {
    if (botao4_onTime > 0 && millis() - botao4_onTime > kDebounceDelay) {
      botao4_botao_apertado = false;
      botao4_onTime = 0;
    }
  }
}

// Função de atualização para botaomodo
void botaomodo_update(void) {
  if (digitalRead(botaomodo_pin) == LOW) {
    if (botaomodo_botao_apertado == false) {
      acrescentaEvento(millis(), BOTAO_MODO, 0);
      botaomodo_onTime = millis();
    }
    botaomodo_botao_apertado = true;
  } else {
    if (botaomodo_onTime > 0 && millis() - botaomodo_onTime > kDebounceDelay) {
      botaomodo_botao_apertado = false;
      botaomodo_onTime = 0;
    }
  }
}


void setup_balancas() {
  Serial.println("Initializing the scale");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);   // Inicializa balanca 1
  scale2.begin(LOADCELL_DOUT_PIN2, LOADCELL_SCK_PIN2);
  scale2.set_scale(CALIBRATION_FACTOR2);   // Inicializa balanca 2 
}

void setPesoReservatorio(double peso) {
  reading = (double) (peso + tareReservatorio) / 1000;
}

void setPesoTigela(double peso) {
  reading2 = peso + tareTigela;
}

String payload_to_string(byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += ((char)payload[i]);
  }
  return message;
}


void printa_peso(double peso, double peso2){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Reserva: ");
  if(peso<0) {
    lcd.print("0");
    lcd.print(" Kg");
  }
  else{
    lcd.print(peso);
    lcd.print("Kg");
  }
  lcd.setCursor(0,1);
  lcd.print("Tigela: ");
  if(peso2<0) {
    lcd.print(0);
    lcd.print("g");
  }
  else {
    lcd.print(peso2);
    lcd.print(" g");
  } 
}



void relogio_incrementa(void) {
  relogio_mudou_segundos = true;
  tick += 1; 
  if (tick == 5){
  relogio_segundos ++;
    tick = 0; 
  }
  if (relogio_segundos == 60) {
    relogio_segundos = 0; relogio_minutos2 ++;
  }
  if (relogio_minutos2 == 10){
    relogio_minutos1 = 0;
    relogio_minutos1 ++;
  }
  if (relogio_minutos1 == 6) {
    relogio_minutos1 = 0; relogio_minutos2 = 0; relogio_horas2 ++;
  }
  if (relogio_horas2 == 10){
    relogio_horas2 = 0;
    relogio_horas1 ++;
  } 
    if (relogio_horas1 == 2 && relogio_horas2 == 4){
    relogio_horas1 = 0; relogio_horas2 = 0; relogio_minutos1 = 0; relogio_minutos2 = 0; 
  } 
  // acrescentaEvento(millis() + 1000, EVENTO_INCREMENTA_RELOGIO,
  // 0);
}


void setup_mqtt() {
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println(length);
  Serial.println(" length ");
  String topico = topic;
  if (topico.equals(topico_motor)) {
    eventoCelularRecebido = true;
    comando_pro_motor = !comando_pro_motor;
    Serial.println("Botão Celular Pressionado");
  } 
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_device, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(topico_motor);
      // client.subscribe(topico_agendamento);
    } else {
      Serial.print("failed, state: ");
      Serial.print(client.state());
      Serial.println(" try again ");
      delay(500);

    }
  }
}


void setup_wifi() {
  Serial.print("connect ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(WiFi.status());
    Serial.print(" \n");
    delay(100);
    Serial.println(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP addredd: ");
  Serial.println(WiFi.localIP());
}
