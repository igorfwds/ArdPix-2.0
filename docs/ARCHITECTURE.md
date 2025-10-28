# Arquitetura do Sistema ArdPix

## Visão Geral

O ArdPix é um sistema distribuído que integra três componentes principais:

1. **AbacatePay** - Plataforma de pagamentos PIX
2. **Flask Dashboard** - Servidor web com MQTT broker
3. **ESP32** - Dispositivo IoT embarcado com FreeRTOS

## Diagrama de Arquitetura

```
┌─────────────────────────────────────────────────────────────────┐
│                         Internet                                 │
└────────────┬────────────────────────────────────────┬────────────┘
             │                                        │
             │ HTTPS Webhook                          │
             │ (JSON)                                 │
             ▼                                        │
    ┌─────────────────┐                               │
    │   AbacatePay    │                               │
    │   (Pagamentos)  │                               │
    └─────────────────┘                               │
                                                       │
┌──────────────────────────────────────────────────────┼────────────┐
│                    Rede Local (WiFi)                 │            │
│                                                      │            │
│    ┌───────────────────────────────────────────┐    │            │
│    │          Servidor Flask                   │    │            │
│    │  ┌─────────────────────────────────────┐ │    │            │
│    │  │         app.py                      │ │    │            │
│    │  │  ┌──────────────┐  ┌──────────────┐ │ │    │            │
│    │  │  │   Webhook    │  │  MQTT Client │ │ │    │            │
│    │  │  │   Handler    │  │              │ │ │    │            │
│    │  │  └──────┬───────┘  └──────┬───────┘ │ │    │            │
│    │  │         │                 │         │ │    │            │
│    │  │         ▼                 ▼         │ │    │            │
│    │  │  ┌──────────────────────────────┐  │ │    │            │
│    │  │  │    Payment Queue Manager     │  │ │    │            │
│    │  │  └──────────────┬───────────────┘  │ │    │            │
│    │  │                 │                  │ │    │            │
│    │  │                 ▼                  │ │    │            │
│    │  │  ┌──────────────────────────────┐  │ │    │            │
│    │  │  │      WebSocket Server        │  │ │    │            │
│    │  │  │      (Socket.IO)             │  │ │    │            │
│    │  │  └──────────────┬───────────────┘  │ │    │            │
│    │  └─────────────────┼───────────────────┘ │    │            │
│    │                    │                     │    │            │
│    │         ┌──────────┼──────────┐          │    │            │
│    │         │          │          │          │    │            │
│    │         ▼          ▼          ▼          │    │            │
│    │    ┌────────┐ ┌────────┐ ┌────────┐     │    │            │
│    │    │Browser │ │Browser │ │Browser │     │    │            │
│    │    │Client 1│ │Client 2│ │Client n│     │    │            │
│    │    └────────┘ └────────┘ └────────┘     │    │            │
│    └───────────────────────────────────────────┘    │            │
│                                                      │            │
│    ┌───────────────────────────────────────────┐    │            │
│    │       Mosquitto MQTT Broker               │    │            │
│    │       (localhost:1883)                    │    │            │
│    └──────────────────┬────────────────────────┘    │            │
│                       │                             │            │
│                       │ MQTT Protocol               │            │
│                       │ (Topics: ardpix/*)          │            │
│                       │                             │            │
│                       ▼                             │            │
│              ┌─────────────────┐                    │            │
│              │     ESP32       │◄───────────────────┘            │
│              │   (FreeRTOS)    │  WiFi 2.4GHz                    │
│              │                 │                                 │
│              │  ┌───────────┐  │                                 │
│              │  │   Core 0  │  │                                 │
│              │  │           │  │                                 │
│              │  │ Payment   │  │                                 │
│              │  │ Processor │  │                                 │
│              │  │   Task    │  │                                 │
│              │  └─────┬─────┘  │                                 │
│              │        │        │                                 │
│              │        ▼        │                                 │
│              │  ┌───────────┐  │                                 │
│              │  │FreeRTOS   │  │                                 │
│              │  │Queue      │  │                                 │
│              │  │(10 items) │  │                                 │
│              │  └───────────┘  │                                 │
│              │                 │                                 │
│              │  ┌───────────┐  │                                 │
│              │  │   Core 1  │  │                                 │
│              │  │           │  │                                 │
│              │  │ MQTT Task │  │                                 │
│              │  │ Status    │  │                                 │
│              │  │   Task    │  │                                 │
│              │  └───────────┘  │                                 │
│              │                 │                                 │
│              │  ┌───────────┐  │                                 │
│              │  │  GPIOs    │  │                                 │
│              │  │  LEDs     │  │                                 │
│              │  │  Buzzer   │  │                                 │
│              │  └───────────┘  │                                 │
│              └─────────────────┘                                 │
└──────────────────────────────────────────────────────────────────┘
```

