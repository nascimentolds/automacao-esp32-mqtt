# Revezamento Automático de Ar-Condicionado com ESP32 e Interface Web

Este projeto, desenvolvido para a disciplina de **Sistemas Embarcados do IFPE - Campus Garanhuns**, é um sistema de automação para gerenciar o revezamento entre dois aparelhos de ar-condicionado (um **LG** e um **Komeco**) usando um **ESP32**, com controle e monitoramento através de uma interface web em tempo real.

## Objetivo
Garantir que apenas um aparelho funcione por vez, automatizando a troca para equilibrar o uso e evitar sobrecarga, ao mesmo tempo que oferece controle remoto ao usuário.

## Principais Funcionalidades
- **Revezamento Automático**: A lógica embarcada no ESP32 garante que ao ligar um ar-condicionado, o outro seja desligado automaticamente.  
- **Controle Remoto via Web**: Uma interface web responsiva permite controlar o aparelho ativo (ligar/desligar, temperatura, modo) de qualquer lugar.  
- **Monitoramento em Tempo Real**: O dashboard exibe o status de ambos os aparelhos (ligado/desligado, temperatura, modo, tempo de uso) e o status da conexão com o broker MQTT.  
- **Comunicação via MQTT**: Utiliza o protocolo MQTT para uma comunicação leve, rápida e confiável entre o hardware e a interface web.  

## Estrutura do Projeto
```
/firmware-esp32   -> Código-fonte em C++ para ser embarcado no ESP32  
/frontend-web     -> Arquivos (HTML, CSS, JavaScript) para a interface web  
```

## Como Executar o Projeto

### Ferramentas Necessárias
**Hardware:**
- Placa de desenvolvimento ESP32  
- Transmissor de IR e Receptor de IR  
- Transistor NPN BC547  
- Resistores (1kΩ e 33Ω)  
- Protoboard e Jumpers  

**Software:**
- Arduino IDE com suporte para ESP32 instalado  
- Broker MQTT (ex.: HiveMQ Cloud)  
- Navegador web moderno (Chrome, Firefox, etc.)  

---

### 1. Configuração do Hardware
Monte o circuito conforme o diagrama abaixo, conectando o transmissor de IR ao pino **GPIO 4** através do transistor **BC547**.  

![Diagrama do Circuito](images/circuito.png)

---

### 2. Configuração e Upload do Firmware (ESP32)

**Instale as Bibliotecas:**
Na Arduino IDE, instale via *Library Manager*:  
- `PubSubClient` (Nick O'Leary)  
- `IRremoteESP8266` (crankyoldgit)  
- `ArduinoJson` (Benoit Blanchon)  

**Configure as Credenciais:**
Edite o arquivo `wifi_broker.ino`:
```cpp
// Credenciais da sua rede Wi-Fi
const char* ssid = "SUA_REDE_WIFI";
const char* password = "SUA_SENHA_WIFI";

// Informações do seu cluster no HiveMQ Cloud (ou outro broker)
const char* mqtt_server = "SEU_BROKER_MQTT_URL";
const char* mqtt_user = "SEU_USUARIO_MQTT";
const char* mqtt_password = "SUA_SENHA_MQTT";
```

**Upload para o ESP32:**
- Conecte o ESP32 via USB  
- Selecione a placa e porta correta na Arduino IDE  
- Clique em **Upload**  

---

### 3. Execução da Interface Web (Frontend)

**Configure as Credenciais:**
Edite o arquivo `script.js`:
```js
// --- Configuração do MQTT ---
const brokerUrl = 'wss://SEU_BROKER_URL:8884/mqtt'; // Use wss:// para conexão segura

// Opções de conexão com as credenciais
const options = {
  username: 'SEU_USUARIO_MQTT',
  password: 'SUA_SENHA_MQTT'
};
```

**Abra no Navegador:**
Abra o arquivo `index.html` em seu navegador.  
Para melhores resultados, utilize um servidor web simples (ex.: extensão **Live Server** no VS Code).  

---

## Licença
Este projeto foi desenvolvido para fins acadêmicos.  
Sinta-se à vontade para adaptá-lo e melhorá-lo.
