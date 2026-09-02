// Test simple con la nueva configuración de pines
// Arduino Nano RP2350 - Configuración alternativa
// D5=Clock+, D1=Data0+, D3=Data1+, D7=Data2+

#include <DevLab_DVIHSTX_Graphics.h>

// Nueva distribución: {Clock+, Data0+, Data1+, Data2+}
// {GPIO14, GPIO18, GPIO16, GPIO12} = {D5, D1, D3, D7}
DVHSTXPinout pinConfig = {14, 18, 16, 12};

DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("===========================================");
  Serial.println("  Test Simple - Nueva Config de Pines");
  Serial.println("===========================================");
  Serial.println();
  Serial.println("Pines: D5=CK+, D1=D0+, D3=D1+, D7=D2+");
  Serial.println();
  
  if (!display.begin()) {
    Serial.println("ERROR: Display init failed!");
    Serial.println("Verifica las conexiones de pines.");
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) {
      digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
    }
  }
  
  Serial.println("✓ Display OK!");
  Serial.print("Resolución: ");
  Serial.print(display.width());
  Serial.print("x");
  Serial.println(display.height());
  Serial.println();
  Serial.println("Dibujando líneas aleatorias...");
  
  display.fillScreen(0x0000);
}

void loop() {
  // Dibujar líneas aleatorias
  display.drawLine(
    random(display.width()),
    random(display.height()),
    random(display.width()),
    random(display.height()),
    random(65536)
  );
  
  delay(1);
}
