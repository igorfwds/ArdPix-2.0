# ArdPix - Sistema de Pagamentos IoT

**Projeto de Sistemas Embarcados - Cesar School**

Sistema IoT que integra ESP32 com plataforma de pagamentos AbacatePay, utilizando MQTT, Flask e FreeRTOS.

## Visão Geral

O ArdPix é um sistema de pagamentos IoT que permite:
- Receber notificações de pagamentos em tempo real via webhook do AbacatePay
- Processar pagamentos através de uma fila gerenciada no ESP32 usando FreeRTOS
- Visualizar transações e saldo em dashboard web em tempo real
- Comunicação bidirecional entre servidor Flask e ESP32 via MQTT

## Arquitetura do Sistema

```
┌─────────────────┐         ┌─────────────────┐         ┌─────────────────┐
│   AbacatePay    │─Webhook─│  Flask Server   │──MQTT──│     ESP32       │
│  (Pagamentos)   │         │   + Dashboard   │         │  (FreeRTOS)     │
└─────────────────┘         └─────────────────┘         └─────────────────┘
                                     │                           │
                                     │                           │
                                 WebSocket                  Fila RTOS
                                     │                           │
                                     ▼                           ▼
                            ┌─────────────────┐         ┌─────────────────┐
                            │  Usuário Web    │         │  LEDs/Buzzer    │
                            │  (Dashboard)    │         │  (Indicadores)  │
                            └─────────────────┘         └─────────────────┘
```

## Estrutura do Repositório

```
ardpix2/
├── README.md                    # Este arquivo
├── flask-dashboard/             # Aplicação web Flask
│   ├── app.py                   # Servidor principal
│   ├── templates/               # Templates HTML
│   │   └── index.html
│   ├── static/                  # Arquivos estáticos
│   │   ├── css/
│   │   │   └── style.css
│   │   └── js/
│   │       └── dashboard.js
│   ├── requirements.txt         # Dependências Python
│   └── .env.example             # Variáveis de ambiente
├── esp32-firmware/              # Firmware ESP32
│   └── ardpix_esp32.ino
├── docs/                        # Documentação
│   └── Projeto-Embarcados_CesarSchool.pdf
└── schematics/                  # Diagramas eletrônicos
```

## Requisitos

### Hardware

- 1x ESP32 (NodeMCU ou similar)
- 1x Buzzer ativo 5V
- 2x LEDs (verde e vermelho)
- 2x Resistores 220Ω
- Jumpers e protoboard
- Cabo USB para programação

### Software

