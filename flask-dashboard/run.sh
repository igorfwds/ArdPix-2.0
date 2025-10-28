#!/bin/bash
# Script para iniciar o servidor Flask ArdPix

# Cores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== ArdPix Dashboard - Iniciando ===${NC}\n"

# Verificar se o arquivo .env existe
if [ ! -f .env ]; then
    echo -e "${YELLOW}Arquivo .env não encontrado!${NC}"
    echo "Copiando .env.example para .env..."
    cp .env.example .env
    echo -e "${RED}IMPORTANTE: Edite o arquivo .env com suas configurações!${NC}"
    echo ""
fi

# Verificar se venv existe
if [ ! -d "venv" ]; then
    echo -e "${YELLOW}Ambiente virtual não encontrado. Criando...${NC}"
    python3 -m venv venv
fi

# Ativar ambiente virtual
echo "Ativando ambiente virtual..."
source venv/bin/activate

# Instalar/atualizar dependências
echo -e "\n${GREEN}Instalando dependências...${NC}"
pip install -q -r requirements.txt

# Verificar se Mosquitto está rodando
echo -e "\n${GREEN}Verificando broker MQTT...${NC}"
if command -v mosquitto &> /dev/null; then
    if pgrep -x "mosquitto" > /dev/null; then
        echo -e "${GREEN}✓ Mosquitto está rodando${NC}"
    else
        echo -e "${YELLOW}⚠ Mosquitto não está rodando${NC}"
        echo "Execute: sudo systemctl start mosquitto"
    fi
else
    echo -e "${YELLOW}⚠ Mosquitto não está instalado${NC}"
    echo "Instale com: sudo apt install mosquitto mosquitto-clients"
fi

# Iniciar servidor
echo -e "\n${GREEN}Iniciando servidor Flask...${NC}\n"
python app.py
