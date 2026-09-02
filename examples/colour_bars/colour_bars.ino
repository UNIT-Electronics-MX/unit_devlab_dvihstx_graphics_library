// SMPTE Color Bars Test Pattern
// Arduino Nano RP2350 + DVI/HDMI
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+(Blue), D5(GPIO14)=Data1+(Green), D3(GPIO16)=Data2+(Red)
//
// Genera un patrón de barras de colores estándar para verificar:
// - Correcta inicialización del display
// - Sincronización de señal DVI/HDMI
// - Precisión de colores RGB
// - Compatibilidad con el monitor
//
// Control Serial:
// - 'r' = Resolución 320x240
// - 'h' = Resolución 640x480 (requiere más memoria)
// - 's' = SMPTE color bars
// - 'f' = Full screen color test
// - 'g' = Grayscale ramp
// - 'c' = RGB component test

#include <DevLab_DVIHSTX_Graphics.h>

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);
// Colores SMPTE estándar (RGB565)
#define SMPTE_WHITE   0xFFFF  // 255, 255, 255
#define SMPTE_YELLOW  0xFFE0  // 255, 255, 0
#define SMPTE_CYAN    0x07FF  // 0, 255, 255
#define SMPTE_GREEN   0x07E0  // 0, 255, 0
#define SMPTE_MAGENTA 0xF81F  // 255, 0, 255
#define SMPTE_RED     0xF800  // 255, 0, 0
#define SMPTE_BLUE    0x001F  // 0, 0, 255
#define SMPTE_BLACK   0x0000  // 0, 0, 0

// Colores adicionales para pruebas
#define GRAY_75       0xBDF7  // 75% gris
#define GRAY_50       0x7BEF  // 50% gris
#define GRAY_25       0x39E7  // 25% gris

int currentPattern = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("  SMPTE Color Bars Test Pattern");
  Serial.println("  RP2350 DVI/HDMI Test");
  Serial.println("========================================");
  Serial.print("Resoluci\u00f3n: ");
  Serial.print(display.width());
  Serial.print("x");
  Serial.println(display.height());
  Serial.print("F_CPU: ");
  Serial.print(F_CPU / 1000000);
  Serial.println(" MHz");
  Serial.println();
  
  if (!display.begin()) {
    Serial.println("ERROR: No se pudo inicializar el display!");
    Serial.println("Verifica:");
    Serial.println("- Conexiones de pines");
    Serial.println("- Cable HDMI");
    Serial.println("- CPU Speed = 150 MHz en Arduino IDE");
    while (1) delay(100);
  }
  
  Serial.println("** EXITO: Display inicializado! **");
  Serial.println();
  Serial.println("Comandos disponibles:");
  Serial.println("  's' = SMPTE color bars (patr\u00f3n est\u00e1ndar)");
  Serial.println("  'f' = Full screen colors (ciclo completo)");
  Serial.println("  'g' = Grayscale ramp (escala de grises)");
  Serial.println("  'c' = RGB component test (rojo/verde/azul)");
  Serial.println();
  
  drawSMPTEBars();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    
    switch(cmd) {
      case 's':
      case 'S':
        Serial.println("Dibujando: SMPTE Color Bars");
        drawSMPTEBars();
        break;
        
      case 'f':
      case 'F':
        Serial.println("Dibujando: Full Screen Color Test");
        fullScreenColorTest();
        break;
        
      case 'g':
      case 'G':
        Serial.println("Dibujando: Grayscale Ramp");
        drawGrayscaleRamp();
        break;
        
      case 'c':
      case 'C':
        Serial.println("Dibujando: RGB Component Test");
        drawRGBComponentTest();
        break;
    }
  }
  
  delay(100);
}

