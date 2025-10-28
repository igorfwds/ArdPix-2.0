# Comandos Úteis - ArdPix

Referência rápida de comandos para desenvolvimento e troubleshooting.

## Índice
- [MQTT](#mqtt)
- [Flask](#flask)
- [ESP32](#esp32)
- [Git](#git)
- [Sistema](#sistema)
- [Testes](#testes)

## MQTT

### Iniciar Broker

```bash
# Linux/Raspberry Pi
sudo systemctl start mosquitto
sudo systemctl status mosquitto

# macOS
brew services start mosquitto

# Manualmente (qualquer sistema)
mosquitto -v
```

### Parar Broker

```bash
# Linux/Raspberry Pi
sudo systemctl stop mosquitto

# macOS
brew services stop mosquitto

# Matar processo manualmente
pkill mosquitto
```

### Testar Conexão

```bash
# Terminal 1 - Subscrever
mosquitto_sub -h localhost -t "test/topic" -v

# Terminal 2 - Publicar
mosquitto_pub -h localhost -t "test/topic" -m "Hello MQTT!"
```

### Monitorar Tópicos do ArdPix

```bash
# Todos os tópicos
mosquitto_sub -h localhost -t "ardpix/#" -v

# Apenas pagamentos
mosquitto_sub -h localhost -t "ardpix/payment" -v

# Apenas status
mosquitto_sub -h localhost -t "ardpix/status" -v

# Apenas confirmações
mosquitto_sub -h localhost -t "ardpix/queue/ack" -v
```

### Publicar Pagamento de Teste

```bash
mosquitto_pub -h localhost -t "ardpix/payment" -m '{
  "payment_id": "test_123",
  "amount": 25.50,
  "timestamp": "2024-12-01T10:00:00",
  "type": "payment"
}'
```

### Verificar Broker Rodando

```bash
# Linux
ps aux | grep mosquitto

# Porta 1883 aberta?
netstat -an | grep 1883

# Testar conexão
telnet localhost 1883
```

## Flask

### Ambiente Virtual

```bash
# Criar
python3 -m venv venv

# Ativar (Linux/macOS)
source venv/bin/activate

# Ativar (Windows)
venv\Scripts\activate

# Desativar
deactivate
```

### Instalar Dependências

```bash
cd flask-dashboard
pip install -r requirements.txt

# Atualizar uma dependência
pip install --upgrade flask

# Listar instaladas
pip list
```

### Executar Servidor

```bash
# Método 1: Script
./run.sh

# Método 2: Python direto
python app.py

# Método 3: Flask CLI
export FLASK_APP=app.py
flask run --host=0.0.0.0 --port=5000

# Método 4: Com debug
FLASK_DEBUG=1 python app.py
```

### Ver Logs

```bash
# Se rodando em foreground, logs aparecem no terminal

# Redirecionar para arquivo
python app.py > flask.log 2>&1

# Ver em tempo real
tail -f flask.log
```

### Verificar Servidor Rodando

```bash
# Porta 5000 aberta?
lsof -i :5000

# Testar endpoint
curl http://localhost:5000

# Testar API
curl http://localhost:5000/api/balance
```

### Variáveis de Ambiente

```bash
# Ver variáveis
cat .env

# Editar
nano .env

# Carregar manualmente
export $(cat .env | xargs)

# Verificar variável
echo $MQTT_BROKER
```

## ESP32

### Compilar e Upload (Arduino IDE)

```
Tools > Board > ESP32 Dev Module
Tools > Port > /dev/ttyUSB0 (ou COM3 no Windows)
Sketch > Upload (Ctrl+U)
```

### Monitor Serial

```
Tools > Serial Monitor (Ctrl+Shift+M)
Baud rate: 115200
```

### Monitor Serial (via Terminal)

```bash
# Linux
screen /dev/ttyUSB0 115200

# macOS
screen /dev/cu.usbserial-* 115200

# Sair do screen: Ctrl+A, depois K, depois Y

# Windows (PowerShell)
# Use PuTTY ou Arduino IDE
```

### Limpar Serial Monitor

```bash
# No screen: Ctrl+L

# Ou feche e abra novamente
```

### Descobrir Porta do ESP32

```bash
# Linux
ls /dev/ttyUSB*
ls /dev/ttyACM*

# macOS
ls /dev/cu.*

# Windows (PowerShell)
Get-WmiObject Win32_SerialPort | Select Name,DeviceID

# Ou via Arduino IDE:
# Tools > Port (mostra portas disponíveis)
```

### Encontrar IP do ESP32

```bash
# Veja no Serial Monitor após boot
# Ou escaneie a rede:

# Linux/macOS
nmap -sP 192.168.1.0/24 | grep ESP

# Ou use o roteador para ver dispositivos conectados
```

### Reset ESP32

```bash
# Hardware: Pressione botão RESET na placa
# Software: Desconecte e reconecte USB
# Código: ESP.restart() no Serial Monitor
```

## Git

### Inicializar Repositório

```bash
cd ardpix2
git init
git add .
git commit -m "Initial commit: ArdPix project structure"
```

### Adicionar Remote

```bash
# GitHub
git remote add origin https://github.com/seu-usuario/ardpix.git

# Verificar
git remote -v

# Push inicial
git branch -M main
git push -u origin main
```

### Commits Frequentes

```bash
# Ver status
git status

# Adicionar arquivos
git add .

# Commit
git commit -m "feat: adiciona validação HMAC no webhook"

# Push
git push
```

### Ver Histórico

```bash
# Últimos commits
git log --oneline -10

# Diferenças
git diff

# Ver branches
git branch -a
```

### .gitignore

```bash
# Verificar o que está sendo ignorado
git status --ignored

# Forçar adicionar arquivo ignorado (não recomendado)
git add -f arquivo.txt
```

## Sistema

### Descobrir IP Local

```bash
# Linux
ip addr show | grep inet

# macOS
ifconfig | grep "inet "

# Windows (PowerShell)
ipconfig

# Todos: alternativa simples
hostname -I  # Linux
ipconfig getifaddr en0  # macOS
```

### Verificar Conectividade

```bash
# Ping ao broker MQTT
ping 192.168.1.100

# Testar porta MQTT
telnet 192.168.1.100 1883

# Traceroute
traceroute 192.168.1.100
```

### Firewall

```bash
# Linux (UFW)
sudo ufw allow 1883/tcp  # MQTT
sudo ufw allow 5000/tcp  # Flask
sudo ufw status

# macOS
# Sistema > Segurança > Firewall > Opções

# Desabilitar temporariamente (não recomendado)
sudo ufw disable
```

### Processos

```bash
# Ver processos Python
ps aux | grep python

# Ver processos Mosquitto
ps aux | grep mosquitto

# Matar processo
kill -9 PID

# Matar por nome
pkill -f flask
pkill mosquitto
```

### Portas em Uso

```bash
# Linux
netstat -tulpn | grep LISTEN

# macOS
lsof -nP -iTCP -sTCP:LISTEN

# Ver porta específica
lsof -i :5000
lsof -i :1883
```

## Testes

### Testar Webhook (AbacatePay)

```bash
# Evento de pagamento
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

# Deve retornar: {"received":true}
```

### Testar Endpoints API

```bash
# Saldo
curl http://localhost:5000/api/balance

# Pagamentos
curl http://localhost:5000/api/payments

# Status ESP32
curl http://localhost:5000/api/esp32/status

# Com formatação bonita (jq)
curl http://localhost:5000/api/balance | jq
```

### Testar WebSocket

```javascript
// No console do navegador (F12)
const socket = io();

socket.on('connect', () => {
  console.log('Conectado!');
});

socket.on('new_payment', (data) => {
  console.log('Novo pagamento:', data);
});

socket.on('balance_update', (data) => {
  console.log('Saldo atualizado:', data);
});
```

### Stress Test

```bash
# Enviar múltiplos pagamentos
for i in {1..10}; do
  mosquitto_pub -h localhost -t "ardpix/payment" -m "{
    \"payment_id\": \"test_$i\",
    \"amount\": $(($i * 10)),
    \"timestamp\": \"$(date -Iseconds)\",
    \"type\": \"payment\"
  }"
  sleep 0.5
done
```

## Backup e Restore

### Backup

```bash
# Código
tar -czf ardpix-backup-$(date +%Y%m%d).tar.gz ardpix2/

# Apenas configurações
cp flask-dashboard/.env .env.backup
```

### Restore

```bash
# Extrair backup
tar -xzf ardpix-backup-20241201.tar.gz

# Restaurar configuração
cp .env.backup flask-dashboard/.env
```

## Troubleshooting

### ESP32 não conecta WiFi

```bash
# Verificar SSID existe
nmcli dev wifi list  # Linux

# Verificar força do sinal
iwconfig wlan0  # Linux

# Testar com hotspot do celular
```

### Porta USB ocupada

```bash
# Ver o que está usando
lsof /dev/ttyUSB0

# Matar processo
sudo fuser -k /dev/ttyUSB0

# Permissões (Linux)
sudo usermod -a -G dialout $USER
sudo chmod 666 /dev/ttyUSB0
```

### MQTT não conecta

```bash
# Verificar serviço rodando
sudo systemctl status mosquitto

# Ver logs
sudo journalctl -u mosquitto -f

# Testar manualmente
mosquitto_sub -h localhost -t "test" -v -d

# Reiniciar
sudo systemctl restart mosquitto
```

### Flask não inicia

```bash
# Verificar porta ocupada
lsof -i :5000

# Tentar outra porta
FLASK_PORT=5001 python app.py

# Ver erro completo
python app.py 2>&1 | tee flask-error.log
```

### Dashboard não atualiza

```bash
# Verificar WebSocket no console do navegador (F12)

# Testar conexão MQTT
mosquitto_pub -h localhost -t "ardpix/payment" -m "test"

# Reiniciar tudo
pkill -f flask
pkill mosquitto
sudo systemctl start mosquitto
python app.py
```

## Atalhos Úteis

### Iniciar Tudo

```bash
# Terminal 1: MQTT
mosquitto -v

# Terminal 2: Flask
cd flask-dashboard && source venv/bin/activate && python app.py

# Arduino IDE: Abrir Serial Monitor

# Browser: http://localhost:5000
```

### Parar Tudo

```bash
# Ctrl+C em todos os terminais

# Ou matar processos
pkill -f flask
pkill mosquitto
```

### Ver Tudo

```bash
# 3 terminais lado a lado:
# 1. MQTT logs
# 2. Flask logs
# 3. ESP32 Serial Monitor

# Ou use tmux/screen para múltiplos painéis
```

## Scripts de Automação

### Criar Script de Início

```bash
cat > start-ardpix.sh << 'EOF'
#!/bin/bash
echo "Iniciando ArdPix..."

# MQTT
sudo systemctl start mosquitto &

# Flask
cd flask-dashboard
source venv/bin/activate
python app.py &

# Abrir browser
sleep 2
xdg-open http://localhost:5000  # Linux
# open http://localhost:5000    # macOS

echo "ArdPix iniciado!"
EOF

chmod +x start-ardpix.sh
```

### Criar Script de Parada

```bash
cat > stop-ardpix.sh << 'EOF'
#!/bin/bash
echo "Parando ArdPix..."

pkill -f flask
sudo systemctl stop mosquitto

echo "ArdPix parado!"
EOF

chmod +x stop-ardpix.sh
```

## Referências Rápidas

### Endpoints Flask
- `GET /` - Dashboard
- `POST /webhook/abacatepay` - Webhook
- `GET /api/balance` - Saldo
- `GET /api/payments` - Histórico
- `GET /api/esp32/status` - Status

### Tópicos MQTT
- `ardpix/payment` - Pagamentos (Flask→ESP32)
- `ardpix/status` - Status (ESP32→Flask)
- `ardpix/queue/ack` - Confirmações (ESP32→Flask)

### Pinos ESP32
- `GPIO 2` - LED interno
- `GPIO 4` - Buzzer
- `GPIO 5` - LED verde
- `GPIO 18` - LED vermelho

---

**Dica:** Salve este arquivo nos favoritos para referência rápida! 📌
