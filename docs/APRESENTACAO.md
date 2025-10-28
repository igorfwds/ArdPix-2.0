# Guia para Apresentação - ArdPix

**Duração:** 15 minutos
**Data:** 04/12

## Estrutura Sugerida (15 min)

### 1. Introdução (2 min)
- Nome do projeto: **ArdPix**
- Objetivo: Sistema de pagamentos IoT
- Apresentação da equipe
- Contexto: Por que esse projeto é relevante?

### 2. Demonstração ao Vivo (5 min) ⭐ MAIS IMPORTANTE
- **Preparar antes:**
  - Dashboard aberto no navegador
  - ESP32 ligado e conectado
  - Terminal do ESP32 (Serial Monitor) visível
  - Conta AbacatePay em dev mode pronta

- **Demonstrar:**
  1. Estado inicial do dashboard (saldo zero)
  2. Criar cobrança PIX no AbacatePay
  3. Simular pagamento
  4. Mostrar:
     - Dashboard atualizar em tempo real
     - Notificação aparecendo
     - ESP32 processando (Serial Monitor)
     - LEDs acendendo
     - Buzzer apitando
  5. Mostrar confirmação de processamento

### 3. Arquitetura do Sistema (3 min)
- Diagrama de componentes:
  ```
  AbacatePay → Flask → MQTT → ESP32
                 ↓
            Dashboard Web
  ```
- Explicar cada componente brevemente
- Destacar tecnologias usadas:
  - Python + Flask
  - MQTT (Mosquitto)
  - WebSocket (tempo real)
  - FreeRTOS (multitarefa)

### 4. Detalhes Técnicos (3 min)
- **ESP32:**
  - 3 tasks FreeRTOS
  - Fila de processamento
  - Dual-core utilizado

- **Dashboard:**
  - Webhook do AbacatePay
  - Validação HMAC
  - Atualização em tempo real

- **Comunicação:**
  - MQTT para ESP32
  - WebSocket para browsers

### 5. Desafios e Aprendizados (1 min)
- Principal desafio enfrentado
- Como foi resolvido
- Aprendizado mais importante

### 6. Conclusão (1 min)
- Objetivos alcançados
- Possíveis melhorias futuras
- Agradecimentos

## Checklist Pré-Apresentação

### Dia Anterior
- [ ] Testar todo o sistema completo
- [ ] Criar slides (opcional, mas recomendado)
- [ ] Preparar vídeo backup (caso demo falhe)
- [ ] Carregar bateria/powerbank para ESP32
- [ ] Testar projetor/compartilhamento de tela

### 2 Horas Antes
- [ ] Chegar cedo na sala
- [ ] Conectar equipamentos
- [ ] Testar WiFi da sala
- [ ] Fazer teste completo do sistema
- [ ] Ter AbacatePay aberto e logado

### 30 Minutos Antes
- [ ] Iniciar broker MQTT
- [ ] Iniciar Flask dashboard
- [ ] Ligar ESP32
- [ ] Verificar todas conexões
- [ ] Ter terminal ESP32 aberto
- [ ] Ter dashboard aberto em tela cheia
- [ ] Limpar histórico (começar do zero)

## Script de Demonstração

### Roteiro Detalhado

**[0:00 - 2:00] Introdução**

> "Bom dia/tarde. Somos o grupo X e vamos apresentar o ArdPix, um sistema de pagamentos IoT que integra a plataforma AbacatePay com um dispositivo ESP32 usando MQTT e FreeRTOS."

> "O objetivo é demonstrar como pagamentos reais podem acionar ações em dispositivos IoT de forma confiável e em tempo real."

**[2:00 - 7:00] Demonstração**

> "Vejam o dashboard. Atualmente temos saldo zero e nenhuma transação."

> "Vou criar uma cobrança PIX de R$ 25,00 aqui no AbacatePay..."
> *(Mostrar tela do AbacatePay criando cobrança)*

> "Agora vou simular o pagamento..."
> *(Clicar em 'Simular Pagamento' no dev mode)*

> "Observem:"
> - "O dashboard atualizou em tempo real" *(apontar saldo)*
> - "A transação apareceu aqui" *(apontar lista)*
> - "O gráfico foi atualizado" *(apontar gráfico)*
> - "E olhem o ESP32..." *(mostrar Serial Monitor)*
> - "Ele recebeu a mensagem, processou, e..."
> - *(Mostrar LEDs acendendo e buzzer apitando)*
> - "Confirmação enviada de volta!"

> "Todo esse fluxo aconteceu em menos de 2 segundos, de ponta a ponta."

**[7:00 - 10:00] Arquitetura**

> "Como isso funciona por trás?"

> *(Mostrar diagrama)*

> "Quando o pagamento é confirmado, o AbacatePay envia um webhook para nosso servidor Flask."

> "O Flask valida a segurança usando HMAC, atualiza o dashboard via WebSocket, e publica a mensagem no MQTT."

> "O ESP32 está subscrito no tópico MQTT. Quando recebe a mensagem, adiciona à fila FreeRTOS."

> "Temos 3 tasks rodando em paralelo nos dois cores do ESP32..."
> *(Explicar tasks brevemente)*

**[10:00 - 13:00] Detalhes Técnicos**

> "Alguns destaques técnicos:"

> "No ESP32, usamos QueueHandle do FreeRTOS para gerenciar até 10 pagamentos simultâneos."

> "No Flask, implementamos validação dupla: secret na URL e assinatura HMAC no header."

> "A comunicação é assíncrona em todas as camadas, garantindo que nenhum componente bloqueie outro."

