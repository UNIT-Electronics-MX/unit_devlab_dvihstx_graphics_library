// Test de nueva distribución de pines
// Arduino Nano RP2350 - Configuración alternativa
//
// Nueva distribución de pines HDMI/DVI:
// D0 (GPIO19) -> Data0−
// D1 (GPIO18) -> Data0+
// D2 (GPIO17) -> Data1−
// D3 (GPIO16) -> Data1+
// D4 (GPIO15) -> Clock−
// D5 (GPIO14) -> Clock+
// D6 (GPIO13) -> Data2−
// D7 (GPIO12) -> Data2+

#include <DevLab_DVIHSTX_Graphics.h>

// Nueva configuración de pines
// Orden: {Clock+, Data0+, Data1+, Data2+}
// Pines: {GPIO14, GPIO18, GPIO16, GPIO12}
// Arduino: {D5, D1, D3, D7}
DVHSTXPinout pinConfig = {14, 18, 16, 12};

DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Colores para el test
uint16_t testColors[] = {
  0xF800,  // Rojo
  0x07E0,  // Verde
  0x001F,  // Azul
  0xFFE0,  // Amarillo
  0xF81F,  // Magenta
  0x07FF,  // Cyan
  0xFFFF   // Blanco
};

String colorNames[] = {
  "RED", "GREEN", "BLUE", "YELLOW", "MAGENTA", "CYAN", "WHITE"
};

int currentTest = 0;
unsigned long lastChange = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("╔════════════════════════════════════════════╗");
  Serial.println("║   PIN TEST - Nueva Distribución RP2350   ║");
  Serial.println("╚════════════════════════════════════════════╝");
  Serial.println();
  Serial.println("Nueva configuración de pines:");
  Serial.println("┌──────────┬──────┬─────────────────┐");
  Serial.println("│ Arduino  │ GPIO │ HDMI Signal     │");
  Serial.println("├──────────┼──────┼─────────────────┤");
  Serial.println("│ D0 (RX)  │  19  │ TMDS Data0−     │");
  Serial.println("│ D1 (TX)  │  18  │ TMDS Data0+  ✓  │");
  Serial.println("│ D2       │  17  │ TMDS Data1−     │");
  Serial.println("│ D3       │  16  │ TMDS Data1+  ✓  │");
  Serial.println("│ D4       │  15  │ TMDS Clock−     │");
  Serial.println("│ D5       │  14  │ TMDS Clock+  ✓  │");
  Serial.println("│ D6       │  13  │ TMDS Data2−     │");
  Serial.println("│ D7       │  12  │ TMDS Data2+  ✓  │");
  Serial.println("└──────────┴──────┴─────────────────┘");
  Serial.println();
  Serial.println("Pines positivos usados:");
  Serial.println("  Clock+ : GPIO14 (D5)");
  Serial.println("  Data0+ : GPIO18 (D1)");
  Serial.println("  Data1+ : GPIO16 (D3)");
  Serial.println("  Data2+ : GPIO12 (D7)");
  Serial.println();
  Serial.println("Inicializando display...");
  
  if (!display.begin()) {
    Serial.println();
    Serial.println("╔════════════════════════════════════╗");
    Serial.println("║  ✗ ERROR: Display no inicializado ║");
    Serial.println("╚════════════════════════════════════╝");
    Serial.println();
    Serial.println("Posibles causas:");
    Serial.println("  1. Pines mal conectados");
    Serial.println("  2. Cable HDMI no conectado");
    Serial.println("  3. RAM insuficiente");
    Serial.println("  4. Configuración incorrecta");
    Serial.println();
    Serial.println("Verifica las conexiones:");
    Serial.println("  D5 -> Clock+");
    Serial.println("  D1 -> Data0+ (Blue)");
    Serial.println("  D3 -> Data1+ (Green)");
    Serial.println("  D7 -> Data2+ (Red)");
    Serial.println();
    Serial.println("LED parpadeando...");
    
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(250);
      digitalWrite(LED_BUILTIN, LOW);
      delay(250);
    }
  }
  
  Serial.println();
  Serial.println("╔════════════════════════════════════╗");
  Serial.println("║  ✓ Display inicializado con éxito ║");
  Serial.println("╚════════════════════════════════════╝");
  Serial.println();
  Serial.print("Resolución: ");
  Serial.print(display.width());
  Serial.print(" x ");
  Serial.println(display.height());
  Serial.println();
  Serial.println("Iniciando test de colores...");
  Serial.println("Cada color se mostrará por 3 segundos");
  Serial.println();
  
  lastChange = millis();
}