## Fluxo de Dados

### 1. Recebimento de Pagamento

```
┌──────────┐      HTTPS       ┌──────────┐      MQTT        ┌──────────┐
│AbacatePay├─────────────────►│  Flask   ├─────────────────►│  ESP32   │
└──────────┘   Webhook Event  └────┬─────┘  Payment Topic  └──────────┘
                                   │
                                   │ WebSocket
                                   ▼
                              ┌──────────┐
                              │ Browser  │
                              │Dashboard │
                              └──────────┘
```

**Passo a Passo:**

1. Cliente realiza pagamento PIX no AbacatePay
2. AbacatePay confirma pagamento
3. AbacatePay envia webhook para Flask: `POST /webhook/abacatepay`
4. Flask valida webhook (secret + HMAC)
5. Flask processa evento `billing.paid`
6. Flask armazena em memória (payment_history)
7. Flask atualiza saldo
8. Flask publica no MQTT topic `ardpix/payment`
9. Flask emite evento WebSocket `new_payment` para browsers
10. ESP32 recebe mensagem MQTT
11. ESP32 adiciona à fila FreeRTOS
12. Task de processamento consome da fila
13. ESP32 processa pagamento (LEDs, buzzer)
14. ESP32 envia ACK no topic `ardpix/queue/ack`
15. Flask recebe ACK e emite `payment_processed` via WebSocket

### 2. Status do ESP32

```
┌──────────┐     MQTT Status    ┌──────────┐    WebSocket    ┌──────────┐
│  ESP32   ├───────────────────►│  Flask   ├────────────────►│ Browser  │
└──────────┘   (a cada 5s)      └──────────┘   esp32_status  └──────────┘
```

**Periodicidade:** A cada 5 segundos

**Dados enviados:**
- Status de conexão
- Tamanho atual da fila
- Número de pagamentos processados
- Memória livre (heap)
- Tempo de atividade (uptime)

## Componentes Detalhados

### Flask Dashboard (app.py)

**Responsabilidades:**
- Receber webhooks do AbacatePay
- Validar segurança (secret + HMAC)
- Gerenciar estado da aplicação (saldo, histórico)
- Comunicar com ESP32 via MQTT
- Servir interface web
- Enviar atualizações em tempo real via WebSocket

**Estrutura de Dados:**

```python
# Histórico de pagamentos (últimos 100)
payment_history = deque(maxlen=100)

# Estrutura de um pagamento
{
    'id': 'pix_char_xxx',
    'amount': 10.50,
    'fee': 0.80,
    'method': 'PIX',
    'status': 'PAID',
    'timestamp': '2024-12-01T10:00:00',
    'dev_mode': False
}

# Saldo atual
balance = {
    'total': 150.75,
    'last_update': '2024-12-01T10:00:00'
}

# Status do ESP32
esp32_status = {
    'connected': True,
    'last_seen': '2024-12-01T10:00:05',
    'queue_size': 2
}
```

**Rotas HTTP:**

| Rota | Método | Descrição |
|------|--------|-----------|
| `/` | GET | Dashboard HTML |
| `/webhook/abacatepay` | POST | Recebe webhooks |
| `/api/balance` | GET | Retorna saldo |
| `/api/payments` | GET | Retorna histórico |
| `/api/esp32/status` | GET | Status do ESP32 |

**Eventos WebSocket:**

| Evento | Direção | Dados |
|--------|---------|-------|
| `connect` | Cliente→Servidor | - |
| `disconnect` | Cliente→Servidor | - |
| `balance_update` | Servidor→Cliente | {total, last_update} |
| `new_payment` | Servidor→Cliente | {id, amount, ...} |
| `esp32_status` | Servidor→Cliente | {connected, queue_size, ...} |
| `payment_processed` | Servidor→Cliente | {payment_id, success} |

