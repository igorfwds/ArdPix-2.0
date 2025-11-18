from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO, emit
import paho.mqtt.client as mqtt
import hmac
import hashlib
import json
from datetime import datetime
from collections import deque
import os
import threading
import ssl
import requests

app = Flask(__name__)
app.config['SECRET_KEY'] = os.getenv('FLASK_SECRET_KEY', 'ardpix-secret-key-change-me')
socketio = SocketIO(app, cors_allowed_origins="*")

# Configurações
MQTT_BROKER = os.getenv('MQTT_BROKER', 'e804c2fdeb734bf2bbd47edfe90fc9b1.s1.eu.hivemq.cloud')
MQTT_PORT = int(os.getenv('MQTT_PORT', 8883))
MQTT_USERNAME = os.getenv('MQTT_USERNAME', 'flash-dashboard')
MQTT_PASSWORD = os.getenv('MQTT_PASSWORD', '#azy8R5VD0QE%Y')
MQTT_USE_TLS = os.getenv('MQTT_USE_TLS', 'True').lower() == 'true'
MQTT_TOPIC_PAYMENT = 'ardpix/payment'
MQTT_TOPIC_STATUS = 'ardpix/status'
MQTT_TOPIC_QUEUE = 'ardpix/queue'

# AbacatePay API
ABACATEPAY_API_KEY = os.getenv('ABACATEPAY_API_KEY', 'abc_prod_06pQKYs0XTpZFnwBSk1MHtbf')
ABACATEPAY_API_URL = 'https://api.abacatepay.com/v1'

# Armazenamento de dados em memória
payment_history = deque(maxlen=100)  # Últimas 100 transações
balance = {'total': 0.0, 'last_update': None}
esp32_status = {'connected': False, 'last_seen': None, 'queue_size': 0}
payment_queue = deque()  # Fila de pagamentos para processar no ESP32

# Cliente MQTT
mqtt_client = mqtt.Client(client_id="flash-dashboard")

def verify_abacate_signature(raw_body: str, signature_from_header: str) -> bool:
    """Verifica a assinatura HMAC do webhook do AbacatePay"""
    try:
        expected_sig = hmac.new(
            ABACATEPAY_PUBLIC_KEY.encode('utf-8'),
            raw_body.encode('utf-8'),
            hashlib.sha256
        ).digest()

        expected_b64 = hashlib.sha256(expected_sig).hexdigest()

        # Comparação segura
        return hmac.compare_digest(expected_b64, signature_from_header)
    except Exception as e:
        print(f"Erro na verificação de assinatura: {e}")
        return False

def on_mqtt_connect(client, userdata, flags, rc):
    """Callback quando conecta ao MQTT"""
    print(f"Conectado ao MQTT broker com código: {rc}")
    client.subscribe(MQTT_TOPIC_STATUS)
    client.subscribe(f"{MQTT_TOPIC_QUEUE}/ack")

def on_mqtt_message(client, userdata, msg):
    """Callback quando recebe mensagem MQTT"""
    try:
        payload = json.loads(msg.payload.decode())

        if msg.topic == MQTT_TOPIC_STATUS:
            # Atualiza status do ESP32
            old_queue_size = esp32_status.get('queue_size', 0)
            esp32_status['connected'] = payload.get('connected', False)
            esp32_status['last_seen'] = datetime.now().isoformat()
            esp32_status['queue_size'] = payload.get('queue_size', 0)

            # Apenas emitir via WebSocket se houve mudança significativa
            # (evita spam de atualizações de status)
            if old_queue_size != esp32_status['queue_size']:
                socketio.emit('esp32_status', esp32_status)

        elif msg.topic == f"{MQTT_TOPIC_QUEUE}/ack":
            # ESP32 confirmou processamento de um item da fila
            payment_id = payload.get('payment_id')
            success = payload.get('success', False)

            print(f"Pagamento {payment_id} processado: {'sucesso' if success else 'falha'}")

            # Emite notificação via WebSocket
            socketio.emit('payment_processed', {
                'payment_id': payment_id,
                'success': success,
                'timestamp': datetime.now().isoformat()
            })

    except Exception as e:
        print(f"Erro ao processar mensagem MQTT: {e}")