void loop() {
  // Cambiar test cada 3 segundos
  if (millis() - lastChange > 3000) {
    currentTest++;
    if (currentTest >= 10) {
      currentTest = 0;
    }
    lastChange = millis();
    
    Serial.print("Test ");
    Serial.print(currentTest + 1);
    Serial.print("/10: ");
    
    switch(currentTest) {
      case 0: Serial.println("Pantalla completa ROJA"); break;
      case 1: Serial.println("Pantalla completa VERDE"); break;
      case 2: Serial.println("Pantalla completa AZUL"); break;
      case 3: Serial.println("Pantalla completa AMARILLA"); break;
      case 4: Serial.println("Pantalla completa MAGENTA"); break;
      case 5: Serial.println("Pantalla completa CYAN"); break;
      case 6: Serial.println("Pantalla completa BLANCA"); break;
      case 7: Serial.println("Barras verticales RGB"); break;
      case 8: Serial.println("Tablero de ajedrez"); break;
      case 9: Serial.println("Gradiente de colores"); break;
    }
  }
  
  // Ejecutar test actual
  switch(currentTest) {
    case 0: testSolidColor(0xF800, "RED"); break;
    case 1: testSolidColor(0x07E0, "GREEN"); break;
    case 2: testSolidColor(0x001F, "BLUE"); break;
    case 3: testSolidColor(0xFFE0, "YELLOW"); break;
    case 4: testSolidColor(0xF81F, "MAGENTA"); break;
    case 5: testSolidColor(0x07FF, "CYAN"); break;
    case 6: testSolidColor(0xFFFF, "WHITE"); break;
    case 7: testVerticalBars(); break;
    case 8: testCheckerboard(); break;
    case 9: testGradient(); break;
  }
  
  delay(10);
}

void testSolidColor(uint16_t color, String name) {
  static bool drawn = false;
  static uint16_t lastColor = 0;
  
  if (!drawn || lastColor != color) {
    display.fillScreen(color);
    
    // Dibujar texto informativo
    display.setCursor(10, 10);
    display.setTextColor(0x0000, color);
    display.setTextSize(2);
    display.print("PIN TEST");
    
    display.setCursor(10, 35);
    display.setTextSize(1);
    display.print("Nueva configuracion");
    
    display.setCursor(10, 50);
    display.print("Color: ");
    display.print(name);
    
    // Info de pines
    display.setCursor(10, 75);
    display.print("Clock+: D5 (GPIO14)");
    display.setCursor(10, 90);
    display.print("Data0+: D1 (GPIO18)");
    display.setCursor(10, 105);
    display.print("Data1+: D3 (GPIO16)");
    display.setCursor(10, 120);
    display.print("Data2+: D7 (GPIO12)");
    
    // Marco
    display.drawRect(0, 0, display.width(), display.height(), 
                    color == 0xFFFF ? 0x0000 : 0xFFFF);
    display.drawRect(1, 1, display.width() - 2, display.height() - 2, 
                    color == 0xFFFF ? 0x0000 : 0xFFFF);
    
    drawn = true;
    lastColor = color;
  }
}

void testVerticalBars() {
  static bool drawn = false;
  
  if (!drawn) {
    int barWidth = display.width() / 7;
    
    for (int i = 0; i < 7; i++) {
      display.fillRect(i * barWidth, 0, barWidth, display.height(), testColors[i]);
    }
    
    // Título
    display.setCursor(10, 10);
    display.setTextColor(0x0000);
    display.setTextSize(1);
    display.print("RGB BARS TEST");
    
    drawn = true;
  }
}

void testCheckerboard() {
  static bool drawn = false;
  
  if (!drawn) {
    int squareSize = 20;
    
    for (int y = 0; y < display.height(); y += squareSize) {
      for (int x = 0; x < display.width(); x += squareSize) {
        uint16_t color = ((x / squareSize) + (y / squareSize)) % 2 ? 0xFFFF : 0x0000;
        display.fillRect(x, y, squareSize, squareSize, color);
      }
    }
    
    // Info
    display.setCursor(10, 10);
    display.setTextColor(0xF800);
    display.setTextSize(1);
    display.print("CHECKERBOARD TEST");
    
    drawn = true;
  }
}

void testGradient() {
  static bool drawn = false;
  
  if (!drawn) {
    for (int y = 0; y < display.height(); y++) {
      for (int x = 0; x < display.width(); x++) {
        uint8_t r = map(x, 0, display.width(), 0, 31);
        uint8_t g = map(y, 0, display.height(), 0, 63);
        uint8_t b = map(x + y, 0, display.width() + display.height(), 0, 31);
        
        uint16_t color = (r << 11) | (g << 5) | b;
        display.drawPixel(x, y, color);
      }
    }
    
    // Texto
    display.setCursor(10, 10);
    display.setTextColor(0xFFFF, 0x0000);
    display.setTextSize(1);
    display.print("GRADIENT TEST");
    
    drawn = true;
  }
}
