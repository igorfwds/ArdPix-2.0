// Conectar ao WebSocket
const socket = io();

// Estado da aplicação
let payments = [];
let chart = null;
let statsCache = {
    total: 0,
    today: 0,
    totalToday: 0,
    lastUpdate: null
};

// Throttle para atualizações do ESP32 (evitar spam)
let esp32UpdateTimeout = null;

// Elementos do DOM
const balanceAmount = document.getElementById('balance-amount');
const balanceUpdate = document.getElementById('balance-update');
const esp32Status = document.getElementById('esp32-status');
const lastUpdate = document.getElementById('last-update');
const queueSize = document.getElementById('queue-size');
const totalTransactions = document.getElementById('total-transactions');
const paymentsToday = document.getElementById('payments-today');
const totalToday = document.getElementById('total-today');
const transactionList = document.getElementById('transaction-list');

// Inicializar gráfico (DESABILITADO temporariamente para performance)
function initChart() {
    // Gráfico desabilitado para otimização
    // Descomentar quando necessário
    /*
    const ctx = document.getElementById('paymentsChart').getContext('2d');
    chart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Valor dos Pagamentos (R$)',
                data: [],
                borderColor: '#667eea',
                backgroundColor: 'rgba(102, 126, 234, 0.1)',
                tension: 0.4,
                fill: true
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: false,
            plugins: {
                legend: {
                    display: true,
                    position: 'top'
                }
            },
            scales: {
                y: {
                    beginAtZero: true,
                    ticks: {
                        callback: function(value) {
                            return 'R$ ' + value.toFixed(2);
                        }
                    }
                }
            }
        }
    });
    */
}

// Formatar moeda
function formatCurrency(value) {
    return new Intl.NumberFormat('pt-BR', {
        style: 'currency',
        currency: 'BRL'
    }).format(value);
}

// Formatar data/hora
function formatDateTime(isoString) {
    if (!isoString) return '-';
    const date = new Date(isoString);
    return date.toLocaleString('pt-BR');
}

// Formatar hora
function formatTime(isoString) {
    if (!isoString) return '-';
    const date = new Date(isoString);
    return date.toLocaleTimeString('pt-BR');
}

// Mostrar notificação (otimizado)
let activeNotifications = 0;
const MAX_NOTIFICATIONS = 3;

function showNotification(title, message, type = 'info') {
    // Limitar número de notificações ativas
    if (activeNotifications >= MAX_NOTIFICATIONS) return;

    activeNotifications++;

    const container = document.getElementById('notification-container');
    const notification = document.createElement('div');
    notification.className = `notification ${type}`;
    notification.innerHTML = `
        <div class="notification-title">${title}</div>
        <div class="notification-message">${message}</div>
    `;

    container.appendChild(notification);

    // Remover após 3 segundos (reduzido de 5)
    setTimeout(() => {
        notification.style.opacity = '0';
        setTimeout(() => {
            notification.remove();
            activeNotifications--;
        }, 300);
    }, 3000);
}

// Atualizar saldo
function updateBalance(balance) {
    balanceAmount.textContent = formatCurrency(balance.total);
    balanceUpdate.textContent = formatDateTime(balance.last_update);
}

// Atualizar status do ESP32
function updateESP32Status(status) {
    if (status.connected) {
        esp32Status.textContent = 'Conectado';
        esp32Status.className = 'status-badge connected';
    } else {
        esp32Status.textContent = 'Desconectado';
        esp32Status.className = 'status-badge disconnected';
    }

    lastUpdate.textContent = formatDateTime(status.last_seen);
    queueSize.textContent = status.queue_size || 0;
}

// Adicionar transação à lista (OTIMIZADO)
function addTransaction(payment) {
    // Remover empty state se existir
    const emptyState = transactionList.querySelector('.empty-state');
    if (emptyState) {
        emptyState.remove();
    }

    const item = document.createElement('div');
    item.className = 'transaction-item';  // Removido .new para evitar animação
    item.innerHTML = `
        <div class="transaction-header">
            <span class="transaction-amount">${formatCurrency(payment.amount)}</span>
            <span class="transaction-id">${payment.id}</span>
        </div>
        <div class="transaction-details">
            <span>Método: ${payment.method}</span>
            <span>${formatTime(payment.timestamp)}</span>
        </div>
    `;

    transactionList.insertBefore(item, transactionList.firstChild);

    // Limitar número de transações exibidas a 10 (reduzido de 20)
    while (transactionList.children.length > 10) {
        transactionList.removeChild(transactionList.lastChild);
    }
}

// Atualizar gráfico (DESABILITADO)
function updateChart(payment) {
    // Gráfico desabilitado - nada a fazer
    return;
}