**Tópicos MQTT (Client):**

| Tópico | Direção | QoS | Descrição |
|--------|---------|-----|-----------|
| `ardpix/payment` | Publish | 1 | Envia pagamento para ESP32 |
| `ardpix/status` | Subscribe | 0 | Recebe status do ESP32 |
| `ardpix/queue/ack` | Subscribe | 1 | Recebe confirmações |

### ESP32 Firmware (ardpix_esp32.ino)

**Responsabilidades:**
- Conectar WiFi e MQTT
- Receber mensagens de pagamento
- Gerenciar fila de processamento (FreeRTOS)
- Processar pagamentos (simular ou controlar hardware)
- Enviar confirmações e status
- Indicadores visuais e sonoros

**Arquitetura FreeRTOS:**

```
Core 0                          Core 1
┌─────────────────┐            ┌─────────────────┐
│                 │            │                 │
│  Payment        │            │  MQTT Task      │
│  Processor      │            │                 │
│  Task           │            │                 │
│                 │            │  Status Report  │
│  (Prioridade 1) │            │  Task           │
│                 │            │                 │
│                 │            │  (Prioridade 2) │
└────────┬────────┘            └────────┬────────┘
         │                              │
         │                              │
         └──────────┬───────────────────┘
                    │
                    ▼
            ┌───────────────┐
            │  Queue Handle │
            │  (10 items)   │
            └───────────────┘
```

**Tasks FreeRTOS:**

1. **mqttTask (Core 1, Prioridade 2)**
   - Gerencia conexão MQTT
   - Reconecta se necessário
   - Processa loop MQTT (client.loop())
   - Delay: 100ms

2. **paymentProcessorTask (Core 0, Prioridade 1)**
   - Aguarda itens na fila (blocking)
   - Processa pagamento
   - Controla LEDs e buzzer
   - Envia ACK via MQTT
   - Sem delay (bloqueante na fila)

3. **statusReportTask (Core 1, Prioridade 1)**
   - Coleta status do sistema
   - Publica no MQTT
   - Delay: 5000ms (5 segundos)

**Estrutura da Fila:**

```cpp
struct PaymentQueueItem {
    char paymentId[50];      // ID do pagamento
    float amount;            // Valor em reais
    char timestamp[30];      // ISO timestamp
    char type[20];           // Tipo: "payment"
};
```

**Tamanho:** 10 itens máximo

**Comportamento:**
- Se fila cheia: rejeita novo pagamento (LED vermelho)
- Se fila vazia: task aguarda (blocking)

**Pinos GPIO:**

| Pino | Função | Descrição |
|------|--------|-----------|
| GPIO 2 | LED interno | Pisca ao receber MQTT |
| GPIO 4 | Buzzer | Notificação sonora |
| GPIO 5 | LED verde | Status/Sucesso |
| GPIO 18 | LED vermelho | Erro/Falha |

**Tópicos MQTT (Client):**

| Tópico | Direção | QoS | Descrição |
|--------|---------|-----|-----------|
| `ardpix/payment` | Subscribe | 1 | Recebe pagamentos |
| `ardpix/status` | Publish | 0 | Envia status |
| `ardpix/queue/ack` | Publish | 1 | Envia confirmações |

### Mosquitto MQTT Broker

**Papel:**
- Intermediário de mensagens
- Gerencia pub/sub entre Flask e ESP32
- Roda localmente (localhost:1883)

**Tópicos:**

```
ardpix/
├── payment          # Flask → ESP32 (novos pagamentos)
├── status           # ESP32 → Flask (status periódico)
└── queue/
    └── ack          # ESP32 → Flask (confirmações)
```

## Protocolos de Comunicação

### 1. Webhook AbacatePay → Flask

**Protocolo:** HTTPS
**Formato:** JSON
**Autenticação:**
- Query param: `webhookSecret`
- Header: `X-Webhook-Signature` (HMAC-SHA256)

**Exemplo de Payload:**

```json
{
  "event": "billing.paid",
  "data": {
    "payment": {
      "amount": 1000,
      "fee": 80,
      "method": "PIX"
    },
    "pixQrCode": {
      "id": "pix_char_xxx",
      "status": "PAID",
      "amount": 1000
    }
  },
  "devMode": false
}
```

