// Ejemplo para Arduino Nano RP2350 con DVI/HDMI
// Configuración de pines personalizada
//
// Conexiones:
// D1 (GPIO18) -> Clock+
// D7 (GPIO12) -> Blue+ (Data0)
// D5 (GPIO14) -> Green+ (Data1)
// D3 (GPIO16) -> Red+ (Data2)

#include <DevLab_DVIHSTX_Graphics.h>

// Configuración de pines para Arduino Nano RP2350
// Orden: {Clock+, Data0+, Data1+, Data2+}
// Orden DVHSTX: {CKP, D0P, D1P, D2P}
DVHSTXPinout pinConfig = {16, 12, 14, 18};

// Crear display con resolución 320x240
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

void setup() {
  Serial.begin(115200);
  delay(2000); // Esperar a que se abra el monitor serial
  
  Serial.println("========================================");
  Serial.println("  DVI-HSTX Arduino Nano RP2350");
  Serial.println("========================================");
  Serial.println();
  Serial.println("Configuración de pines:");
  Serial.println("  Clock+ : GPIO18 (D1)");
  Serial.println("  Blue+  : GPIO12 (D7) - Data0");
  Serial.println("  Green+ : GPIO14 (D5) - Data1");
  Serial.println("  Red+   : GPIO16 (D3) - Data2");
  Serial.println();
  
  Serial.println("Inicializando display...");
  
  if (!display.begin()) {
    Serial.println("ERROR: No se pudo inicializar el display!");
    Serial.println("Posibles causas:");
    Serial.println("  - RAM insuficiente");
    Serial.println("  - Configuración de pines incorrecta");
    Serial.println("  - Conexiones de hardware incorrectas");
    Serial.println();
    Serial.println("LED parpadeando...");
    
    // Parpadear LED si falla
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
    }
  }
  
  Serial.println("✓ Display inicializado correctamente!");
  Serial.print("Resolución: ");
  Serial.print(display.width());
  Serial.print("x");
  Serial.println(display.height());
  Serial.println();
  Serial.println("Dibujando líneas aleatorias...");
}

void loop() {
  // Dibujar líneas aleatorias con colores aleatorios
  display.drawLine(
    random(display.width()),
    random(display.height()),
    random(display.width()),
    random(display.height()),
    random(65536)  // Color RGB565 aleatorio
  );
  
  delay(1);
}
