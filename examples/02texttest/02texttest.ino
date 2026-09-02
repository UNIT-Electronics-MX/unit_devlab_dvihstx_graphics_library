// Text mode demo - FIXED VERSION
// With apologies to Stanley Kubrick et al
// Configuración: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+(Blue), D5(GPIO14)=Data1+(Green), D3(GPIO16)=Data2+(Red)
// NOTA: Usa DVHSTX16 con Adafruit_GFX en lugar de DVHSTXText (que no funciona)

#include <DevLab_DVIHSTX_Graphics.h>

#if defined(ADAFRUIT_FEATHER_RP2350_HSTX)
DVHSTXPinout pinConfig = ADAFRUIT_FEATHER_RP2350_CFG;
#elif defined(ADAFRUIT_METRO_RP2350)
DVHSTXPinout pinConfig = ADAFRUIT_METRO_RP2350_CFG;
#elif defined(ARDUINO_ADAFRUIT_FRUITJAM_RP2350)
DVHSTXPinout pinConfig = ADAFRUIT_FRUIT_JAM_CFG;
#elif defined(ARDUINO_NANO_RP2350_CONNECT)
DVHSTXPinout pinConfig = ARDUINO_NANO_RP2350_DVI_CFG;
#elif (defined(ARDUINO_RASPBERRY_PI_PICO_2) || defined(ARDUINO_RASPBERRY_PI_PICO_2W))
DVHSTXPinout pinConfig = ADAFRUIT_HSTXDVIBELL_CFG;
#else
// Configuración estándar: {Clock+, Data0+(Blue), Data1+(Green), Data2+(Red)}
DVHSTXPinout pinConfig = {14, 18, 16, 12};
#endif

// Usar DVHSTX16 con GFX en lugar de DVHSTXText
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

const uint16_t colors[] = {
    COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_BLUE,
    COLOR_YELLOW, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE
};

const char* colorNames[] = {
    "Negro", "Rojo", "Verde", "Azul",
    "Amarillo", "Magenta", "Cyan", "Blanco"
};

int cursorX = 5, cursorY = 5;
int textSize = 1;
uint16_t currentColor = COLOR_WHITE;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Text Test - Using DVHSTX16 with GFX");
  
  if (!display.begin()) {
    Serial.println("ERROR: Display initialization failed!");
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;)
      digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  Serial.println("Display initialized successfully!");
  
  // Fondo blanco, texto negro
  display.fillScreen(COLOR_WHITE);
  display.setTextSize(1);
  display.setTextColor(COLOR_BLACK);
  display.setTextWrap(true);
  display.setCursor(5, 5);
  
  display.println("display initialized");
  display.println("(black on white background)");
  display.println();
  display.println();
  
  // Line wrap test
  display.println("line wrap test. one line should");
  display.println("be full of 'w's and the next");
  display.println("line should start 'xy'.");
  
  for (int i = 0; i < 106; i++) {  // ~106 caracteres en 640px
    display.write('w');
  }
  display.println("xy");
  display.println();
  display.println();
  
  // Color test
  display.println("Color test:");
  display.println();
  
  for (int i = 0; i < 8; i++) {
    display.setTextColor(colors[i], COLOR_WHITE);
    display.print(colorNames[i]);
    display.print("  ");
    if (i == 3) display.println();
  }
  
  display.println();
  display.println();
  
  // Size test
  display.setTextColor(COLOR_BLACK, COLOR_WHITE);
  display.println("Text size test:");
  
  for (int size = 1; size <= 3; size++) {
    display.setTextSize(size);
    display.print("Size ");
    display.println(size);
  }
  
  display.setTextSize(1);
  display.println();
  
  // ASCII test
  display.println("ASCII characters:");
  display.print("!@#$%^&*()_+-=[]{}|;:,.<>?/");
  display.println();
  display.println();
  
  cursorY = display.getCursorY();
  
  Serial.println("Setup complete - starting loop");
}

const char message[] = "All work and no play makes Jack a dull boy ";
int msgIndex = 0;
uint16_t loopColor = COLOR_BLACK;

void loop() {
  // Cambiar color aleatoriamente al inicio de cada frase
  if (msgIndex == 0) {
    loopColor = colors[random(1, 8)];  // Evitar negro a veces
    
    // Agregar algunos espacios aleatorios
    for (int j = random(6); j; j--) {
      display.setTextColor(loopColor, COLOR_WHITE);
      display.write(' ');
    }
  }

  // Escribir el siguiente caracter
  char ch = message[msgIndex++];
  if (ch) {
    display.setTextColor(loopColor, COLOR_WHITE);
    display.write(ch);
  } else {
    msgIndex = 0;
  }

  delay(50 + random(50));
}