# Configurar callbacks MQTT
mqtt_client.on_connect = on_mqtt_connect
mqtt_client.on_message = on_mqtt_message

# Conectar ao broker MQTT em thread separada
def connect_mqtt():
    try:
        # Configurar autenticação se fornecida
        if MQTT_USERNAME and MQTT_PASSWORD:
            mqtt_client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
            print(f"Autenticação MQTT configurada para usuário: {MQTT_USERNAME}")

        # Configurar TLS/SSL se habilitado
        if MQTT_USE_TLS:
            mqtt_client.tls_set(cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLSv1_2)
            mqtt_client.tls_insecure_set(False)
            print("TLS/SSL habilitado para MQTT")

        print(f"Conectando ao broker: {MQTT_BROKER}:{MQTT_PORT}")
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_start()
        print("Cliente MQTT iniciado com sucesso!")
    except Exception as e:
        print(f"Erro ao conectar ao MQTT: {e}")

# Iniciar conexão MQTT
threading.Thread(target=connect_mqtt, daemon=True).start()

@app.route('/')
def index():
    """Página principal do dashboard"""
    return render_template('index.html')

@app.route('/webhook/abacatepay', methods=['POST'])
def webhook_abacatepay():
    """Endpoint para receber webhooks do AbacatePay"""

    # Processar evento
    event_data = request.json
    event_type = event_data.get('event')

    print(f"Webhook recebido: {event_type}")

    if event_type == 'billing.paid':
        # Processar pagamento confirmado
        handle_payment_paid(event_data)

    elif event_type == 'withdraw.done':
        # Processar saque concluído
        handle_withdraw_done(event_data)

    elif event_type == 'withdraw.failed':
        # Processar saque falho
        handle_withdraw_failed(event_data)

    return jsonify({'received': True}), 200

def handle_payment_paid(event_data):
    """Processa evento de pagamento confirmado"""
    data = event_data.get('data', {})
    payment = data.get('payment', {})
    pix_qr_code = data.get('pixQrCode', {})

    payment_info = {
        'id': pix_qr_code.get('id'),
        'amount': payment.get('amount') / 100,  # Converter de centavos para reais
        'fee': payment.get('fee') / 100,
        'method': payment.get('method'),
        'status': pix_qr_code.get('status'),
        'timestamp': datetime.now().isoformat(),
        'dev_mode': event_data.get('devMode', False)
    }

    # Adicionar ao histórico
    payment_history.append(payment_info)

    # Atualizar saldo
    balance['total'] += payment_info['amount'] - payment_info['fee']
    balance['last_update'] = payment_info['timestamp']

    # Adicionar à fila para processar no ESP32
    queue_item = {
        'type': 'payment',
        'payment_id': payment_info['id'],
        'amount': payment_info['amount'],
        'timestamp': payment_info['timestamp']
    }
    payment_queue.append(queue_item)

    # Publicar no MQTT para ESP32 processar
    mqtt_client.publish(
        MQTT_TOPIC_PAYMENT,
        json.dumps(queue_item),
        qos=1  # Garantir entrega
    )

    print(f"Pagamento recebido: R$ {payment_info['amount']:.2f}")

    # Emitir evento via WebSocket para atualizar dashboard em tempo real
    socketio.emit('new_payment', payment_info)
    socketio.emit('balance_update', balance)

def handle_withdraw_done(event_data):
    """Processa evento de saque concluído"""
    data = event_data.get('data', {})
    transaction = data.get('transaction', {})

    withdraw_info = {
        'id': transaction.get('id'),
        'amount': transaction.get('amount') / 100,
        'fee': transaction.get('platformFee') / 100,
        'status': 'completed',
        'timestamp': datetime.now().isoformat()
    }

    # Atualizar saldo
    balance['total'] -= withdraw_info['amount']
    balance['last_update'] = withdraw_info['timestamp']

    print(f"Saque concluído: R$ {withdraw_info['amount']:.2f}")

    socketio.emit('withdraw_update', withdraw_info)
    socketio.emit('balance_update', balance)

