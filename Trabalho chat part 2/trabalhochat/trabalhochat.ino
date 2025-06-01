#include <WiFi.h>
#include <HTTPClient.h>
#include <UrlEncode.h>

// --- Configurações de Rede e API ---
const char* ssid = "Redmi Note 7";
const char* password = "jtct7903"; // Considere armazenar senhas de forma mais segura se necessário

// --- Pinos ---
const int sensorPin = 21;      // Pino do sensor de presença
const int ledVermelho = 19;  // Pino do LED vermelho
const int ledAmarelo = 18;   // Pino do LED amarelo
const int ledVerde = 5;      // Pino do LED verde

// --- Configurações da API CallMeBot ---
String phoneNumber = "+554192270628"; // Seu número de telefone com código do país
String apiKey = "4467246";          // Sua API Key do CallMeBot

// --- URL da Imagem do Alerta ---
// !!! IMPORTANTE: Substitua pela URL DIRETA da imagem que você hospedou online !!!
// Exemplo: "https://i.imgur.com/seu_codigo_de_imagem.jpg"
const char* alertImageUrl = "https://i.postimg.cc/6QRWKL0K/fa-a-o-codigo-de-uma-imagen-de-um-bandido-comun-abrindo-uma-ma-aneta-de-uma-porta.jpg";

// --- Definição do tipo de Callback ---
typedef void (*MessageSentCallback)(bool success, int httpCode);

// --- Otimização de Memória: Histórico de Alertas ---
const int MAX_HISTORY_EVENTS = 10;
struct AlertEvent {
  unsigned long timestamp;
  bool messageSentSuccessfully;
  int responseCode;
};
AlertEvent alertHistory[MAX_HISTORY_EVENTS];
int currentEventIndex = 0;
int eventCount = 0;

// --- Variáveis para controle de tempo do alerta (envio de mensagem) ---
unsigned long lastAlertTime = 0;
const unsigned long alertCooldown = 5000; // Cooldown em ms para ENVIO DE MENSAGENS.

// --- Variáveis para otimização da detecção de movimento ---
const unsigned long MOTION_STABILITY_THRESHOLD_MS = 200; // Milissegundos que o sensor deve ficar HIGH para confirmar movimento.
unsigned long potentialMotionStartTime = 0;
bool isPotentialMotion = false;
bool ongoingValidatedMotion = false;

void addAlertToHistory(bool success, int httpCode) {
  alertHistory[currentEventIndex].timestamp = millis();
  alertHistory[currentEventIndex].messageSentSuccessfully = success;
  alertHistory[currentEventIndex].responseCode = httpCode;

  currentEventIndex = (currentEventIndex + 1) % MAX_HISTORY_EVENTS;
  if (eventCount < MAX_HISTORY_EVENTS) {
    eventCount++;
  }
  Serial.println("Evento de alerta adicionado ao histórico.");
}

void sendMessage(String message, MessageSentCallback callback) {
  String encodedMessage = urlEncode(message);
  String url;
  url.reserve(250 + phoneNumber.length() + apiKey.length() + encodedMessage.length()); // Ajuste a reserva conforme o tamanho da URL da imagem

  url = "https://api.callmebot.com/whatsapp.php?phone=";
  url += phoneNumber;
  url += "&apikey=";
  url += apiKey;
  url += "&text=";
  url += encodedMessage;

  HTTPClient http;
  Serial.print("URL CallMeBot: ");
  Serial.println(url);

  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpResponseCode = http.GET();

  bool success = false;
  if (httpResponseCode > 0) {
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
    if (httpResponseCode == 200) {
      Serial.println("Mensagem enviada com sucesso via CallMeBot.");
      success = true;
    } else {
      Serial.println("Erro no envio da mensagem via CallMeBot (código HTTP não foi 200).");
    }
  } else {
    Serial.print("Erro na conexão HTTP ou envio: ");
    Serial.println(http.errorToString(httpResponseCode).c_str());
  }
  http.end();
  if (callback) {
    callback(success, httpResponseCode);
  }
}

void handleMessageSent(bool success, int httpCode) {
  Serial.print("Callback executado: Envio ");
  Serial.print(success ? "bem-sucedido" : "falhou");
  Serial.print(", HTTP Code: ");
  Serial.println(httpCode);
  addAlertToHistory(success, httpCode);
}

void setup() {
  Serial.begin(115200);

  pinMode(sensorPin, INPUT);
  pinMode(ledVermelho, OUTPUT);
  pinMode(ledAmarelo, OUTPUT);
  pinMode(ledVerde, OUTPUT);

  digitalWrite(ledVermelho, LOW);
  digitalWrite(ledAmarelo, LOW);
  digitalWrite(ledVerde, LOW);

  Serial.println("Sistema de Alerta de Movimento Iniciado.");
  Serial.println("Alertas incluirão um link para uma imagem hospedada.");
  if (String(alertImageUrl) == "https://i.postimg.cc/6QRWKL0K/fa-a-o-codigo-de-uma-imagen-de-um-bandido-comun-abrindo-uma-ma-aneta-de-uma-porta.jpg") {
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    Serial.println("!!! ATENÇÃO: Atualize a variável 'alertImageUrl' com a URL da sua imagem !!!");
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
  } else {
    Serial.print("URL da Imagem para Alertas: ");
    Serial.println(alertImageUrl);
  }


  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startTime > 20000) {
      Serial.println("\nFalha ao conectar ao WiFi!");
      return;
    }
  }
  Serial.println("\nConectado!");
  Serial.print("Endereço IP do ESP32: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  int sensorValue = digitalRead(sensorPin);
  unsigned long currentTime = millis();

  if (sensorValue == HIGH) {
    if (!isPotentialMotion) {
      isPotentialMotion = true;
      potentialMotionStartTime = currentTime;
      Serial.println("Sensor HIGH. Verificando estabilidade...");
    } else {
      if (!ongoingValidatedMotion && (currentTime - potentialMotionStartTime >= MOTION_STABILITY_THRESHOLD_MS)) {
        ongoingValidatedMotion = true;
        Serial.println("MOVIMENTO ESTÁVEL CONFIRMADO!");

        if (currentTime - lastAlertTime > alertCooldown) {
          lastAlertTime = currentTime;
          Serial.println("Movimento estável detectado E fora do cooldown de alerta. Acionando alerta...");

          digitalWrite(ledVermelho, HIGH); delay(250); digitalWrite(ledVermelho, LOW);
          digitalWrite(ledAmarelo, HIGH); delay(250); digitalWrite(ledAmarelo, LOW);
          digitalWrite(ledVerde, HIGH); delay(250); digitalWrite(ledVerde, LOW);

          String alertMessage = "ALERTA DE INVASÃO!\n\nMovimento detectado!\n\n";
          alertMessage += "Imagem do evento:\n";
          alertMessage += alertImageUrl; // Adiciona a URL da imagem à mensagem

          Serial.println("--- Mensagem de Alerta ---");
          Serial.println(alertMessage);
          Serial.println("--------------------------");

          Serial.print("Enviando mensagem para CallMeBot...");
          sendMessage(alertMessage, handleMessageSent);
          Serial.println("Alerta enviado. Aguardando próximo evento após cooldown de " + String(alertCooldown / 1000) + "s.");
        } else {
          Serial.println("Movimento estável detectado, mas DENTRO do cooldown de alerta. Nenhuma mensagem será reenviada.");
        }
      }
    }
  } else {
    if (isPotentialMotion) {
      Serial.println("Sensor LOW. Fim da sequência de movimento atual.");
      isPotentialMotion = false;
      ongoingValidatedMotion = false;
    }
  }
  delay(50);
}