// Patrón estándar SMPTE color bars
void drawSMPTEBars() {
  int width = display.width();
  int height = display.height();
  int barWidth = width / 7;
  
  // Fila superior (75% de altura) - 7 barras principales
  int topHeight = (height * 3) / 4;
  
  uint16_t topColors[] = {
    SMPTE_WHITE, SMPTE_YELLOW, SMPTE_CYAN, SMPTE_GREEN,
    SMPTE_MAGENTA, SMPTE_RED, SMPTE_BLUE
  };
  
  for (int i = 0; i < 7; i++) {
    display.fillRect(i * barWidth, 0, barWidth, topHeight, topColors[i]);
  }
  
  // Fila inferior (25% de altura) - Barras de pluge
  int bottomY = topHeight;
  int bottomHeight = height - topHeight;
  
  // Azul, Negro, Magenta, Negro, Cyan, Negro, Blanco
  display.fillRect(0, bottomY, barWidth, bottomHeight, SMPTE_BLUE);
  display.fillRect(barWidth, bottomY, barWidth, bottomHeight, SMPTE_BLACK);
  display.fillRect(barWidth * 2, bottomY, barWidth, bottomHeight, SMPTE_MAGENTA);
  display.fillRect(barWidth * 3, bottomY, barWidth, bottomHeight, SMPTE_BLACK);
  display.fillRect(barWidth * 4, bottomY, barWidth, bottomHeight, SMPTE_CYAN);
  display.fillRect(barWidth * 5, bottomY, barWidth, bottomHeight, SMPTE_BLACK);
  display.fillRect(barWidth * 6, bottomY, barWidth, bottomHeight, SMPTE_WHITE);
  
  Serial.println("Patr\u00f3n SMPTE dibujado - 7 barras principales + pluge");
}

// Test de pantalla completa con todos los colores
void fullScreenColorTest() {
  uint16_t colors[] = {
    SMPTE_RED, SMPTE_GREEN, SMPTE_BLUE,
    SMPTE_YELLOW, SMPTE_CYAN, SMPTE_MAGENTA,
    SMPTE_WHITE, SMPTE_BLACK
  };
  
  const char* names[] = {
    "Rojo", "Verde", "Azul",
    "Amarillo", "Cyan", "Magenta",
    "Blanco", "Negro"
  };
  
  for (int i = 0; i < 8; i++) {
    Serial.print("Color: ");
    Serial.println(names[i]);
    display.fillScreen(colors[i]);
    delay(1000);
  }
  
  Serial.println("Test de colores completo");
}

// Rampa de escala de grises
void drawGrayscaleRamp() {
  int width = display.width();
  int height = display.height();
  int steps = 16;
  int barWidth = width / steps;
  
  for (int i = 0; i < steps; i++) {
    // Calcular nivel de gris (RGB565: 5 bits R, 6 bits G, 5 bits B)
    uint8_t level = (i * 255) / (steps - 1);
    uint8_t r = level >> 3;  // 5 bits
    uint8_t g = level >> 2;  // 6 bits
    uint8_t b = level >> 3;  // 5 bits
    
    uint16_t gray = (r << 11) | (g << 5) | b;
    
    display.fillRect(i * barWidth, 0, barWidth, height, gray);
  }
  
  Serial.print("Escala de grises: ");
  Serial.print(steps);
  Serial.println(" niveles");
}

// Test de componentes RGB individuales
void drawRGBComponentTest() {
  int width = display.width();
  int height = display.height();
  int bandHeight = height / 3;
  
  // Banda roja (arriba)
  for (int x = 0; x < width; x++) {
    uint8_t intensity = (x * 31) / width;  // 0-31 (5 bits)
    uint16_t color = intensity << 11;
    display.drawFastVLine(x, 0, bandHeight, color);
  }
  
  // Banda verde (medio)
  for (int x = 0; x < width; x++) {
    uint8_t intensity = (x * 63) / width;  // 0-63 (6 bits)
    uint16_t color = intensity << 5;
    display.drawFastVLine(x, bandHeight, bandHeight, color);
  }
  
  // Banda azul (abajo)
  for (int x = 0; x < width; x++) {
    uint8_t intensity = (x * 31) / width;  // 0-31 (5 bits)
    uint16_t color = intensity;
    display.drawFastVLine(x, bandHeight * 2, bandHeight, color);
  }
  
  Serial.println("Test RGB: Rojo arriba, Verde medio, Azul abajo");
  Serial.println("Gradiente de izquierda (oscuro) a derecha (brillante)");
}
