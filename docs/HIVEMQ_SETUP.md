# Configuração do HiveMQ Cloud para ArdPix

Este guia explica como configurar o broker MQTT na nuvem usando HiveMQ Cloud.

## Por que usar HiveMQ Cloud?

- ✅ Broker MQTT na nuvem (não precisa de servidor local)
- ✅ Plano gratuito com até 100 conexões simultâneas
- ✅ Suporte a TLS/SSL seguro
- ✅ Interface web para monitoramento
- ✅ Alta disponibilidade e confiabilidade

## Passo a Passo

### 1. Criar Conta no HiveMQ Cloud

1. Acesse: https://www.hivemq.com/mqtt-cloud-broker/
2. Clique em "Get started for free"
3. Preencha os dados e crie sua conta
4. Confirme seu email

### 2. Criar um Cluster MQTT

1. Após login, clique em "Create Cluster"
2. Selecione o plano **Free** (gratuito)
3. Escolha a região mais próxima (ex: EU-West-1)
4. Dê um nome ao seu cluster (ex: "ardpix-cluster")
5. Clique em "Create"

### 3. Obter Credenciais

Após criar o cluster, você verá:

- **Host/URL**: `xxx.s1.eu.hivemq.cloud`
- **Port (TLS)**: `8883`
- **Port (WebSocket)**: `8884`

### 4. Criar Credenciais de Acesso

1. No dashboard do cluster, vá em "Access Management"
2. Clique em "Add credentials"
3. Escolha um username (ex: `ardpix-user`)
4. Escolha uma senha forte
5. Clique em "Create"

⚠️ **IMPORTANTE**: Anote as credenciais, você precisará delas!

### 5. Configurar o Projeto ArdPix

#### 5.1 Atualizar Flask Dashboard

Edite o arquivo `flask-dashboard/.env`:

```bash
# MQTT Broker Configuration (HiveMQ Cloud)
MQTT_BROKER=SEU-CLUSTER.s1.eu.hivemq.cloud
MQTT_PORT=8883
MQTT_USERNAME=seu-usuario-criado
MQTT_PASSWORD=sua-senha-criada
MQTT_USE_TLS=True
```

#### 5.2 Atualizar ESP32

Edite o arquivo `src/main.cpp`:

```cpp
// MQTT Broker (HiveMQ Cloud)
const char* MQTT_BROKER = "SEU-CLUSTER.s1.eu.hivemq.cloud";
const int MQTT_PORT = 8883;
const char* MQTT_CLIENT_ID = "ardpix-esp32";
const char* MQTT_USERNAME = "seu-usuario-criado";
const char* MQTT_PASSWORD = "sua-senha-criada";
```

### 6. Testar a Conexão

#### Testar Flask Dashboard:

```bash
cd flask-dashboard
source venv/bin/activate
python app.py
```

Você deve ver no log:
```
Autenticação MQTT configurada para usuário: seu-usuario
TLS/SSL habilitado para MQTT
Conectando ao broker: xxx.s1.eu.hivemq.cloud:8883
Cliente MQTT iniciado com sucesso!
```

#### Testar ESP32:

1. Compile e faça upload do firmware:
   ```bash
   pio run --target upload
   ```

2. Monitore a saída serial:
   ```bash
   pio device monitor
   ```

Você deve ver:
```
Conectando ao MQTT broker...conectado!
Subscrito em: ardpix/payment
```

### 7. Monitorar no HiveMQ Cloud

1. Acesse o dashboard do HiveMQ Cloud
2. Vá em "Web Client"
3. Conecte-se usando suas credenciais
4. Subscreva aos tópicos:
   - `ardpix/payment`
   - `ardpix/status`
   - `ardpix/queue/ack`

Agora você pode ver as mensagens em tempo real!

## Troubleshooting

### Erro: Connection refused

- Verifique se as credenciais estão corretas
- Confirme que a porta é 8883 (TLS)
- Verifique se o cluster está ativo no dashboard

### Erro: SSL Handshake failed

- ESP32: Certifique-se de ter `wifiClient.setInsecure();` no código
- Flask: Verifique se `MQTT_USE_TLS=True` está configurado

### ESP32 não conecta

- Verifique a conexão WiFi primeiro
- Confirme que as credenciais no código estão corretas
- Teste a conexão usando o Web Client do HiveMQ primeiro

## Limites do Plano Gratuito

- **100 conexões simultâneas**
- **10 GB de dados por mês**
- **Retenção de mensagens**: 1 dia
- **QoS 0, 1, 2 suportados**

Para a maioria dos projetos IoT, isso é mais que suficiente!

## Alternativas ao HiveMQ Cloud

Se precisar de outras opções:

1. **EMQX Cloud** - Plano gratuito com 1M msgs/mês
2. **CloudMQTT** - Gratuito até 10 conexões
3. **AWS IoT Core** - Pago, mas muito robusto
4. **Azure IoT Hub** - Pago
5. **Adafruit IO** - Gratuito com limites

## Segurança

Para produção, considere:

- ✅ Usar certificados TLS próprios em vez de `setInsecure()`
- ✅ Rotacionar credenciais regularmente
- ✅ Usar ACLs (Access Control Lists) no HiveMQ
- ✅ Limitar permissões por tópico
- ✅ Monitorar logs de acesso

## Próximos Passos

Agora que seu broker está na nuvem:

1. ✅ Seu ESP32 pode se conectar de qualquer rede WiFi
2. ✅ Seu dashboard Flask pode rodar em qualquer servidor
3. ✅ Você pode monitorar tudo pelo painel do HiveMQ
4. ✅ Escalabilidade garantida até 100 dispositivos

Divirta-se! 🚀