- Python 3.8+
- Arduino IDE ou PlatformIO
- Broker MQTT (Mosquitto recomendado)
- Conta no AbacatePay (https://abacatepay.com)

## Instalação

### 1. Configurar Broker MQTT

#### No Linux/Raspberry Pi:

```bash
sudo apt update
sudo apt install mosquitto mosquitto-clients
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
```

#### No macOS:

```bash
brew install mosquitto
brew services start mosquitto
```

### 2. Configurar Flask Dashboard

```bash
cd flask-dashboard

# Criar ambiente virtual
python3 -m venv venv
source venv/bin/activate  # No Windows: venv\Scripts\activate

# Instalar dependências
pip install -r requirements.txt

# Configurar variáveis de ambiente
cp .env.example .env
nano .env  # Editar com suas configurações
```

### 3. Configurar ESP32

1. Abra o arquivo `esp32-firmware/ardpix_esp32.ino` no Arduino IDE
2. Instale as bibliotecas necessárias:
   - PubSubClient
   - ArduinoJson
3. Edite as configurações no início do arquivo:
   ```cpp
   const char* WIFI_SSID = "SUA_REDE_WIFI";
   const char* WIFI_PASSWORD = "SUA_SENHA_WIFI";
   const char* MQTT_BROKER = "IP_DO_SERVIDOR";  // IP onde o Flask está rodando
   ```
4. Conecte o ESP32 via USB e faça o upload do código

### 4. Configurar Webhook no AbacatePay

1. Acesse o dashboard do AbacatePay
2. Vá em **Webhooks** > **Criar**
3. Configure:
   - **Nome**: ArdPix Webhook
   - **URL**: `https://seu-dominio.com/webhook/abacatepay?webhookSecret=SEU_SECRET`
   - **Secret**: Escolha um secret seguro (use o mesmo no arquivo `.env`)

**Importante**: Para desenvolvimento local, use um serviço como [ngrok](https://ngrok.com/) para expor seu servidor:

```bash
ngrok http 5000
# Use a URL gerada no webhook do AbacatePay
```

## Configuração de Variáveis de Ambiente

Crie o arquivo `.env` na pasta `flask-dashboard/`:

```bash
# Flask
FLASK_SECRET_KEY=sua-chave-secreta-muito-segura

# AbacatePay
WEBHOOK_SECRET=seu-webhook-secret

# MQTT
MQTT_BROKER=localhost  # ou IP do broker
MQTT_PORT=1883
```

## Uso

### Iniciar o Sistema

1. **Iniciar Broker MQTT** (se não estiver rodando):
   ```bash
   mosquitto
   ```

2. **Iniciar Flask Dashboard**:
   ```bash
   cd flask-dashboard
   source venv/bin/activate
   python app.py
   ```

   Acesse: http://localhost:5000

3. **Ligar ESP32**:
   - Conecte à alimentação
   - Aguarde conexão WiFi e MQTT (LEDs indicarão status)

### Testar Pagamento

1. Acesse o dashboard do AbacatePay
2. Crie uma cobrança PIX de teste (modo dev)
3. Simule o pagamento
4. Observe:
   - Dashboard web atualizar em tempo real
   - ESP32 processar o pagamento (LEDs e buzzer)
   - Confirmação retornar ao servidor

## Funcionalidades Implementadas

### Dashboard Flask

- ✅ Recebimento de webhooks do AbacatePay
- ✅ Validação de assinatura HMAC
- ✅ Verificação de secret
- ✅ Publicação de pagamentos via MQTT
- ✅ Interface web em tempo real (WebSocket)
- ✅ Visualização de saldo e histórico
- ✅ Gráficos de transações
- ✅ Notificações em tempo real

### ESP32 Firmware

- ✅ Conexão WiFi automática
- ✅ Cliente MQTT com reconexão automática
- ✅ Fila de pagamentos usando FreeRTOS (QueueHandle)
- ✅ 3 Tarefas FreeRTOS:
  - Task MQTT (Core 1)
  - Task Processador de Pagamentos (Core 0)
  - Task Relatório de Status (Core 1)
- ✅ Indicadores visuais (LEDs)
- ✅ Notificação sonora (Buzzer)
- ✅ Envio de confirmações (ACK)
- ✅ Relatório de status periódico

## Esquema de Ligação do ESP32

```
ESP32 NodeMCU
┌─────────────┐
│             │
│ GPIO 2  ────┼──── LED interno (built-in)
│             │
│ GPIO 4  ────┼──── Buzzer (+) ──┬── Buzzer
│             │                   └── GND
│ GPIO 5  ────┼──── LED Verde (+) ──┬── Resistor 220Ω ──┬── LED Verde
│             │                                          └── GND
│ GPIO 18 ────┼──── LED Vermelho (+) ──┬── Resistor 220Ω ──┬── LED Vermelho
│             │                                             └── GND
│ 3.3V    ────┼──── (não usado neste projeto)
│ GND     ────┼──── GND comum
└─────────────┘
```

### Legenda de LEDs

- **LED GPIO 2 (interno)**: Pisca ao receber mensagem MQTT
- **LED Verde (GPIO 5)**: Status conectado e processamento bem-sucedido
- **LED Vermelho (GPIO 18)**: Erros e falhas

### Buzzer

- 1 beep: Pagamento < R$ 10,00
- 2 beeps: Pagamento R$ 10,00 - R$ 49,99
- 3 beeps: Pagamento R$ 50,00 - R$ 99,99
- 4 beeps: Pagamento ≥ R$ 100,00

## Tópicos MQTT

| Tópico | Direção | Descrição |
|--------|---------|-----------|
| `ardpix/payment` | Flask → ESP32 | Dados de novo pagamento |
| `ardpix/status` | ESP32 → Flask | Status do dispositivo |
| `ardpix/queue/ack` | ESP32 → Flask | Confirmação de processamento |

## API REST

### GET /
- **Descrição**: Dashboard principal
- **Retorno**: Página HTML

### POST /webhook/abacatepay?webhookSecret=SECRET
- **Descrição**: Recebe eventos do AbacatePay
- **Headers**: `X-Webhook-Signature` (opcional)
- **Body**: JSON do evento
- **Eventos suportados**:
  - `billing.paid`: Pagamento confirmado
  - `withdraw.done`: Saque concluído
  - `withdraw.failed`: Saque falhou

### GET /api/balance
- **Descrição**: Retorna saldo atual
- **Retorno**: `{"total": 0.0, "last_update": "..."}`

### GET /api/payments
- **Descrição**: Retorna histórico de pagamentos
- **Retorno**: Array de transações

### GET /api/esp32/status
- **Descrição**: Status do ESP32
- **Retorno**: `{"connected": true, "last_seen": "...", "queue_size": 0}`

## WebSocket Events

### Emitidos pelo servidor:

- `balance_update`: Atualização de saldo
- `new_payment`: Novo pagamento recebido
- `esp32_status`: Status do ESP32
- `payment_processed`: Confirmação de processamento
- `withdraw_update`: Atualização de saque

## Desenvolvimento

### Estrutura do Código Flask

```python
app.py
├── Configuração (MQTT, WebSocket, variáveis)
├── Callbacks MQTT
├── Rotas Flask
│   ├── Dashboard (/)
│   ├── Webhook (/webhook/abacatepay)
│   └── APIs (/api/*)
├── Handlers de eventos
│   ├── handle_payment_paid()
│   ├── handle_withdraw_done()
│   └── handle_withdraw_failed()
└── WebSocket handlers
```

### Estrutura do Código ESP32

```cpp
ardpix_esp32.ino
├── Configurações e constantes
├── Setup
│   ├── Configuração de pinos
│   ├── Criação de fila FreeRTOS
│   ├── Conexão WiFi
│   └── Criação de tasks
├── Tasks FreeRTOS
│   ├── mqttTask: Gerencia conexão MQTT
│   ├── paymentProcessorTask: Processa fila de pagamentos
│   └── statusReportTask: Envia status periódico
├── Funções MQTT
│   ├── mqttCallback: Recebe mensagens
│   └── mqttReconnect: Reconecta ao broker
└── Funções auxiliares
    ├── processPayment: Lógica de processamento
    └── beep: Notificação sonora
```

## Testes

### Testar Webhook Localmente

```bash
curl -X POST http://localhost:5000/webhook/abacatepay?webhookSecret=SEU_SECRET \
  -H "Content-Type: application/json" \
  -d '{
    "event": "billing.paid",
    "data": {
      "payment": {
        "amount": 1000,
        "fee": 80,
        "method": "PIX"
      },
      "pixQrCode": {
        "id": "test_payment_123",
        "status": "PAID",
        "amount": 1000
      }
    },
    "devMode": true
  }'
```

### Testar MQTT

```bash
# Publicar mensagem de teste
mosquitto_pub -h localhost -t "ardpix/payment" -m '{"payment_id":"test_123","amount":10.50,"timestamp":"2024-12-01T10:00:00","type":"payment"}'

# Escutar mensagens de status
mosquitto_sub -h localhost -t "ardpix/status"
```

## Troubleshooting

### ESP32 não conecta ao WiFi

- Verifique SSID e senha
- Certifique-se que a rede é 2.4GHz (ESP32 não suporta 5GHz)
- Verifique força do sinal WiFi

### ESP32 não conecta ao MQTT

- Verifique IP do broker MQTT
- Teste conectividade: `ping IP_DO_BROKER`
- Verifique se o broker está rodando: `sudo systemctl status mosquitto`

### Dashboard não atualiza

- Verifique console do navegador (F12)
- Verifique se WebSocket está conectado
- Reinicie o servidor Flask

### Webhook não recebe eventos

- Verifique URL do webhook no AbacatePay
- Use ngrok para desenvolvimento local
- Verifique logs do Flask

## Melhorias Futuras

- [ ] Persistência de dados em banco (SQLite/PostgreSQL)
- [ ] Autenticação de usuários
- [ ] Múltiplos ESP32 conectados
- [ ] Integração com display LCD/OLED no ESP32
- [ ] Modo offline com sincronização posterior
- [ ] Alertas por email/SMS
- [ ] API para integração com outros sistemas
- [ ] Suporte IPv6
- [ ] Logs estruturados
- [ ] Testes automatizados

## Requisitos do Projeto Atendidos

- ✅ ESP32 como dispositivo sensor/atuador
- ✅ Comunicação WiFi
- ✅ Protocolo MQTT para troca de dados
- ✅ Dashboard web em tempo real
- ✅ FreeRTOS com múltiplas tasks
- ✅ Fila de processamento (QueueHandle)
- ✅ Biblioteca MQTT
- ✅ Aplicação Web com Python + Flask
- ✅ WebSocket para tempo real
- ✅ Versionamento com Git/GitHub

## Autores

Projeto desenvolvido para a disciplina de Sistemas Embarcados - Cesar School

## Licença

MIT License - Livre para uso educacional

## Contato

Para dúvidas sobre o projeto, consulte os professores:
- Bella Nunes
- Jymmy Barreto

---

**ArdPix** - Connecting Payments to IoT 🚀
