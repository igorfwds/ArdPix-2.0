# Guia de Instalação e Configuração - ArdPix

Este guia detalha o passo a passo completo para configurar o projeto ArdPix.

## Índice

1. [Configuração do Ambiente](#1-configuração-do-ambiente)
2. [Instalação do Broker MQTT](#2-instalação-do-broker-mqtt)
3. [Configuração do Flask](#3-configuração-do-flask)
4. [Programação do ESP32](#4-programação-do-esp32)
5. [Configuração do AbacatePay](#5-configuração-do-abacatepay)
6. [Testes](#6-testes)

## 1. Configuração do Ambiente

### Requisitos do Sistema

- Python 3.8 ou superior
- Arduino IDE 1.8.x ou 2.x
- Git
- Conta no AbacatePay

### Clonar o Repositório

```bash
git clone <seu-repositorio>
cd ardpix2
```

## 2. Instalação do Broker MQTT

### Ubuntu/Debian/Raspberry Pi

```bash
# Atualizar repositórios
sudo apt update

# Instalar Mosquitto
sudo apt install -y mosquitto mosquitto-clients

# Habilitar para iniciar automaticamente
sudo systemctl enable mosquitto

# Iniciar serviço
sudo systemctl start mosquitto

# Verificar status
sudo systemctl status mosquitto
```

### macOS

```bash
# Instalar via Homebrew
brew install mosquitto

# Iniciar serviço
brew services start mosquitto

# Verificar se está rodando
brew services list | grep mosquitto
```

### Windows

1. Baixar Mosquitto em: https://mosquitto.org/download/
2. Executar instalador
3. Adicionar ao PATH do sistema
4. Iniciar serviço:
   ```
   net start mosquitto
   ```

### Testar Instalação MQTT

Em um terminal:
```bash
# Subscrever a um tópico de teste
mosquitto_sub -h localhost -t "test/topic"
```

Em outro terminal:
```bash
# Publicar mensagem
mosquitto_pub -h localhost -t "test/topic" -m "Hello MQTT!"
```

Você deve ver a mensagem aparecer no primeiro terminal.

## 3. Configuração do Flask

### 3.1. Criar Ambiente Virtual

```bash
cd flask-dashboard

# Criar ambiente virtual
python3 -m venv venv

# Ativar (Linux/macOS)
source venv/bin/activate

# Ativar (Windows)
venv\Scripts\activate
```

### 3.2. Instalar Dependências

```bash
pip install -r requirements.txt
```

### 3.3. Configurar Variáveis de Ambiente

```bash
# Copiar arquivo exemplo
cp .env.example .env

# Editar arquivo
nano .env  # ou use seu editor preferido
```

Edite o arquivo `.env`:

```bash
FLASK_SECRET_KEY=minha-chave-super-secreta-123456
WEBHOOK_SECRET=meu-webhook-secret-abacate
MQTT_BROKER=localhost
MQTT_PORT=1883
```

**IMPORTANTE**: Escolha valores únicos e seguros para produção!

### 3.4. Testar Flask

```bash
python app.py
```

Acesse: http://localhost:5000

Você deve ver o dashboard (ainda sem dados).

## 4. Programação do ESP32

### 4.1. Instalar Arduino IDE

Baixe em: https://www.arduino.cc/en/software

### 4.2. Adicionar Suporte ao ESP32

1. Abra Arduino IDE
2. Vá em **File** > **Preferences**
3. Em **Additional Board Manager URLs**, adicione:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Vá em **Tools** > **Board** > **Boards Manager**
5. Procure "esp32" e instale "esp32 by Espressif Systems"

### 4.3. Instalar Bibliotecas

1. Vá em **Sketch** > **Include Library** > **Manage Libraries**
2. Instale as seguintes bibliotecas:
   - **PubSubClient** (by Nick O'Leary)
   - **ArduinoJson** (by Benoit Blanchon) - versão 6.x

### 4.4. Configurar o Código

1. Abra `esp32-firmware/ardpix_esp32.ino`
2. Edite as linhas 18-24:

```cpp
// WiFi - EDITE AQUI
const char* WIFI_SSID = "Nome_da_Sua_Rede";
const char* WIFI_PASSWORD = "Senha_do_WiFi";

// MQTT Broker - EDITE AQUI
const char* MQTT_BROKER = "192.168.1.100";  // IP do computador rodando Flask
const int MQTT_PORT = 1883;
```

**Como descobrir o IP do computador:**

Linux/macOS:
```bash
ifconfig | grep "inet "
```

Windows:
```
ipconfig
```

### 4.5. Fazer Upload para o ESP32

1. Conecte o ESP32 via USB
2. Selecione a placa:
   - **Tools** > **Board** > **ESP32 Arduino** > **ESP32 Dev Module**
3. Selecione a porta:
   - **Tools** > **Port** > (selecione a porta COM/ttyUSB do ESP32)
4. Clique em **Upload** (seta →)

### 4.6. Monitorar Serial

1. Abra o Serial Monitor: **Tools** > **Serial Monitor**
2. Configure para **115200 baud**
3. Você deve ver:
   ```
   === ArdPix ESP32 Iniciando ===
   Conectando ao WiFi: Nome_da_Sua_Rede
   ...
   WiFi conectado!
   IP: 192.168.x.x
   Conectando ao MQTT broker...conectado!
   === Sistema Iniciado ===
   ```

## 5. Configuração do AbacatePay

### 5.1. Criar Conta

1. Acesse: https://abacatepay.com
2. Crie uma conta
3. Faça login no dashboard

### 5.2. Modo Desenvolvedor

1. No dashboard, ative o **modo desenvolvedor** (dev mode)
2. Isso permite testar sem pagamentos reais

### 5.3. Expor Servidor Local (ngrok)

Para desenvolvimento local, use ngrok para criar um túnel público:

```bash
# Instalar ngrok
# macOS
brew install ngrok

# Linux
wget https://bin.equinox.io/c/bNyj1mQVY4c/ngrok-v3-stable-linux-amd64.tgz
tar xvzf ngrok-v3-stable-linux-amd64.tgz
sudo mv ngrok /usr/local/bin/

# Criar túnel
ngrok http 5000
```

Você verá algo como:
```
Forwarding  https://abc123.ngrok.io -> http://localhost:5000
```

**Copie a URL HTTPS** (abc123.ngrok.io).

### 5.4. Criar Webhook

1. No dashboard do AbacatePay, vá em **Webhooks**
2. Clique em **Criar**
3. Preencha:
   - **Nome**: ArdPix Webhook
   - **URL**: `https://abc123.ngrok.io/webhook/abacatepay?webhookSecret=SEU_SECRET`
     - Substitua `abc123.ngrok.io` pela URL do ngrok
     - Substitua `SEU_SECRET` pelo valor que você colocou no `.env`
   - **Secret**: Use o mesmo valor do `.env`
4. Clique em **Salvar**

### 5.5. Testar Webhook

1. No dashboard do AbacatePay, vá em **Cobranças** > **Criar Cobrança**
2. Crie uma cobrança PIX de teste (ex: R$ 10,00)
3. **Simule o pagamento** (no modo dev)
4. Observe:
   - Dashboard Flask deve mostrar novo pagamento
   - ESP32 deve processar (LEDs e buzzer)
   - Console do ESP32 mostra processamento

## 6. Testes

### 6.1. Testar Webhook Manualmente

```bash
curl -X POST "http://localhost:5000/webhook/abacatepay?webhookSecret=SEU_SECRET" \
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

### 6.2. Testar MQTT Diretamente

Publicar pagamento manualmente:

```bash
mosquitto_pub -h localhost -t "ardpix/payment" -m '{
  "payment_id": "manual_test_456",
  "amount": 25.50,
  "timestamp": "2024-12-01T15:30:00",
  "type": "payment"
}'
```

Observe o ESP32 processar o pagamento.

### 6.3. Monitorar Tópicos MQTT

```bash
# Em um terminal
mosquitto_sub -h localhost -t "ardpix/#" -v
```

Isso mostra todas as mensagens nos tópicos do ArdPix.

## Troubleshooting

### Problema: ESP32 não conecta ao WiFi

**Soluções:**
- Verifique SSID e senha
- Certifique-se que é rede 2.4 GHz (ESP32 não suporta 5 GHz)
- Tente aproximar ESP32 do roteador
- Verifique no Serial Monitor o erro exato

### Problema: ESP32 conecta WiFi mas não conecta MQTT

**Soluções:**
- Verifique IP do broker MQTT
- Teste conexão: `ping IP_DO_BROKER` do computador na mesma rede
- Verifique firewall:
  ```bash
  # Linux
  sudo ufw allow 1883

  # macOS
  # Vá em Preferências > Segurança > Firewall > Opções > Permitir
  ```
- Verifique se Mosquitto está rodando: `sudo systemctl status mosquitto`

### Problema: Webhook não recebe eventos

**Soluções:**
- Verifique URL no AbacatePay
- Verifique se ngrok está rodando
- Teste webhook manualmente com curl
- Verifique logs do Flask

### Problema: Dashboard não atualiza em tempo real

**Soluções:**
- Abra console do navegador (F12)
- Verifique erros JavaScript
- Verifique se WebSocket está conectado (mensagem verde no canto)
- Reinicie servidor Flask

## Próximos Passos

Após configurar tudo:

1. **Teste o fluxo completo**:
   - Criar cobrança no AbacatePay
   - Simular pagamento
   - Observar dashboard atualizar
   - Observar ESP32 processar

2. **Personalize**:
   - Adicione sensores ao ESP32
   - Modifique lógica de processamento
   - Customize interface do dashboard

3. **Documente**:
   - Tire fotos/vídeos do funcionamento
   - Documente problemas e soluções
   - Prepare apresentação

## Recursos Adicionais

- [Documentação AbacatePay](https://docs.abacatepay.com)
- [Documentação ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [Tutorial FreeRTOS](https://www.freertos.org/Documentation/RTOS_book.html)
- [MQTT.org](https://mqtt.org/)

## Suporte

Para dúvidas, consulte:
- README.md principal
- Professores da disciplina
- Documentação oficial das ferramentas

---

Bom projeto! 🚀
