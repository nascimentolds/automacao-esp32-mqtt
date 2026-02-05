#include <Arduino.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>

const uint16_t kRecvPin = 32; // Pino do receptor IR
IRrecv irrecv(kRecvPin);
decode_results results;

void setup() {
  Serial.begin(115200);
  irrecv.enableIRIn(); // Inicia o receptor IR
  Serial.println("Pronto para capturar códigos IR...");
}

void loop() {
  if (irrecv.decode(&results)) {
    Serial.println("Código IR capturado:");
    
    // Exibe o valor RAW (cru) em um formato que pode ser usado no seu código
    Serial.print("uint16_t rawData[");
    Serial.print(results.rawlen - 1);
    Serial.println("] = {");
    
    for (uint16_t i = 1; i < results.rawlen; i++) {
      Serial.print(results.rawbuf[i] * kRawTick);
      if (i < results.rawlen - 1) Serial.print(", ");
      if ((i % 8) == 0) Serial.println();
    }
    Serial.println("};");
    
    Serial.println("\nFormato NEC (se aplicável):");
    Serial.print("Valor: 0x");
    Serial.println(results.value, HEX);
    
    irrecv.resume(); // Prepara para receber o próximo código
  }
  delay(100);
}