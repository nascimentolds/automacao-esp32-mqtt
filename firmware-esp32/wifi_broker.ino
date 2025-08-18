// Minhas bibliotecas. Preciso delas para Wi-Fi, MQTT seguro, controle infravermelho, JSON e os protocolos específicos dos meus aparelhos.
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ArduinoJson.h>
#include <ir_LG.h>
#include <ir_Goodweather.h>

// =================== CONFIGURAÇÕES DO USUÁRIO ===================
// Insira aqui as credenciais da sua rede Wi-Fi
const char* ssid = "SUA_REDE_WIFI";
const char* password = "SUA_SENHA_WIFI";

// Insira aqui as informações do seu broker MQTT
const char* mqtt_server = "SEU_BROKER_MQTT_URL"; // Ex: "abcdef123.s1.eu.hivemq.cloud"
const int mqtt_port = 8883;
const char* mqtt_user = "SEU_USUARIO_MQTT";
const char* mqtt_password = "SUA_SENHA_MQTT";
// ===============================================================

// Definição do pino do ESP32 onde conectei o LED IR.
const uint16_t kIrLed = 4;

// Nomes dos tópicos MQTT para organizar a comunicação com cada ar-condicionado.
const char* topic_ac1 = "sala/ac1/comando";
const char* topic_ac2 = "sala/ac2/comando";
const char* state_topic_ac1 = "sala/ac1/estado";
const char* state_topic_ac2 = "sala/ac2/estado";

// Criei uma estrutura para guardar o estado atual de cada ar.
// Assim, eu sei se ele está ligado, a temperatura, modo, etc.
struct ACState {
  bool on = false;
  int mode = 1;
  int fan = 1;
  int temp = 22;
  unsigned long lastOnTime = 0; // Para registrar quando foi ligado
};

// Declaro as variáveis para guardar o estado de cada um dos meus aparelhos.
ACState ac1_currentState;
ACState ac2_currentState;

// Crio os objetos de cliente para a conexão segura (Wi-Fi e MQTT).
WiFiClientSecure espClientSecure;
PubSubClient client(espClientSecure);

// Crio os objetos para controlar cada ar-condicionado pelo infravermelho.
IRLgAc ac1_lg(kIrLed);          // O meu AC1 é um LG.
IRGoodweatherAc ac2_komeco(kIrLed); // O meu AC2 é um Komeco (usa protocolo Goodweather).

// Crio variáveis "flag" para avisar o loop principal que um comando novo chegou.
volatile bool comando_pendente_ac1 = false;
volatile bool comando_pendente_ac2 = false;

// Documentos JSON para armazenar os comandos recebidos.
StaticJsonDocument<256> doc_ac1;
StaticJsonDocument<256> doc_ac2;

// --- Protótipos das minhas funções ---
void controlAc1_Lg(JsonDocument& doc);
void controlAc2_Komeco(JsonDocument& doc);
void publishState(int acNumber);
void setup_wifi();
void reconnect();

// Esta função é chamada automaticamente quando uma mensagem chega em um tópico que eu assinei.
void callback(char* topic, byte* message, unsigned int length) {
  Serial.println("------");
  Serial.print("Mensagem recebida do tópico: ");
  Serial.println(topic);

  // Verifico de qual tópico a mensagem veio e guardo no JSON correspondente.
  if (strcmp(topic, topic_ac1) == 0) {
    deserializeJson(doc_ac1, (char*) message);
    comando_pendente_ac1 = true; // Aviso que tem comando novo para o AC1
  } else if (strcmp(topic, topic_ac2) == 0) {
    deserializeJson(doc_ac2, (char*) message);
    comando_pendente_ac2 = true; // Aviso que tem comando novo para o AC2
  }
}

