/**
 * ArdPix - Sistema de Pagamentos IoT com ESP32
 *
 * Este firmware implementa:
 * - Conexão WiFi
 * - Cliente MQTT para comunicação com dashboard
 * - Fila de processamento de pagamentos usando FreeRTOS
 * - LED indicador de status
 * - Buzzer para notificação de pagamentos
 *
 * Autor: Projeto Sistemas Embarcados - Cesar School
 * Data: 2024
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ===================== CONFIGURAÇÕES =====================

// WiFi
const char* WIFI_SSID = "iPhone de Cláudio";
const char* WIFI_PASSWORD = "cc123456";

// MQTT Broker (HiveMQ Cloud)
const char* MQTT_BROKER = "e804c2fdeb734bf2bbd47edfe90fc9b1.s1.eu.hivemq.cloud";
const int MQTT_PORT = 8883;  // Porta TLS
const char* MQTT_CLIENT_ID = "ardpix-esp32";
const char* MQTT_USERNAME = "admin";  // Substitua pelo seu usuário HiveMQ
const char* MQTT_PASSWORD = "qD01%59u#VtzrK";    // Substitua pela sua senha HiveMQ

// Tópicos MQTT
const char* TOPIC_PAYMENT = "ardpix/payment";
const char* TOPIC_STATUS = "ardpix/status";
const char* TOPIC_QUEUE = "ardpix/queue";
const char* TOPIC_QUEUE_ACK = "ardpix/queue/ack";

// Pinos GPIO
const int LED_PIN = 2;           // LED interno do ESP32
const int BUZZER_PIN = 4;        // Buzzer para notificação sonora
const int LED_STATUS_PIN = 5;    // LED externo de status (verde)
const int LED_ERROR_PIN = 18;    // LED de erro (vermelho)

// ===================== ESTRUTURAS DE DADOS =====================

// Estrutura para item da fila de pagamentos
struct PaymentQueueItem {
    char paymentId[50];
    float amount;
    char timestamp[30];
    char type[20];
};

// ===================== VARIÁVEIS GLOBAIS =====================

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

// Fila FreeRTOS para processar pagamentos
QueueHandle_t paymentQueue;
const int QUEUE_SIZE = 10;

// Tarefas FreeRTOS
TaskHandle_t mqttTaskHandle;
TaskHandle_t paymentProcessorTaskHandle;
TaskHandle_t statusReportTaskHandle;

// Controle de status
unsigned long lastReconnectAttempt = 0;
int queueItemsProcessed = 0;
bool isProcessing = false;

// ===================== DECLARAÇÕES DE FUNÇÕES =====================

void setupWiFi();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void handlePaymentMessage(char* jsonMessage);
bool mqttReconnect();
void mqttTask(void* parameter);
void paymentProcessorTask(void* parameter);
bool processPayment(PaymentQueueItem* item);
void sendPaymentAck(const char* paymentId, bool success);
void statusReportTask(void* parameter);
void beep(int times);

// ===================== SETUP =====================

void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=== ArdPix ESP32 Iniciando ===");

    // Configurar pinos
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_STATUS_PIN, OUTPUT);
    pinMode(LED_ERROR_PIN, OUTPUT);

    // Teste inicial dos LEDs
    digitalWrite(LED_STATUS_PIN, HIGH);
    digitalWrite(LED_ERROR_PIN, HIGH);
    delay(500);
    digitalWrite(LED_STATUS_PIN, LOW);
    digitalWrite(LED_ERROR_PIN, LOW);

    // Criar fila FreeRTOS
    paymentQueue = xQueueCreate(QUEUE_SIZE, sizeof(PaymentQueueItem));
    if (paymentQueue == NULL) {
        Serial.println("ERRO: Falha ao criar fila!");
        digitalWrite(LED_ERROR_PIN, HIGH);
        while(1);  // Parar execução
    }
    Serial.println("Fila criada com sucesso!");

    // Conectar WiFi
    setupWiFi();

    // Configurar cliente WiFi seguro (aceitar todos os certificados para HiveMQ)
    wifiClient.setInsecure();  // Para produção, use certificados apropriados

    // Configurar MQTT
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    // Criar tarefas FreeRTOS
    xTaskCreatePinnedToCore(
        mqttTask,
        "MQTT Task",
        4096,
        NULL,
        2,  // Prioridade alta
        &mqttTaskHandle,
        1   // Core 1
    );

    xTaskCreatePinnedToCore(
        paymentProcessorTask,
        "Payment Processor",
        4096,
        NULL,
        1,  // Prioridade média
        &paymentProcessorTaskHandle,
        0   // Core 0
    );

    xTaskCreatePinnedToCore(
        statusReportTask,
        "Status Report",
        2048,
        NULL,
        1,  // Prioridade média
        &statusReportTaskHandle,
        1   // Core 1
    );

    Serial.println("=== Sistema Iniciado ===\n");
}

void loop() {
    // Loop principal vazio - tudo é gerenciado por tarefas FreeRTOS
    vTaskDelay(pdMS_TO_TICKS(1000));
}

// ===================== WIFI =====================

void setupWiFi() {
    Serial.print("Conectando ao WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi conectado!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        digitalWrite(LED_STATUS_PIN, HIGH);

        // Sinal sonoro de sucesso
        beep(2);
    } else {
        Serial.println("\nFalha ao conectar WiFi!");
        digitalWrite(LED_ERROR_PIN, HIGH);
    }
}

// ===================== MQTT =====================

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Mensagem recebida [");
    Serial.print(topic);
    Serial.print("]: ");

    // Converter payload para string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    Serial.println(message);

    // Processar mensagem de pagamento
    if (strcmp(topic, TOPIC_PAYMENT) == 0) {
        handlePaymentMessage(message);
    }
}

void handlePaymentMessage(char* jsonMessage) {
    // Parse JSON - ArduinoJson 7.x usa JsonDocument
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonMessage);

    if (error) {
        Serial.print("Erro ao fazer parse JSON: ");
        Serial.println(error.c_str());
        return;
    }

    // Extrair dados
    PaymentQueueItem item;
    strlcpy(item.paymentId, doc["payment_id"] | "unknown", sizeof(item.paymentId));
    item.amount = doc["amount"] | 0.0;
    strlcpy(item.timestamp, doc["timestamp"] | "", sizeof(item.timestamp));
    strlcpy(item.type, doc["type"] | "payment", sizeof(item.type));

    // Adicionar à fila
    if (xQueueSend(paymentQueue, &item, pdMS_TO_TICKS(100)) == pdTRUE) {
        Serial.print("Pagamento adicionado à fila: R$ ");
        Serial.println(item.amount, 2);

        // Notificação visual
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
    } else {
        Serial.println("ERRO: Fila cheia! Pagamento rejeitado.");
        digitalWrite(LED_ERROR_PIN, HIGH);
        delay(1000);
        digitalWrite(LED_ERROR_PIN, LOW);
    }
}

bool mqttReconnect() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi desconectado, reconectando...");
        WiFi.reconnect();
        return false;
    }

    Serial.print("Conectando ao MQTT broker...");

    // Conectar com autenticação
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
        Serial.println("conectado!");

        // Subscrever tópicos
        mqttClient.subscribe(TOPIC_PAYMENT);

        Serial.print("Subscrito em: ");
        Serial.println(TOPIC_PAYMENT);

        // Sinal sonoro de conexão
        beep(1);

        return true;
    } else {
        Serial.print("falhou, rc=");
        Serial.println(mqttClient.state());
        return false;
    }
}

// ===================== TAREFAS FREERTOS =====================

void mqttTask(void* parameter) {
    const TickType_t xDelay = pdMS_TO_TICKS(100);

    for (;;) {
        if (!mqttClient.connected()) {
            unsigned long now = millis();
            if (now - lastReconnectAttempt > 5000) {
                lastReconnectAttempt = now;
                if (mqttReconnect()) {
                    lastReconnectAttempt = 0;
                }
            }
        } else {
            mqttClient.loop();
        }

        vTaskDelay(xDelay);
    }
}

void paymentProcessorTask(void* parameter) {
    PaymentQueueItem item;

    for (;;) {
        // Aguardar item na fila (bloqueante)
        if (xQueueReceive(paymentQueue, &item, portMAX_DELAY) == pdTRUE) {
            isProcessing = true;

            Serial.println("\n=== Processando Pagamento ===");
            Serial.print("ID: ");
            Serial.println(item.paymentId);
            Serial.print("Valor: R$ ");
            Serial.println(item.amount, 2);
            Serial.print("Timestamp: ");
            Serial.println(item.timestamp);

            // Simular processamento do pagamento
            bool success = processPayment(&item);

            // Enviar confirmação via MQTT
            sendPaymentAck(item.paymentId, success);

            queueItemsProcessed++;
            isProcessing = false;

            Serial.println("=== Processamento Concluído ===\n");
        }
    }
}

bool processPayment(PaymentQueueItem* item) {
    // Aqui você pode implementar a lógica real de processamento
    // Por exemplo: liberar produto, acionar relé, etc.

    Serial.println("Iniciando processamento...");

    // LED indicando processamento
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_STATUS_PIN, HIGH);
        delay(200);
        digitalWrite(LED_STATUS_PIN, LOW);
        delay(200);
    }

    // Notificação sonora proporcional ao valor
    int beeps = 1;
    if (item->amount >= 10.0) beeps = 2;
    if (item->amount >= 50.0) beeps = 3;
    if (item->amount >= 100.0) beeps = 4;

    beep(beeps);

    // Simular tempo de processamento
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Sucesso (99% das vezes)
    bool success = (random(100) < 99);

    if (success) {
        Serial.println("Pagamento processado com SUCESSO!");
        digitalWrite(LED_STATUS_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(2000));
        digitalWrite(LED_STATUS_PIN, LOW);
    } else {
        Serial.println("FALHA no processamento!");
        digitalWrite(LED_ERROR_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(2000));
        digitalWrite(LED_ERROR_PIN, LOW);
    }

    return success;
}

void sendPaymentAck(const char* paymentId, bool success) {
    // ArduinoJson 7.x usa JsonDocument
    JsonDocument doc;
    doc["payment_id"] = paymentId;
    doc["success"] = success;
    doc["timestamp"] = millis();

    char buffer[256];
    serializeJson(doc, buffer);

    if (mqttClient.connected()) {
        mqttClient.publish(TOPIC_QUEUE_ACK, buffer);
        Serial.print("ACK enviado: ");
        Serial.println(buffer);
    }
}

void statusReportTask(void* parameter) {
    const TickType_t xDelay = pdMS_TO_TICKS(30000);  // Enviar status a cada 30 segundos (otimizado)

    for (;;) {
        if (mqttClient.connected()) {
            // Obter tamanho atual da fila
            UBaseType_t queueSize = uxQueueMessagesWaiting(paymentQueue);

            // ArduinoJson 7.x usa JsonDocument
            JsonDocument doc;
            doc["connected"] = true;
            doc["queue_size"] = queueSize;
            doc["processed"] = queueItemsProcessed;
            doc["processing"] = isProcessing;
            doc["free_heap"] = ESP.getFreeHeap();
            doc["uptime"] = millis() / 1000;

            char buffer[256];
            serializeJson(doc, buffer);

            mqttClient.publish(TOPIC_STATUS, buffer);

            Serial.print("Status enviado - Fila: ");
            Serial.print(queueSize);
            Serial.print(" | Processados: ");
            Serial.println(queueItemsProcessed);
        }

        vTaskDelay(xDelay);
    }
}

// ===================== FUNÇÕES AUXILIARES =====================

void beep(int times) {
    for (int i = 0; i < times; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
    }
}