**[13:00 - 14:00] Desafios**

> "O maior desafio foi [seu desafio específico]."

> "Resolvemos com [sua solução]."

**[14:00 - 15:00] Conclusão**

> "Conseguimos cumprir todos os requisitos do projeto:"
> - ESP32 com FreeRTOS
> - Comunicação MQTT
> - Dashboard em tempo real
> - Integração com serviço externo

> "Possíveis melhorias: banco de dados, múltiplos ESP32, display OLED..."

> "Agradecemos a atenção. Perguntas?"

## Perguntas Possíveis e Respostas

### "O que acontece se o ESP32 desconectar?"

> "O broker MQTT mantém as mensagens com QoS 1. Quando o ESP32 reconectar, receberá as mensagens pendentes."

### "E se a fila do ESP32 encher?"

> "O código rejeita novas mensagens e acende o LED vermelho. Em produção, poderíamos aumentar o tamanho da fila ou adicionar múltiplos ESP32."

### "Como garantem segurança dos webhooks?"

> "Validamos em duas camadas: secret na query string e assinatura HMAC no header usando a chave pública do AbacatePay."

### "Por que usar MQTT e não HTTP direto?"

> "MQTT é mais leve, suporta publish/subscribe, e mantém conexão persistente. Ideal para IoT com recursos limitados."

### "Funciona com pagamentos reais?"

> "Sim! Basta desativar o dev mode no AbacatePay. Demonstramos em dev mode por segurança."

### "Quantos pagamentos simultâneos suporta?"

> "A fila atual suporta 10 pagamentos. Cada um é processado em ~1-2 segundos. Throughput de ~5-10 pagamentos/minuto."

## Materiais para Levar

### Essencial
- [ ] Laptop com projeto rodando
- [ ] ESP32 montado e testado
- [ ] Cabo USB longo (para ESP32)
- [ ] Powerbank (backup)
- [ ] Cabo adaptador (HDMI/VGA para projetor)

### Recomendado
- [ ] Slides em PDF
- [ ] Vídeo da demonstração (backup)
- [ ] Prints do dashboard salvos
- [ ] Código impresso (apêndice do relatório)
- [ ] Diagramas impressos

### Opcional
- [ ] QR Code para o repositório GitHub
- [ ] Cartões com resumo do projeto
- [ ] Demonstração extra (display OLED, etc)

## Dicas de Apresentação

### Para Falar Bem
1. **Pratique** 3-4 vezes antes
2. **Cronometre** cada seção
3. **Divida** tarefas entre o grupo
4. **Ensaie** transições entre apresentadores

### Para Demo Funcionar
1. **Teste** 30 minutos antes
2. **Tenha backup** (vídeo)
3. **WiFi** pode falhar - tenha hotspot
4. **Mostre** terminal/logs (prova que é real)

### Linguagem Corporal
- Olhe para a plateia (não só para tela)
- Gesticule ao explicar conceitos
- Sorria e demonstre entusiasmo
- Fale devagar e claramente

### Se Algo Der Errado
- **Mantenha calma**
- Use o vídeo backup
- Explique o que deveria acontecer
- Mostre código/logs da última execução

## Divisão de Tarefas Sugerida

**Apresentador 1:** Introdução + Demonstração
**Apresentador 2:** Arquitetura + Tecnologias
**Apresentador 3:** ESP32 + Detalhes técnicos
**Apresentador 4:** Desafios + Conclusão

Todos devem estar prontos para responder perguntas!

## Slide Deck Sugerido (Opcional)

1. **Capa**
   - Nome: ArdPix
   - Integrantes
   - Data

2. **Problema/Motivação**
   - Por que IoT + Pagamentos?
   - Casos de uso

3. **Solução**
   - Diagrama da arquitetura
   - Tecnologias usadas

4. **Demo**
   - (Demonstração ao vivo)

5. **Detalhes Técnicos**
   - FreeRTOS tasks
   - Comunicação MQTT
   - WebSocket

6. **Resultados**
   - Métricas (latência, throughput)
   - Requisitos atendidos

7. **Aprendizados**
   - Principais desafios
   - Soluções encontradas

8. **Futuro**
   - Melhorias possíveis
   - Escalabilidade

9. **Obrigado**
   - GitHub repo (QR Code)
   - Contato

## Métricas para Mencionar

- **Latência ponta-a-ponta:** ~1-3 segundos
- **Taxa de sucesso:** 99%+ (simular falha ocasional no código)
- **Tamanho da fila:** 10 itens
- **Throughput:** ~5-10 pagamentos/minuto
- **Uso de memória ESP32:** ~250KB heap livre
- **Uptime:** Horas de operação contínua

## Critérios de Avaliação - Foco

Lembre-se dos critérios:
- **Clareza** (15 pts)
- **Demonstração ao vivo** (crucial!)
- **Divisão de tarefas no grupo** (15 pts)

Portanto:
- Ensaiem juntos
- Falem claramente
- **GARANTAM que a demo funcione!**

## Último Check - Minutos Antes

```bash
# Terminal 1: MQTT
mosquitto -v

# Terminal 2: Flask
cd flask-dashboard && python app.py

# Terminal 3: Monitor ESP32
# Arduino IDE > Tools > Serial Monitor

# Browser: Dashboard
http://localhost:5000

# Browser 2: AbacatePay
https://abacatepay.com/dashboard
```

Todos devem estar:
- ✅ Conectados
- ✅ Funcionando
- ✅ Visíveis

---

**Boa apresentação! Vocês conseguem! 🚀**