### 2. Flask ↔ ESP32 (MQTT)

**Protocolo:** MQTT v3.1.1
**Porta:** 1883 (não criptografada)
**Formato:** JSON

**Mensagem de Pagamento (Flask → ESP32):**

```json
{
  "type": "payment",
  "payment_id": "pix_char_xxx",
  "amount": 10.50,
  "timestamp": "2024-12-01T10:00:00"
}
```

**Mensagem de Status (ESP32 → Flask):**

```json
{
  "connected": true,
  "queue_size": 2,
  "processed": 45,
  "processing": false,
  "free_heap": 250000,
  "uptime": 3600
}
```

**Mensagem de ACK (ESP32 → Flask):**

```json
{
  "payment_id": "pix_char_xxx",
  "success": true,
  "timestamp": 12345678
}
```

### 3. Flask ↔ Browser (WebSocket)

**Protocolo:** WebSocket (Socket.IO)
**Porta:** 5000 (mesma porta HTTP)
**Formato:** JSON

**Exemplo de Eventos:**

```javascript
// Servidor → Cliente
socket.emit('new_payment', {
  id: 'pix_char_xxx',
  amount: 10.50,
  fee: 0.80,
  method: 'PIX',
  timestamp: '2024-12-01T10:00:00'
});

// Cliente → Servidor
socket.on('connect', () => {
  console.log('Conectado!');
});
```

## Segurança

### Webhook AbacatePay

1. **Validação de Secret (Query String)**
   - Verifica `webhookSecret` em cada requisição
   - Rejeita se não corresponder ao configurado

2. **Validação HMAC (Header)**
   - Calcula HMAC-SHA256 do corpo da requisição
   - Compara com `X-Webhook-Signature`
   - Usa chave pública do AbacatePay
   - Comparação timing-safe

### MQTT

**Aviso:** Configuração atual NÃO usa autenticação!

Para produção, configure:
```bash
# mosquitto.conf
allow_anonymous false
password_file /etc/mosquitto/passwd
```

E adicione credenciais ao código.

### WebSocket

- Mesma origem que servidor Flask
- CORS configurado para permitir conexões
- Sem autenticação atualmente

**Para produção:** Implemente autenticação JWT/sessão.

## Escalabilidade

### Limitações Atuais

1. **Armazenamento em Memória**
   - Histórico limitado a 100 transações
   - Dados perdidos ao reiniciar servidor

2. **Fila do ESP32**
   - Limitada a 10 itens
   - Pode rejeitar pagamentos em alta demanda

3. **Single ESP32**
   - Apenas um dispositivo suportado

### Melhorias Futuras

1. **Adicionar Banco de Dados**
   ```python
   # SQLite ou PostgreSQL
   from flask_sqlalchemy import SQLAlchemy
   ```

2. **Aumentar Tamanho da Fila**
   ```cpp
   const int QUEUE_SIZE = 50;
   ```

3. **Suporte a Múltiplos ESP32**
   ```python
   # Tópicos dinâmicos
   TOPIC_PATTERN = "ardpix/{device_id}/payment"
   ```

## Performance

### Latência Esperada

```
Pagamento PIX → Dashboard Web: ~500ms - 2s
  ├─ Webhook AbacatePay → Flask: 100-500ms
  ├─ Flask processing: 10-50ms
  └─ WebSocket → Browser: 10-50ms

Pagamento PIX → ESP32 Processing: ~1-3s
  ├─ Webhook → Flask: 100-500ms
  ├─ Flask → MQTT → ESP32: 50-200ms
  ├─ Queue wait: 0-1000ms (depende da fila)
  └─ ESP32 processing: 1000-2000ms
```

### Throughput

- **Flask:** ~100 req/s (limitado por Python/GIL)
- **MQTT:** ~1000 msg/s
- **ESP32 Queue:** ~1 pagamento/s (devido ao delay de processamento)

## Conclusão

O ArdPix demonstra uma arquitetura IoT completa com:
- Integração de serviços externos (AbacatePay)
- Comunicação assíncrona (MQTT)
- Processamento em tempo real (WebSocket)
- Sistema embarcado multitarefa (FreeRTOS)

A arquitetura é modular e pode ser estendida para diversos casos de uso IoT.