// Atualizar estatísticas (versão otimizada com cache)
function updateStats() {
    // Usar cache para evitar recalcular tudo a cada atualização
    statsCache.total = payments.length;
    totalTransactions.textContent = statsCache.total;

    const today = new Date().toDateString();

    // Apenas recalcular se necessário
    if (statsCache.lastUpdate !== today) {
        const paymentsTodays = payments.filter(p => {
            const paymentDate = new Date(p.timestamp).toDateString();
            return paymentDate === today;
        });

        statsCache.today = paymentsTodays.length;
        statsCache.totalToday = paymentsTodays.reduce((sum, p) => sum + p.amount, 0);
        statsCache.lastUpdate = today;
    }

    paymentsToday.textContent = statsCache.today;
    totalToday.textContent = formatCurrency(statsCache.totalToday);
}

// Atualizar estatísticas incrementalmente (mais eficiente)
function updateStatsIncremental(payment) {
    statsCache.total++;
    totalTransactions.textContent = statsCache.total;

    const today = new Date().toDateString();
    const paymentDate = new Date(payment.timestamp).toDateString();

    if (paymentDate === today) {
        statsCache.today++;
        statsCache.totalToday += payment.amount;
        statsCache.lastUpdate = today;

        paymentsToday.textContent = statsCache.today;
        totalToday.textContent = formatCurrency(statsCache.totalToday);
    }
}

// Carregar dados iniciais (OTIMIZADO)
async function loadInitialData() {
    try {
        // Carregar tudo em paralelo
        const [balanceRes, paymentsRes, statusRes] = await Promise.all([
            fetch('/api/balance'),
            fetch('/api/payments'),
            fetch('/api/esp32/status')
        ]);

        const balance = await balanceRes.json();
        payments = await paymentsRes.json();
        const status = await statusRes.json();

        // Atualizar UI de forma otimizada
        updateBalance(balance);
        updateESP32Status(status);

        // Usar DocumentFragment para adicionar transações (muito mais rápido)
        const fragment = document.createDocumentFragment();
        const emptyState = transactionList.querySelector('.empty-state');
        if (emptyState) {
            emptyState.remove();
        }

        // Limitar a 20 transações mais recentes
        const recentPayments = payments.slice(-20).reverse();

        recentPayments.forEach(payment => {
            const item = document.createElement('div');
            item.className = 'transaction-item';
            item.innerHTML = `
                <div class="transaction-header">
                    <span class="transaction-amount">${formatCurrency(payment.amount)}</span>
                    <span class="transaction-id">${payment.id}</span>
                </div>
                <div class="transaction-details">
                    <span>Método: ${payment.method}</span>
                    <span>${formatTime(payment.timestamp)}</span>
                </div>
            `;
            fragment.appendChild(item);
        });

        transactionList.appendChild(fragment);
        updateStats();

    } catch (error) {
        console.error('Erro ao carregar dados iniciais:', error);
        showNotification('Erro', 'Falha ao carregar dados iniciais', 'error');
    }
}

// Eventos WebSocket

socket.on('connect', () => {
    // Conectado - não mostrar notificação para evitar spam
});

socket.on('disconnect', () => {
    // Desconectado - apenas log
    console.warn('WebSocket desconectado');
});

socket.on('balance_update', (balance) => {
    updateBalance(balance);
});

socket.on('esp32_status', (status) => {
    // Throttle: apenas atualizar a cada 1 segundo no máximo
    if (esp32UpdateTimeout) return;

    updateESP32Status(status);

    esp32UpdateTimeout = setTimeout(() => {
        esp32UpdateTimeout = null;
    }, 1000);
});

// Debounce para notificações
let notificationQueue = [];
let notificationTimeout = null;

socket.on('new_payment', (payment) => {
    payments.push(payment);

    // Usar requestAnimationFrame para otimizar atualizações do DOM
    requestAnimationFrame(() => {
        addTransaction(payment);
        updateStatsIncremental(payment);
    });

    // Debounce de notificações (evitar spam)
    notificationQueue.push(payment);

    if (!notificationTimeout) {
        const firstPayment = notificationQueue[0];
        showNotification(
            'Novo Pagamento!',
            `Recebido: ${formatCurrency(firstPayment.amount)}`,
            'success'
        );

        notificationTimeout = setTimeout(() => {
            if (notificationQueue.length > 1) {
                const total = notificationQueue.reduce((sum, p) => sum + p.amount, 0);
                showNotification(
                    `${notificationQueue.length} Pagamentos`,
                    `Total: ${formatCurrency(total)}`,
                    'success'
                );
            }
            notificationQueue = [];
            notificationTimeout = null;
        }, 2000);
    }
});

