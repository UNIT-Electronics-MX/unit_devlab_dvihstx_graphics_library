// Text mode demo usando Adafruit GFX (funcional)
// Arduino Nano RP2350 + DVI
// Configuración: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+(Blue), D5(GPIO14)=Data1+(Green), D3(GPIO16)=Data2+(Red)

#include <DevLab_DVIHSTX_Graphics.h>

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Colores RGB565
#define COLOR_BLACK   0x0000
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_MAGENTA 0xF81F
#define COLOR_CYAN    0x07FF
#define COLOR_WHITE   0xFFFF

uint16_t colors[] = {
  COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_BLUE,
  COLOR_YELLOW, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE
};

const char* colorNames[] = {
  "Negro", "Rojo", "Verde", "Azul",
  "Amarillo", "Magenta", "Cyan", "Blanco"
};

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("  Text Test - Adafruit GFX Mode");
  Serial.println("  RP2350 DVI/HDMI - 640x480");
  Serial.println("========================================");
  
  if (!display.begin()) {
    Serial.println("ERROR: No se pudo inicializar!");
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) {
      digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
    }
  }
  
  Serial.println("** Display inicializado correctamente **");
  Serial.print("Resolución: ");
  Serial.print(display.width());
  Serial.print("x");
  Serial.println(display.height());
  
  display.fillScreen(COLOR_BLACK);
  display.setTextWrap(true);
}

void loop() {
  // Test 1: Diferentes tamaños de texto
  testTextSizes();
  delay(3000);
  
  // Test 2: Colores de texto
  testTextColors();
  delay(3000);
  
  // Test 3: ASCII Art
  testASCIIArt();
  delay(3000);
  
  // Test 4: Texto scrolling
  testScrollingText();
  delay(3000);
  
  // Test 5: Matriz de caracteres
  testCharacterMatrix();
  delay(3000);
}

void testTextSizes() {
  display.fillScreen(COLOR_BLACK);
  display.setCursor(10, 10);
  
  Serial.println("Test 1: Tamaños de texto");
  
  for (int size = 1; size <= 4; size++) {
    display.setTextSize(size);
    display.setTextColor(COLOR_WHITE);
    display.print("Texto tamano ");
    display.println(size);
    display.println();
  }
}

void testTextColors() {
  display.fillScreen(COLOR_BLACK);
  display.setCursor(10, 10);
  display.setTextSize(2);
  
  Serial.println("Test 2: Colores de texto");
  
  for (int i = 0; i < 8; i++) {
    display.setTextColor(colors[i]);
    display.print(colorNames[i]);
    display.println();
  }
}

void testASCIIArt() {
  display.fillScreen(COLOR_BLACK);
  display.setCursor(10, 10);
  display.setTextSize(1);
  display.setTextColor(COLOR_GREEN);
  
  Serial.println("Test 3: ASCII Art");
  
  // Logo simple
  display.println("  ____  ____  ____  _____  ____   ___  ");
  display.println(" |  _ \\|  _ \\|___ \\|___ / | ___| / _ \\ ");
  display.println(" | |_) | |_) | __) | |_ \\  |___ \\| | | |");
  display.println(" |  _ <|  __/ / __/ ___) |  ___) | |_| |");
  display.println(" |_| \\_\\_|   |_____|____/  |____/ \\___/ ");
  display.println();
  display.setTextColor(COLOR_CYAN);
  display.println(" DVI/HDMI Output Test");
  display.setTextColor(COLOR_YELLOW);
  display.println(" Arduino Nano RP2350 Connect");
  display.println();
  display.setTextColor(COLOR_WHITE);
  display.println(" Pines:");
  display.setTextColor(COLOR_MAGENTA);
  display.println("  D1 (GPIO18) = Clock+");
  display.println("  D7 (GPIO12) = Blue  (Data0)");
  display.println("  D5 (GPIO14) = Green (Data1)");
  display.println("  D3 (GPIO16) = Red   (Data2)");
}

void testScrollingText() {
  display.fillScreen(COLOR_BLUE);
  display.setTextSize(2);
  display.setTextColor(COLOR_YELLOW);
  
  Serial.println("Test 4: Texto en movimiento");
  
  const char* message = "*** RP2350 HSTX DVI ***";
  int messageWidth = strlen(message) * 12; // 12 pixels por caracter aprox (size 2)
  
  for (int x = display.width(); x > -messageWidth; x -= 5) {
    display.fillScreen(COLOR_BLUE);
    display.setCursor(x, display.height() / 2 - 8);
    display.print(message);
    delay(30);
  }
}

void testCharacterMatrix() {
  display.fillScreen(COLOR_BLACK);
  display.setTextSize(1);
  
  Serial.println("Test 5: Matriz de caracteres ASCII");
  
  int x = 5;
  int y = 5;
  int colorIndex = 0;
  
  // Imprimir caracteres ASCII imprimibles (32-126)
  for (int c = 32; c <= 126; c++) {
    display.setTextColor(colors[colorIndex % 8]);
    display.setCursor(x, y);
    display.write(c);
    
    x += 7;
    if (x > display.width() - 10) {
      x = 5;
      y += 10;
      colorIndex++;
      if (y > display.height() - 15) {
        break;
      }
    }
  }
  
  // Mostrar algunos caracteres especiales
  display.setCursor(10, display.height() - 40);
  display.setTextSize(2);
  display.setTextColor(COLOR_WHITE);
  display.println("Especiales: !@#$%^&*()");
  display.println("            <>?/\\|[]{}");
}
