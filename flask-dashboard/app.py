import threading
import time
import json
import ssl
import requests
from datetime import datetime
from collections import deque
from flask import Flask, render_template, jsonify
from flask_socketio import SocketIO
import paho.mqtt.client as mqtt

# ==================== CONFIGURAÇÕES ====================

app = Flask(__name__)
app.config['SECRET_KEY'] = 'ardpix-secret-key'

socketio = SocketIO(app, cors_allowed_origins="*", async_mode='threading')

# MQTT
MQTT_BROKER   = '4d976cc55da74717896f08453a044a5b.s1.eu.hivemq.cloud'
MQTT_PORT     = 8883
MQTT_USER     = 'flash-dashboard'
MQTT_PASS     = '#azy8R5VD0QE%Y'

TOPIC_UPDATE  = 'ardpix/value/update'
TOPIC_REQ     = 'ardpix/pix/request'
TOPIC_RES     = 'ardpix/pix/response'
TOPIC_STATUS  = 'ardpix/status'

ABACATE_KEY   = 'abc_prod_06pQKYs0XTpZFnwBSk1MHtbf'
ABACATE_URL   = 'https://api.abacatepay.com/v1'

# ==================== ESTADO E DADOS ====================

STATE = {
    'current_value': 0.0,
    'connected': False
}

PROCESSED_IDS = set()

# Histórico de pagamentos (Guarda os últimos 50 na memória)
PAYMENT_HISTORY = deque(maxlen=50)

# ==================== FUNÇÕES ====================

def generate_pix_worker(amount, req_id):
    try:
        print(f"💰 [API] Gerando QRCode PIX de R$ {amount:.2f}...")
        
        headers = {
            "Authorization": f"Bearer {ABACATE_KEY}",
            "Content-Type": "application/json"
        }
        
        payload = {
            "amount": int(amount * 100),
            "expiresIn": 3600,
            "description": "Pagamento ArdPix Display",
            "customer": {
                "name": "Cliente Balcão",
                "cellphone": "(11) 99999-9999",
                "email": "cliente@ardpix.local",
                "taxId": "110.515.174-35"
            }
        }

        response = requests.post(f"{ABACATE_URL}/pixQrCode/create", json=payload, headers=headers, timeout=10)
        
        if response.status_code == 200:
            data = response.json().get('data', {})
            br_code = data.get('brCode')
            qr_image = data.get('brCodeBase64')
            pix_id = data.get('id', f'pix_{int(time.time())}')

            if br_code and qr_image:
                # 1. Envia para o Cliente (Index)
                socketio.emit('pix_generated', {
                    'amount': amount,
                    'brCode': br_code,      
                    'qrCodeBase64': qr_image
                })

                # 2. Salva no Histórico
                transaction = {
                    'id': pix_id,
                    'amount': amount,
                    'status': 'pending', # Como não temos webhook público, fica pendente
                    'timestamp': datetime.now().isoformat(),
                    'method': 'PIX'
                }
                PAYMENT_HISTORY.appendleft(transaction) # Adiciona no topo

                # 3. Atualiza o Dashboard (Admin)
                socketio.emit('new_payment', transaction)
                
                notify_esp32_success(True)
                print(f"✅ [API] QRCode Gerado e registrado!")
            else:
                notify_esp32_success(False)
        else:
            print(f"❌ [API] Erro {response.status_code}: {response.text}")
            notify_esp32_success(False)

    except Exception as e:
        print(f"❌ [API] Exceção: {e}")
        notify_esp32_success(False)

def notify_esp32_success(success):
    payload = json.dumps({"success": success})
    mqtt_client.publish(TOPIC_RES, payload)

# ==================== MQTT ====================

mqtt_client = mqtt.Client(client_id="flask_server_pix", clean_session=True)

def on_connect(client, userdata, flags, rc):
    print(f"📡 [MQTT] Conectado! (RC: {rc})")
    client.subscribe([(TOPIC_UPDATE, 0), (TOPIC_REQ, 0), (TOPIC_STATUS, 0)])

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        if not payload: return
        data = json.loads(payload)

        if msg.topic == TOPIC_UPDATE:
            val = float(data.get('value', 0.0))
            STATE['current_value'] = val
            socketio.emit('value_update', {'value': val})

        elif msg.topic == TOPIC_STATUS:
            STATE['connected'] = data.get('connected', False)

        elif msg.topic == TOPIC_REQ:
            amount = float(data.get('amount', 0.0))
            req_id = data.get('req_id', str(time.time()))

            if req_id in PROCESSED_IDS: return
            PROCESSED_IDS.add(req_id)
            if len(PROCESSED_IDS) > 50: PROCESSED_IDS.clear()

            if amount > 0:
                t = threading.Thread(target=generate_pix_worker, args=(amount, req_id))
                t.start()

    except Exception as e:
        print(f"❌ [MQTT] Erro: {e}")

# ==================== ROTAS E APIs ====================

def start_mqtt():
    mqtt_client.username_pw_set(MQTT_USER, MQTT_PASS)
    mqtt_client.tls_set(cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLSv1_2)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    try:
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_start() 
    except Exception as e:
        print(f"💀 [FATAL] Erro MQTT: {e}")

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/admin')
def admin():
    return render_template('admin.html')

# APIs para o Dashboard
@app.route('/api/payments')
def get_payments():
    return jsonify(list(PAYMENT_HISTORY))

@app.route('/api/balance')
def get_balance():
    # Soma simples de tudo gerado (pois não temos webhook de confirmação local)
    total = sum(t['amount'] for t in PAYMENT_HISTORY)
    return jsonify({'total': total})

@app.route('/api/current-value')
def get_current():
    return jsonify(STATE)

if __name__ == '__main__':
    print("🚀 === ARDPIX SERVER COM ADMIN ===")
    start_mqtt()
    socketio.run(app, host='0.0.0.0', port=8080, debug=False, allow_unsafe_werkzeug=True)