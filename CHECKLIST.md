# Checklist de Verificação - ArdPix

Use este checklist para garantir que tudo está funcionando antes da apresentação e entrega.

## 📋 Índice
- [Instalação Inicial](#instalação-inicial)
- [Configuração](#configuração)
- [Funcionalidades](#funcionalidades)
- [Testes](#testes)
- [Documentação](#documentação)
- [Apresentação](#apresentação)
- [Entrega Final](#entrega-final)

---

## Instalação Inicial

### Software Base
- [ ] Python 3.8+ instalado
- [ ] pip atualizado (`python3 -m pip install --upgrade pip`)
- [ ] Arduino IDE instalado
- [ ] Git instalado
- [ ] Mosquitto MQTT instalado

### Dependências Python
- [ ] Ambiente virtual criado (`python3 -m venv venv`)
- [ ] Ambiente virtual ativado
- [ ] Dependências instaladas (`pip install -r requirements.txt`)
- [ ] Todas bibliotecas importando sem erro

### Bibliotecas Arduino
- [ ] Suporte ESP32 instalado no Arduino IDE
- [ ] PubSubClient instalada
- [ ] ArduinoJson instalada (versão 6.x)

---

## Configuração

### Arquivos de Configuração
- [ ] `.env` criado a partir do `.env.example`
- [ ] `FLASK_SECRET_KEY` definida (valor único)
- [ ] `WEBHOOK_SECRET` definida (mesma do AbacatePay)
- [ ] `MQTT_BROKER` configurado (localhost ou IP correto)
- [ ] `MQTT_PORT` configurado (padrão: 1883)

### Código ESP32
- [ ] `WIFI_SSID` configurado (nome da sua rede)
- [ ] `WIFI_PASSWORD` configurado (senha da rede)
- [ ] `MQTT_BROKER` configurado (IP do servidor Flask)
- [ ] `MQTT_PORT` configurado (mesmo do Flask, padrão: 1883)

### AbacatePay
- [ ] Conta criada
- [ ] Dev mode ativado (para testes)
- [ ] Webhook configurado corretamente
- [ ] URL do webhook acessível (ngrok para desenvolvimento local)
- [ ] Secret do webhook corresponde ao `.env`

---

## Funcionalidades

### MQTT Broker
- [ ] Mosquitto rodando (`sudo systemctl status mosquitto`)
- [ ] Porta 1883 aberta (`lsof -i :1883`)
- [ ] Consegue publicar mensagem de teste
- [ ] Consegue receber mensagem de teste

### Flask Dashboard
- [ ] Servidor inicia sem erros
- [ ] Porta 5000 acessível
- [ ] Dashboard carrega no navegador
- [ ] Conecta ao MQTT broker
- [ ] WebSocket funciona (ver console do navegador)

### ESP32
- [ ] Código compila sem erros
- [ ] Upload bem-sucedido
- [ ] Conecta ao WiFi (ver Serial Monitor)
- [ ] Conecta ao MQTT (ver Serial Monitor)
- [ ] LEDs testados (acendem/apagam)
- [ ] Buzzer testado (emite som)

---

## Testes

### Teste 1: Comunicação MQTT
- [ ] ESP32 envia status a cada 5 segundos
- [ ] Flask recebe status do ESP32
- [ ] Dashboard mostra "ESP32: Conectado"

### Teste 2: Pagamento Manual (MQTT)
```bash
mosquitto_pub -h localhost -t "ardpix/payment" -m '{
  "payment_id": "test_123",
  "amount": 10.50,
  "timestamp": "2024-12-01T10:00:00",
  "type": "payment"
}'
```
- [ ] ESP32 recebe mensagem (ver Serial Monitor)
- [ ] LED pisca ao receber
- [ ] Pagamento adicionado à fila
- [ ] Processamento executado (LEDs, buzzer)
- [ ] ACK enviado de volta
- [ ] Dashboard NÃO mostra este pagamento (apenas via webhook)

### Teste 3: Webhook Local
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
        "id": "test_payment_456",
        "status": "PAID",
        "amount": 1000
      }
    },
    "devMode": true
  }'
```
- [ ] Flask recebe webhook
- [ ] Retorna `{"received": true}`
- [ ] Dashboard atualiza em tempo real
- [ ] Notificação aparece no dashboard
- [ ] Saldo é atualizado
- [ ] Gráfico é atualizado
- [ ] Transação aparece na lista
- [ ] ESP32 recebe via MQTT
- [ ] ESP32 processa pagamento
- [ ] Confirmação retorna ao Flask

### Teste 4: Pagamento Real (AbacatePay)
- [ ] Criar cobrança no AbacatePay
- [ ] Simular pagamento (dev mode)
- [ ] Webhook é disparado
- [ ] Dashboard atualiza
- [ ] ESP32 processa
- [ ] Fluxo completo funciona em < 5 segundos

### Teste 5: WebSocket
Abrir console do navegador (F12):
- [ ] "Conectado ao servidor WebSocket" aparece
- [ ] Ao receber pagamento, evento `new_payment` é emitido
- [ ] Evento `balance_update` é emitido
- [ ] Evento `esp32_status` é emitido

### Teste 6: Stress Test (Fila)
Enviar 5 pagamentos rapidamente:
```bash
for i in {1..5}; do
  mosquitto_pub -h localhost -t "ardpix/payment" -m "{
    \"payment_id\": \"stress_$i\",
    \"amount\": $((i * 10)),
    \"timestamp\": \"$(date -Iseconds)\",
    \"type\": \"payment\"
  }"
done
```
- [ ] Todos os 5 pagamentos são recebidos
- [ ] Fila não enche (tamanho máximo: 10)
- [ ] Todos são processados em sequência
- [ ] ACKs são enviados para todos

### Teste 7: Reconexão
- [ ] Desligar WiFi → ESP32 tenta reconectar
- [ ] Religar WiFi → ESP32 reconecta automaticamente
- [ ] Parar MQTT broker → ESP32 tenta reconectar
- [ ] Reiniciar MQTT → ESP32 reconecta
- [ ] Fechar Flask → ESP32 continua tentando
- [ ] Reiniciar Flask → comunicação restaurada

---

## Funcionalidades Específicas

### Dashboard Web
- [ ] Exibe saldo atual
- [ ] Exibe última atualização do saldo
- [ ] Lista últimas transações
- [ ] Gráfico de pagamentos funciona
- [ ] Estatísticas calculadas corretamente:
  - [ ] Total de transações
  - [ ] Pagamentos hoje
  - [ ] Valor total hoje
- [ ] Status do ESP32 atualiza
- [ ] Tamanho da fila exibido
- [ ] Notificações aparecem e somem após 5s
- [ ] Design responsivo (testar em mobile)

### ESP32 Firmware
- [ ] Task MQTT roda no Core 1
- [ ] Task Payment Processor roda no Core 0
- [ ] Task Status Report roda no Core 1
- [ ] Fila FreeRTOS funciona
- [ ] Fila aceita até 10 itens
- [ ] Rejeita item 11 (LED vermelho acende)
- [ ] Processamento assíncrono funciona
- [ ] Buzzer apita proporcional ao valor:
  - [ ] < R$ 10,00: 1 beep
  - [ ] R$ 10-49,99: 2 beeps
  - [ ] R$ 50-99,99: 3 beeps
  - [ ] ≥ R$ 100,00: 4 beeps
- [ ] LED verde acende durante processamento
- [ ] LED vermelho acende em erros

### Segurança
- [ ] Webhook valida secret (query string)
- [ ] Webhook valida HMAC (header)
- [ ] Webhook rejeita secret inválido (401)
- [ ] Webhook rejeita HMAC inválido (401)
- [ ] Comparação HMAC é timing-safe

---

## Documentação

### Arquivos Obrigatórios
- [ ] `README.md` completo
- [ ] `requirements.txt` atualizado
- [ ] `.env.example` criado
- [ ] `.gitignore` configurado
- [ ] Código comentado adequadamente

### Documentação Adicional
- [ ] `docs/SETUP_GUIDE.md` completo
- [ ] `docs/ARCHITECTURE.md` completo
- [ ] `docs/APRESENTACAO.md` completo
- [ ] `schematics/ESP32_SCHEMATIC.txt` completo
- [ ] `RESUMO_EXECUTIVO.md` completo
- [ ] `COMANDOS_UTEIS.md` completo

### Diagramas
- [ ] Diagrama de arquitetura criado
- [ ] Esquema de ligação do ESP32 criado
- [ ] Fluxo de dados documentado

---

## GitHub

### Repositório
- [ ] Repositório criado
- [ ] Repositório é público
- [ ] README.md completo e claro
- [ ] `.gitignore` funcionando (não comita `.env`, `venv/`, etc.)

### Commits
- [ ] Commits frequentes (não apenas 1 commit gigante)
- [ ] Mensagens descritivas
- [ ] Histórico organizado
- [ ] Todos os membros do grupo fizeram commits

### Organização
- [ ] Estrutura de pastas clara
- [ ] Código bem organizado
- [ ] Sem arquivos desnecessários commitados
- [ ] Licença definida (MIT sugerida)

---

## Apresentação (04/12)

### Preparação
- [ ] Slides preparados (opcional mas recomendado)
- [ ] Demonstração ensaiada
- [ ] Vídeo backup gravado (caso demo falhe)
- [ ] Tempo cronometrado (15 minutos)
- [ ] Divisão de falas definida

### Equipamentos
- [ ] Laptop carregado
- [ ] ESP32 montado e testado
- [ ] Cabo USB para ESP32
- [ ] Powerbank (backup)
- [ ] Adaptador HDMI/VGA (se necessário)
- [ ] Todos os softwares funcionando

### No Dia (2h antes)
- [ ] Chegar cedo
- [ ] Testar projetor/tela
- [ ] Conectar à WiFi da sala
- [ ] Fazer teste completo
- [ ] AbacatePay aberto e logado
- [ ] Dashboard em tela cheia
- [ ] Serial Monitor aberto
- [ ] Histórico limpo (começar do zero)

### Durante Apresentação
- [ ] Introdução clara
- [ ] **Demonstração ao vivo funciona** ⭐
- [ ] Explicação da arquitetura
- [ ] Detalhes técnicos mencionados
- [ ] Desafios discutidos
- [ ] Conclusão objetiva
- [ ] Preparados para perguntas

---

## Entrega Final (09/12)

### Repositório GitHub
- [ ] Código final commitado
- [ ] README.md atualizado
- [ ] Documentação completa
- [ ] Link do repositório enviado

### Relatório ABNT2
- [ ] Capa completa
- [ ] Introdução escrita
- [ ] Metodologia documentada
- [ ] Resultados apresentados
- [ ] Conclusão escrita
- [ ] Referências formatadas (ABNT)
- [ ] Apêndice com códigos completos
- [ ] PDF gerado e revisado
- [ ] Arquivo nomeado corretamente: `Projeto_IoT_GrupoX.pdf`

### Evidências
- [ ] Prints do dashboard funcionando
- [ ] Fotos do hardware montado
- [ ] Vídeo da demonstração
- [ ] Logs de execução
- [ ] Gráficos/tabelas de resultados

---

## Requisitos do Projeto (Verificação Final)

### Hardware
- [x] 1x ou 2x módulos ESP32
- [x] Sensores básicos (LEDs como indicadores)
- [x] Componentes eletrônicos (resistores, LEDs, buzzer)

### Software
- [x] Broker MQTT (Mosquitto)
- [x] Aplicação Web (Python + Flask)
- [x] ESP32: Firmware FreeRTOS
- [x] Biblioteca MQTT
- [x] Versionamento: GitHub público

### Funcionalidades
- [x] Comunicação WiFi
- [x] Protocolo MQTT para troca de dados
- [x] Dashboard para visualização em tempo real
- [x] Coleta de dados (pagamentos)
- [x] Exibição de dados (dashboard)

### Extras (Bônus)
- [x] Integração com serviço externo (AbacatePay)
- [x] Validação de segurança robusta (HMAC)
- [x] WebSocket para tempo real
- [x] Arquitetura profissional

---

## Pontuação Esperada

| Item | Pontos | Status |
|------|--------|--------|
| Check Point (18/11) | 5 pts | ✅ |
| Funcionamento do Sistema | 25 pts | ⬜ |
| Dashboard | 20 pts | ⬜ |
| Relatório Técnico (ABNT2) | 20 pts | ⬜ |
| Apresentação (04/12) | 15 pts | ⬜ |
| GitHub (Organização) | 15 pts | ⬜ |
| **Bônus** (extras) | +10 pts | ⬜ |
| **TOTAL** | 100-110 pts | - |

**Meta:** 100+ pontos

---

## Últimas Verificações (1 dia antes)

### Ambiente
- [ ] Tudo funciona no laptop que será usado
- [ ] Bateria carregada
- [ ] Backup em pen drive/nuvem

### Código
- [ ] Última versão commitada
- [ ] Sem erros ou warnings
- [ ] Testes passando

### Demo
- [ ] Testada 3+ vezes
- [ ] Tempo < 5 minutos
- [ ] Backup em vídeo pronto

### Equipe
- [ ] Todos ensaiaram
- [ ] Divisão clara de tarefas
- [ ] Preparados para perguntas

---

## Checklist da Manhã da Apresentação

**2 horas antes:**
- [ ] Café tomado ☕
- [ ] Equipamentos conferidos
- [ ] Chegada na sala
- [ ] Teste completo realizado

**30 minutos antes:**
- [ ] Sistema rodando
- [ ] Dashboard aberto
- [ ] ESP32 conectado
- [ ] Respiro fundo 🧘

**5 minutos antes:**
- [ ] Slides/demo prontos
- [ ] Confiante
- [ ] Preparado para arrasar! 🚀

---

## Notas Finais

### ✅ Pronto para apresentar quando:
- Todos os itens de "Funcionalidades" marcados
- Todos os itens de "Testes" passando
- Demo funciona 3+ vezes seguidas
- Documentação completa

### ⚠️ Atenção especial para:
- **Demonstração ao vivo** - É o mais importante!
- Validação HMAC funcionando
- ESP32 processando via fila
- Dashboard atualizando em tempo real

### 🎯 Dicas Finais:
- Teste, teste, teste!
- Tenha sempre um plano B
- Documente tudo
- Trabalhe em equipe
- Peça ajuda quando necessário

---

**BOA SORTE! VOCÊS CONSEGUEM! 🚀**

Quando todos os checkboxes estiverem marcados, você está pronto! ✅
