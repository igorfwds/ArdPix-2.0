/**
 * ArdPix - Firmware ESP32 com Keypad 4x4 (Modo ATM)
 * Digitação numérica direta
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Keypad.h>

// ==================== CORREÇÃO DE BROWNOUT ====================
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ==================== CONFIGURAÇÕES WIFI/MQTT ====================

const char* WIFI_SSID     = "uaifai-tiradentes";
const char* WIFI_PASSWORD = "bemvindoaocesar";

const char* MQTT_BROKER    = "4d976cc55da74717896f08453a044a5b.s1.eu.hivemq.cloud";
const int   MQTT_PORT      = 8883;
const char* MQTT_CLIENT_ID = "ardpix-esp32-keypad";
const char* MQTT_USERNAME  = "flash-dashboard";
const char* MQTT_PASSWORD  = "#azy8R5VD0QE%Y";

// Tópicos
const char* TOPIC_VALUE_UPDATE = "ardpix/value/update";
const char* TOPIC_PIX_REQUEST  = "ardpix/pix/request";
const char* TOPIC_PIX_RESPONSE = "ardpix/pix/response";
const char* TOPIC_STATUS       = "ardpix/status";

// ==================== CONFIGURAÇÃO DO KEYPAD ====================

const byte ROWS = 4; 
const byte COLS = 4; 

char keys[ROWS][COLS] = {
  {'1','2','3','A'}, // A = APAGAR ÚLTIMO (Backspace)
  {'4','5','6','B'}, // B = (Sem uso)
  {'7','8','9','C'}, // C = LIMPAR TUDO (Clear)
  {'*','0','#','D'}  // D = CONFIRMAR (Enter)
};

// Pinos Seguros ESP32
byte rowPins[ROWS] = {13, 12, 14, 27}; 
byte colPins[COLS] = {26, 25, 33, 32}; 

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ==================== VARIÁVEIS ====================

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

// Usamos INTEIRO para guardar centavos (evita erros de arredondamento)
long currentCents = 0; 

bool pixRequested = false;
unsigned long lastStatusLog = 0;

// ==================== PROTOTIPOS ====================
void setupWiFi();
void setupMQTT();
void connectMQTT();
void sendValueUpdate();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void handleKey(char key);

// ==================== SETUP ====================

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== ArdPix (Modo ATM) Iniciado ===");

    setupWiFi();
    setupMQTT();
}

// ==================== LOOP ====================

void loop() {
    unsigned long now = millis();

    // 1. Ler Teclado
    char key = keypad.getKey();
    if (key) {
        handleKey(key);
    }

    // 2. Conexão MQTT
    if (WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) {
            connectMQTT();
        } else {
            mqttClient.loop();
        }
    }

    // 3. Status periódico
    if (now - lastStatusLog > 10000) {
        lastStatusLog = now;
        if (mqttClient.connected()) {
            JsonDocument doc;
            doc["connected"] = true;
            // Converte centavos para float na hora de enviar
            doc["current_value"] = currentCents / 100.0;
            char buffer[128];
            serializeJson(doc, buffer);
            mqttClient.publish(TOPIC_STATUS, buffer);
        }
    }
    
    delay(10); 
}

// ==================== LÓGICA DO TECLADO (ATM) ====================

void handleKey(char key) {
    Serial.print("Tecla: ");
    Serial.println(key);

    // Se for número (0-9)
    if (key >= '0' && key <= '9') {
        // Limite de segurança (para não estourar a variável long)
        if (currentCents < 10000000) { 
            // Desloca para a esquerda e adiciona o novo dígito
            // Ex: Tem 12 (0.12). Digita 5. Fica 125 (1.25)
            currentCents = (currentCents * 10) + (key - '0');
            sendValueUpdate();
        }
    }
    // Ações Especiais
    else if (key == 'A') { 
        // BACKSPACE (Apaga último dígito)
        currentCents = currentCents / 10;
        sendValueUpdate();
    }
    else if (key == 'C') {
        // CLEAR (Limpa tudo)
        currentCents = 0;
        Serial.println("[RESET] Valor zerado");
        sendValueUpdate();
    }
    else if (key == 'D') {
        // CONFIRMAR (Enter)
        if (currentCents > 0 && !pixRequested && mqttClient.connected()) {
            float finalValue = currentCents / 100.0;
            Serial.printf("[PIX] Solicitando R$ %.2f...\n", finalValue);
            
            JsonDocument doc;
            doc["amount"] = finalValue;
            doc["req_id"] = String(millis());
            
            char buffer[256];
            serializeJson(doc, buffer);
            mqttClient.publish(TOPIC_PIX_REQUEST, buffer);
            
            pixRequested = true;
        }
    }
}

// ==================== FUNÇÕES AUXILIARES ====================

void sendValueUpdate() {
    float displayValue = currentCents / 100.0;
    
    Serial.printf("[DISPLAY] R$ %.2f\n", displayValue);

    if (!mqttClient.connected()) return;
    
    JsonDocument doc;
    doc["value"] = displayValue;
    
    char buffer[64];
    serializeJson(doc, buffer);
    mqttClient.publish(TOPIC_VALUE_UPDATE, buffer);
}

void setupWiFi() {
    Serial.print("WiFi: ");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" OK");
}

void setupMQTT() {
    wifiClient.setInsecure();
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(512);
}

void connectMQTT() {
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
        Serial.println("[MQTT] Conectado");
        mqttClient.subscribe(TOPIC_PIX_RESPONSE);
        sendValueUpdate();
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) msg += (char)payload[i];

    if (String(topic) == TOPIC_PIX_RESPONSE) {
        JsonDocument doc;
        deserializeJson(doc, msg);
        
        bool success = doc["success"];
        pixRequested = false;
        
        if (success) {
            Serial.println("[PIX] SUCESSO - Limpando valor");
            currentCents = 0; // Opcional: zera o valor após gerar com sucesso
            sendValueUpdate();
        } else {
            Serial.println("[PIX] FALHA");
        }
    }
}