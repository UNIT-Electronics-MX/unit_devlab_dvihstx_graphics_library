// Test de diagnóstico - RP2350 DVI/HDMI
// Configuración de pines:
// D1 (GPIO18) = Clock+
// D7 (GPIO12) = Data0+ (Blue)
// D5 (GPIO14) = Data1+ (Green)
// D3 (GPIO16) = Data2+ (Red)

#include <DevLab_DVIHSTX_Graphics.h>

// Configuración: {Clock+, Data0+, Data1+, Data2+}
DVHSTXPinout pinConfig = {18, 12, 14, 16};

DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

void setup() {
  Serial.begin(115200);
  delay(3000); // Dar tiempo para abrir Serial Monitor
  
  Serial.println();
  Serial.println("================================================");
  Serial.println("  DIAGNOSTICO DVI/HDMI - RP2350");
  Serial.println("================================================");
  Serial.println();
  
  Serial.println("Configuracion de pines:");
  Serial.println("  GPIO14 -> Clock+");
  Serial.println("  GPIO18 -> Data0+ (Blue)");
  Serial.println("  GPIO16 -> Data1+ (Green)");
  Serial.println("  GPIO12 -> Data2+ (Red)");
  Serial.println();
  
  Serial.println("Verificando sistema...");
  Serial.print("  F_CPU: ");
  Serial.print(F_CPU / 1000000);
  Serial.println(" MHz");
  
  #if F_CPU != 150000000
    Serial.println("  ADVERTENCIA: F_CPU debe ser 150MHz en el menu");
    Serial.println("  Tools -> CPU Speed -> 150 MHz");
  #endif
  
  Serial.println();
  Serial.println("Intentando inicializar display...");
  Serial.println();
  
  if (!display.begin()) {
    Serial.println("XX ERROR: Display NO inicializado XX");
    Serial.println();
    Serial.println("Checklist:");
    Serial.println("  [ ] Cable HDMI conectado?");
    Serial.println("  [ ] Monitor encendido?");
    Serial.println("  [ ] Pines correctos:");
    Serial.println("      GPIO14, GPIO18, GPIO16, GPIO12");
    Serial.println("  [ ] CPU Speed = 150 MHz?");
    Serial.println("  [ ] Board = Raspberry Pi Pico 2 o Arduino Nano RP2350?");
    Serial.println();
    Serial.println("LED parpadeando rapido = ERROR");
    
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }
  
  Serial.println("** EXITO: Display inicializado! **");
  Serial.println();
  Serial.print("Resolucion: ");
  Serial.print(display.width());
  Serial.print(" x ");
  Serial.println(display.height());
  Serial.println();
  Serial.println("Iniciando test de colores...");
  Serial.println("Deberias ver colores en la pantalla.");
  Serial.println();
  
  // Test básico de colores
  testDisplay();
}

void loop() {
  // Test continuo - cambiar color cada segundo
  static unsigned long lastChange = 0;
  static int colorIndex = 0;
  
  if (millis() - lastChange > 1000) {
    lastChange = millis();
    
    uint16_t colors[] = {
      0xF800,  // Rojo
      0x07E0,  // Verde
      0x001F,  // Azul
      0xFFE0,  // Amarillo
      0xF81F,  // Magenta
      0x07FF,  // Cyan
      0xFFFF   // Blanco
    };
    
    String names[] = {
      "ROJO", "VERDE", "AZUL", "AMARILLO", "MAGENTA", "CYAN", "BLANCO"
    };
    
    display.fillScreen(colors[colorIndex]);
    
    // Texto
    display.setCursor(10, 10);
    display.setTextSize(2);
    display.setTextColor(colorIndex == 6 ? 0x0000 : 0xFFFF);
    display.print("TEST: ");
    display.println(names[colorIndex]);
    
    display.setTextSize(1);
    display.setCursor(10, 40);
    display.print("GPIO14=CK+ GPIO18=D0+");
    display.setCursor(10, 55);
    display.print("GPIO16=D1+ GPIO12=D2+");
    
    // Marco
    display.drawRect(0, 0, display.width(), display.height(), 
                    colorIndex == 6 ? 0x0000 : 0xFFFF);
    
    Serial.print("Mostrando: ");
    Serial.println(names[colorIndex]);
    
    colorIndex = (colorIndex + 1) % 7;
    
    // LED parpadeando lento = OK
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

void testDisplay() {
  Serial.println("Test 1: Pantalla ROJA");
  display.fillScreen(0xF800);
  delay(1000);
  
  Serial.println("Test 2: Pantalla VERDE");
  display.fillScreen(0x07E0);
  delay(1000);
  
  Serial.println("Test 3: Pantalla AZUL");
  display.fillScreen(0x001F);
  delay(1000);
  
  Serial.println("Test 4: Pantalla BLANCA");
  display.fillScreen(0xFFFF);
  delay(1000);
  
  Serial.println("Test inicial completado!");
  Serial.println();
}