// Função para conectar o ESP32 na minha rede Wi-Fi.
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// Se a conexão com o MQTT cair, esta função tenta reconectar.
void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão com o broker MQTT...");
    // Ignoro a verificação do certificado para simplificar.
    espClientSecure.setInsecure();
    if (client.connect("ESP32_Sala_ACs", mqtt_user, mqtt_password)) {
      Serial.println(" conectado");
      // Assim que conecta, eu me inscrevo nos tópicos de comando novamente.
      client.subscribe(topic_ac1);
      client.subscribe(topic_ac2);
      Serial.println("Inscrito nos tópicos de comando.");
    } else {
      Serial.print(" falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

// Função de setup, roda uma vez quando o ESP32 liga.
void setup() {
  Serial.begin(115200);

  // Configuro o servidor e a função de callback do MQTT.
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Inicializo os objetos de controle IR.
  ac1_lg.begin();
  ac2_komeco.begin();

  Serial.println("Sistema de Controle de Ar-Condicionado Inicializando...");

  // Garanto que ambos os aparelhos comecem desligados.
  ac1_lg.off();
  ac1_lg.send();
  delay(500); // Pequena pausa entre os comandos.
  ac2_komeco.off();
  ac2_komeco.send(3); // O Komeco precisa de múltiplos envios para garantir.
  Serial.println("Comando inicial 'OFF' enviado para ambos os ACs.");

  // Conecto ao Wi-Fi.
  setup_wifi();
}

// Loop principal, fica rodando sem parar.
void loop() {
  // Se não estiver conectado ao MQTT, tenta reconectar.
  if (!client.connected()) {
    reconnect();
  }
  // Mantém a comunicação MQTT ativa.
  client.loop();

  // Verifico se tem um comando pendente para o AC1.
  if (comando_pendente_ac1) {
    comando_pendente_ac1 = false; // "Processado", reseto a flag.
    // Lógica importante: se o comando é para ligar o AC1, eu desligo o AC2 primeiro.
    if (doc_ac1["on"] | false) {
      Serial.println(">>> Comando para LIGAR o AC1 (LG). Desligando o AC2 (Komeco) primeiro...");
      ac2_komeco.off();
      ac2_komeco.send(3);
      delay(500);
    }
    // Chamo a função para efetivamente controlar o AC1.
    controlAc1_Lg(doc_ac1);
  }

  // Verifico se tem um comando pendente para o AC2.
  if (comando_pendente_ac2) {
    comando_pendente_ac2 = false; // Reseto a flag.
    // Lógica inversa: se o comando é para ligar o AC2, eu desligo o AC1 primeiro.
    if (doc_ac2["on"] | false) {
       Serial.println(">>> Comando para LIGAR o AC2 (Komeco). Desligando o AC1 (LG) primeiro...");
       ac1_lg.off();
       ac1_lg.send();
       delay(500);
    }
    // Chamo a função para controlar o AC2.
    controlAc2_Komeco(doc_ac2);
  }
}

// Função que processa e envia o comando para o ar-condicionado da LG.
void controlAc1_Lg(JsonDocument& doc) {
  Serial.println("Processando comando para o AC1 (LG)...");

  // Atualizo o estado interno com os dados que vieram do JSON.
  if (doc.containsKey("on")) ac1_currentState.on = doc["on"];
  if (doc.containsKey("mode")) ac1_currentState.mode = doc["mode"];
  if (doc.containsKey("fan")) ac1_currentState.fan = doc["fan"];
  if (doc.containsKey("temp")) ac1_currentState.temp = doc["temp"];
  if (ac1_currentState.on && ac1_currentState.lastOnTime == 0) { ac1_currentState.lastOnTime = millis(); } else if (!ac1_currentState.on) { ac1_currentState.lastOnTime = 0; }
  
  // Configuro o objeto do ar LG com base no estado atualizado.
  ac1_currentState.on ? ac1_lg.on() : ac1_lg.off();
  switch (ac1_currentState.mode) {
    case 1: ac1_lg.setMode(kLgAcCool); break;
    case 2: ac1_lg.setMode(kLgAcHeat); break;
    case 3: ac1_lg.setMode(kLgAcFan); break;
    case 4: ac1_lg.setMode(kLgAcDry); break;
    default: ac1_lg.setMode(kLgAcAuto); break;
  }
  switch (ac1_currentState.fan) {
    case 1: ac1_lg.setFan(kLgAcFanLow); break;
    case 2: ac1_lg.setFan(kLgAcFanMedium); break;
    case 3: ac1_lg.setFan(kLgAcFanHigh); break;
    case 4: ac1_lg.setFan(kLgAcFanMax); break;
    default: ac1_lg.setFan(kLgAcFanAuto); break;
  }
  ac1_lg.setTemp(ac1_currentState.temp);
  
  // Envio o comando infravermelho.
  ac1_lg.send();
  Serial.println("O controle do Ar LG está no seguinte estado:");
  Serial.printf("  %s\n", ac1_lg.toString().c_str());
  
  // Publico o novo estado no MQTT para a interface saber que o comando foi executado.
  publishState(1);
}

// Função que processa e envia o comando para o ar-condicionado Komeco.
void controlAc2_Komeco(JsonDocument& doc) {
  Serial.println("Processando comando para o AC2 (Komeco)...");

  // Atualizo o estado interno com os dados do JSON.
  if (doc.containsKey("on")) ac2_currentState.on = doc["on"];
  if (doc.containsKey("mode")) ac2_currentState.mode = doc["mode"];
  if (doc.containsKey("fan")) ac2_currentState.fan = doc["fan"];
  if (doc.containsKey("temp")) ac2_currentState.temp = doc["temp"];
  if (ac2_currentState.on && ac2_currentState.lastOnTime == 0) { ac2_currentState.lastOnTime = millis(); } else if (!ac2_currentState.on) { ac2_currentState.lastOnTime = 0; }
  
  // Configuro o objeto do ar Komeco com base no estado atualizado.
  ac2_komeco.setPower(ac2_currentState.on);
  switch (ac2_currentState.mode) {
    case 1: ac2_komeco.setMode(kGoodweatherCool); break;
    case 2: ac2_komeco.setMode(kGoodweatherHeat); break;
    case 3: ac2_komeco.setMode(kGoodweatherFan); break;
    case 4: ac2_komeco.setMode(kGoodweatherDry); break;
    default: ac2_komeco.setMode(kGoodweatherAuto); break;
  }
  switch (ac2_currentState.fan) {
    case 1: ac2_komeco.setFan(kGoodweatherFanAuto); break;
    case 2: ac2_komeco.setFan(kGoodweatherFanLow); break;
    case 3: ac2_komeco.setFan(kGoodweatherFanMed); break;
    case 4: ac2_komeco.setFan(kGoodweatherFanHigh); break;
    default: ac2_komeco.setFan(kGoodweatherFanAuto); break;
  }
  ac2_komeco.setTemp(ac2_currentState.temp);
  if (doc.containsKey("swing")) ac2_komeco.setSwing(doc["swing"] ? 1 : kGoodweatherSwingOff);
  if (doc.containsKey("light")) ac2_komeco.setLight(doc["light"]);
  if (doc.containsKey("turbo")) ac2_komeco.setTurbo(doc["turbo"]);
  
  // Envio o comando IR, repetindo 3 vezes para garantir a recepção.
  ac2_komeco.send(3);
  Serial.println("O controle do Ar Komeco está no seguinte estado:");
  Serial.printf("  %s\n", ac2_komeco.toString().c_str());
  
  // Publico o novo estado no MQTT.
  publishState(2);
}

// Esta função monta um JSON com o estado atual de um AC e publica no tópico de estado correspondente.
void publishState(int acNumber) {
  JsonDocument stateDoc;
  char buffer[256];
  if (acNumber == 1) {
    stateDoc["on"] = ac1_currentState.on;
    stateDoc["mode"] = ac1_currentState.mode;
    stateDoc["fan"] = ac1_currentState.fan;
    stateDoc["temp"] = ac1_currentState.temp;
    stateDoc["lastOnTime"] = ac1_currentState.lastOnTime;
    serializeJson(stateDoc, buffer);
    client.publish(state_topic_ac1, buffer);
    Serial.println(">>> Estado do AC1 publicado para a interface.");
  } else if (acNumber == 2) {
    stateDoc["on"] = ac2_currentState.on;
    stateDoc["mode"] = ac2_currentState.mode;
    stateDoc["fan"] = ac2_currentState.fan;
    stateDoc["temp"] = ac2_currentState.temp;
    stateDoc["lastOnTime"] = ac2_currentState.lastOnTime;
    serializeJson(stateDoc,  buffer);
    client.publish(state_topic_ac2, buffer);
    Serial.println(">>> Estado do AC2 publicado para a interface.");
  }
}