def handle_withdraw_failed(event_data):
    """Processa evento de saque falho"""
    data = event_data.get('data', {})
    transaction = data.get('transaction', {})

    withdraw_info = {
        'id': transaction.get('id'),
        'amount': transaction.get('amount') / 100,
        'status': 'failed',
        'timestamp': datetime.now().isoformat()
    }

    print(f"Saque falhou: R$ {withdraw_info['amount']:.2f}")

    socketio.emit('withdraw_update', withdraw_info)

@app.route('/api/balance', methods=['GET'])
def get_balance():
    """Retorna saldo atual"""
    return jsonify(balance)

@app.route('/api/payments', methods=['GET'])
def get_payments():
    """Retorna histórico de pagamentos"""
    return jsonify(list(payment_history))

@app.route('/api/esp32/status', methods=['GET'])
def get_esp32_status():
    """Retorna status do ESP32"""
    return jsonify(esp32_status)

@app.route('/api/queue', methods=['GET'])
def get_queue():
    """Retorna fila de pagamentos"""
    return jsonify(list(payment_queue))

@app.route('/api/create-pix-payment', methods=['POST'])
def create_pix_payment():
    """Cria um novo pagamento PIX via AbacatePay"""
    try:
        data = request.json
        amount = data.get('amount')  # Valor em reais
        description = data.get('description', 'Pagamento ArdPix')
        expires_in = data.get('expiresIn', 3600)  # 1 hora por padrão

        if not amount or amount <= 0:
            return jsonify({'error': 'Valor inválido'}), 400

        # Converter valor de reais para centavos
        amount_cents = int(amount * 100)

        # Preparar payload para API do AbacatePay
        payload = {
            'amount': amount_cents,
            'description': description,
            'expiresIn': expires_in
        }

        # Fazer requisição para API do AbacatePay
        headers = {
            'Authorization': f'Bearer {ABACATEPAY_API_KEY}',
            'Content-Type': 'application/json'
        }

        response = requests.post(
            f'{ABACATEPAY_API_URL}/pixQrCode/create',
            json=payload,
            headers=headers,
            timeout=30
        )

        if response.status_code == 201 or response.status_code == 200:
            result = response.json()
            print(f"Resposta da API AbacatePay: {result}")  # Log para debug

            # A resposta vem dentro do objeto 'data'
            data = result.get('data', {})

            payment_data = {
                'id': data.get('id'),
                'amount': amount,
                'qrCodeBase64': data.get('brCodeBase64'),  # Nome correto do campo
                'brCode': data.get('brCode'),
                'expiresAt': data.get('expiresAt'),
                'status': data.get('status'),
                'createdAt': data.get('createdAt', datetime.now().isoformat())
            }

            print(f"Pagamento PIX criado: {payment_data['id']} - R$ {amount:.2f}")

            return jsonify(payment_data), 201
        else:
            result = response.json()
            error_msg = result.get('error', 'Erro ao criar pagamento')
            print(f"Erro ao criar pagamento PIX: {response.status_code} - {error_msg}")
            return jsonify({'error': error_msg}), response.status_code

    except requests.exceptions.RequestException as e:
        print(f"Erro de conexão com AbacatePay API: {e}")
        return jsonify({'error': 'Erro de conexão com API de pagamentos'}), 500
    except Exception as e:
        print(f"Erro ao criar pagamento: {e}")
        return jsonify({'error': str(e)}), 500

@socketio.on('connect')
def handle_connect():
    """Cliente WebSocket conectou"""
    print('Cliente conectado via WebSocket')
    # Enviar estado atual
    emit('balance_update', balance)
    emit('esp32_status', esp32_status)

@socketio.on('disconnect')
def handle_disconnect():
    """Cliente WebSocket desconectou"""
    print('Cliente desconectado do WebSocket')

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=8080, debug=True)