socket.on('payment_processed', (data) => {
    if (data.success) {
        showNotification(
            'Pagamento Processado',
            `ID: ${data.payment_id}`,
            'success'
        );
    } else {
        showNotification(
            'Falha no Processamento',
            `ID: ${data.payment_id}`,
            'error'
        );
    }
});

socket.on('withdraw_update', (withdraw) => {
    if (withdraw.status === 'completed') {
        showNotification(
            'Saque Concluído',
            `Valor: ${formatCurrency(withdraw.amount)}`,
            'info'
        );
    } else {
        showNotification(
            'Saque Falhou',
            `Valor: ${formatCurrency(withdraw.amount)}`,
            'error'
        );
    }
});

// Criar pagamento PIX
async function createPixPayment(amount, description) {
    try {
        const response = await fetch('/api/create-pix-payment', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                amount: parseFloat(amount),
                description: description || 'Pagamento ArdPix'
            })
        });

        const data = await response.json();

        if (!response.ok) {
            throw new Error(data.error || 'Erro ao criar pagamento');
        }

        return data;
    } catch (error) {
        console.error('Erro ao criar pagamento PIX:', error);
        throw error;
    }
}

// Mostrar modal com QR Code
function showQRCodeModal(paymentData) {
    const modal = document.getElementById('qrcode-modal');
    const qrImage = document.getElementById('qrcode-image');
    const qrAmount = document.getElementById('qr-amount');
    const qrPaymentId = document.getElementById('qr-payment-id');
    const qrExpiresAt = document.getElementById('qr-expires-at');
    const qrStatus = document.getElementById('qr-status');
    const brCodeText = document.getElementById('brcode-text');

    console.log('Payment data:', paymentData); // Debug

    // Atualizar conteúdo do modal
    // brCodeBase64 já vem com o prefixo data:image/png;base64,
    qrImage.src = paymentData.qrCodeBase64 || '';
    qrAmount.textContent = formatCurrency(paymentData.amount || 0);
    qrPaymentId.textContent = paymentData.id || 'N/A';
    qrExpiresAt.textContent = formatDateTime(paymentData.expiresAt);
    qrStatus.textContent = paymentData.status || 'AGUARDANDO';
    brCodeText.value = paymentData.brCode || '';

    // Mostrar modal
    modal.style.display = 'flex';
}

// Fechar modal
function closeQRCodeModal() {
    const modal = document.getElementById('qrcode-modal');
    modal.style.display = 'none';
}

// Copiar código BR (PIX Copia e Cola)
function copyBRCode() {
    const brCodeText = document.getElementById('brcode-text');
    brCodeText.select();
    brCodeText.setSelectionRange(0, 99999); // Para mobile

    try {
        document.execCommand('copy');
        showNotification('Copiado!', 'Código PIX copiado para área de transferência', 'success');
    } catch (err) {
        // Fallback para navegadores modernos
        navigator.clipboard.writeText(brCodeText.value).then(() => {
            showNotification('Copiado!', 'Código PIX copiado para área de transferência', 'success');
        }).catch(() => {
            showNotification('Erro', 'Falha ao copiar código PIX', 'error');
        });
    }
}

// Handler do formulário de criação de pagamento
document.addEventListener('DOMContentLoaded', () => {
    initChart();
    loadInitialData();

    const createPaymentForm = document.getElementById('create-payment-form');
    const createPaymentBtn = document.getElementById('create-payment-btn');
    const paymentLoading = document.getElementById('payment-loading');

    createPaymentForm.addEventListener('submit', async (e) => {
        e.preventDefault();

        const amount = document.getElementById('payment-amount').value;
        const description = document.getElementById('payment-description').value;

        // Validar valor
        if (!amount || parseFloat(amount) <= 0) {
            showNotification('Erro', 'Por favor, insira um valor válido', 'error');
            return;
        }

        // Mostrar loading
        createPaymentBtn.disabled = true;
        paymentLoading.style.display = 'block';

        try {
            const paymentData = await createPixPayment(amount, description);

            // Esconder loading
            paymentLoading.style.display = 'none';
            createPaymentBtn.disabled = false;

            // Limpar formulário
            createPaymentForm.reset();

            // Mostrar modal com QR Code
            showQRCodeModal(paymentData);

            showNotification('Sucesso!', 'QR Code PIX gerado com sucesso', 'success');

        } catch (error) {
            // Esconder loading
            paymentLoading.style.display = 'none';
            createPaymentBtn.disabled = false;

            showNotification('Erro', error.message || 'Falha ao gerar QR Code PIX', 'error');
        }
    });

    // Fechar modal ao clicar fora
    const modal = document.getElementById('qrcode-modal');
    modal.addEventListener('click', (e) => {
        if (e.target === modal) {
            closeQRCodeModal();
        }
    });
});
