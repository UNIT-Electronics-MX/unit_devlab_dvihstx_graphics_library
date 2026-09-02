// Menú de selección de demos
// Arduino Nano RP2350 + DVI
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+(Blue), D5(GPIO14)=Data1+(Green), D3(GPIO16)=Data2+(Red)
// Control: Serial Monitor - envía número (0-4) para seleccionar demo

#include <DevLab_DVIHSTX_Graphics.h>

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

int currentDemo = -1; // -1 = menú
int lastDemo = -1;
unsigned long demoStartTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║  DEMO SELECTOR - Arduino Nano RP2350  ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  Serial.println("Demos disponibles:");
  Serial.println("  0 - Bouncing Balls");
  Serial.println("  1 - Plasma Effect");
  Serial.println("  2 - Rainbow Bars");
  Serial.println("  3 - Starfield");
  Serial.println("  4 - Matrix Rain");
  Serial.println("  m - Volver al menú");
  Serial.println();
  Serial.println("Envía un número para seleccionar demo");
  Serial.println("========================================");
  Serial.println();
  
  showMenu();
}

void loop() {
  // Leer comandos
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    if (cmd == 'm' || cmd == 'M') {
      currentDemo = -1;
      Serial.println("\n→ Volviendo al menú...\n");
      showMenu();
    } else if (cmd >= '0' && cmd <= '4') {
      currentDemo = cmd - '0';
      demoStartTime = millis();
      display.fillScreen(0x0000);
      
      Serial.print("→ Iniciando Demo ");
      Serial.print(currentDemo);
      Serial.print(": ");
      printDemoName(currentDemo);
      Serial.println();
    }
  }
  
  // Ejecutar demo actual
  if (currentDemo != lastDemo && currentDemo >= 0) {
    initCurrentDemo();
    lastDemo = currentDemo;
  }
  
  if (currentDemo >= 0) {
    runCurrentDemo();
    
    // Mostrar info en esquina
    display.setCursor(5, 5);
    display.setTextColor(0xFFFF, 0x0000);
    display.setTextSize(1);
    display.print("Demo ");
    display.print(currentDemo);
    display.print(" | t:");
    display.print((millis() - demoStartTime) / 1000);
    display.print("s");
  }
}

void showMenu() {
  display.fillScreen(0x0010); // Azul oscuro
  
  // Título
  display.fillRect(0, 0, display.width(), 30, 0x001F);
  display.setCursor(25, 10);
  display.setTextColor(0xFFFF);
  display.setTextSize(2);
  display.print("DEMO SELECTOR");
  
  // Menú
  int y = 50;
  display.setTextSize(1);
  
  for (int i = 0; i < 5; i++) {
    // Número
    display.fillCircle(20, y + 6, 8, 0x07E0);
    display.setCursor(17, y + 2);
    display.setTextColor(0x0000);
    display.print(i);
    
    // Nombre
    display.setCursor(40, y + 2);
    display.setTextColor(0xFFFF);
    printDemoName(i);
    
    y += 25;
  }
  
  // Instrucciones
  display.setCursor(10, 190);
  display.setTextColor(0x07FF); // Cyan
  display.print("Send number via Serial");
  
  display.setCursor(10, 205);
  display.print("to start demo");
  
  display.setCursor(10, 225);
  display.setTextColor(0xF800); // Rojo
  display.print("Press 'm' for menu");
}

void printDemoName(int demo) {
  switch(demo) {
    case 0: Serial.print("Bouncing Balls"); display.print("Bouncing Balls"); break;
    case 1: Serial.print("Plasma Effect"); display.print("Plasma Effect"); break;
    case 2: Serial.print("Rainbow Bars"); display.print("Rainbow Bars"); break;
    case 3: Serial.print("Starfield"); display.print("Starfield"); break;
    case 4: Serial.print("Matrix Rain"); display.print("Matrix Rain"); break;
  }
}

// Variables para demos
struct Ball { float x, y, vx, vy; uint16_t color; uint8_t r; };
Ball balls[5];
int stars_x[50], stars_y[50], stars_z[50];
uint8_t matrix_cols[320];
int frameCount = 0;

void initCurrentDemo() {
  display.fillScreen(0x0000);
  frameCount = 0;
  
  switch(currentDemo) {
    case 0: // Balls
      for (int i = 0; i < 5; i++) {
        balls[i].x = random(20, display.width() - 20);
        balls[i].y = random(20, display.height() - 20);
        balls[i].vx = random(2, 5) * (random(2) ? 1 : -1);
        balls[i].vy = random(2, 5) * (random(2) ? 1 : -1);
        balls[i].r = random(5, 15);
        balls[i].color = random(65536);
      }
      break;
      
    case 3: // Starfield
      for (int i = 0; i < 50; i++) {
        stars_x[i] = random(-1000, 1000);
        stars_y[i] = random(-1000, 1000);
        stars_z[i] = random(1, 1000);
      }
      break;
      
    case 4: // Matrix
      for (int i = 0; i < display.width(); i++) {
        matrix_cols[i] = random(display.height());
      }
      break;
  }
}

void runCurrentDemo() {
  switch(currentDemo) {
    case 0: demoBouncingBalls(); break;
    case 1: demoPlasma(); break;
    case 2: demoRainbowBars(); break;
    case 3: demoStarfield(); break;
    case 4: demoMatrixRain(); break;
  }
  frameCount++;
}

void demoBouncingBalls() {
  // Fade
  for (int y = 0; y < display.height(); y += 2) {
    for (int x = 0; x < display.width(); x += 2) {
      uint16_t p = display.getPixel(x, y);
      if (p > 0) {
        uint8_t r = ((p >> 11) & 0x1F);
        uint8_t g = ((p >> 5) & 0x3F);
        uint8_t b = (p & 0x1F);
        if (r > 0) r--;
        if (g > 1) g -= 2;
        if (b > 0) b--;
        display.drawPixel(x, y, (r << 11) | (g << 5) | b);
      }
    }
  }
  
  for (int i = 0; i < 5; i++) {
    balls[i].x += balls[i].vx;
    balls[i].y += balls[i].vy;
    
    if (balls[i].x - balls[i].r < 0 || balls[i].x + balls[i].r >= display.width()) {
      balls[i].vx = -balls[i].vx;
      balls[i].color = random(65536);
    }
    if (balls[i].y - balls[i].r < 0 || balls[i].y + balls[i].r >= display.height()) {
      balls[i].vy = -balls[i].vy;
      balls[i].color = random(65536);
    }
    
    display.fillCircle((int)balls[i].x, (int)balls[i].y, balls[i].r, balls[i].color);
  }
  delay(20);
}

void demoPlasma() {
  for (int y = 0; y < display.height(); y += 3) {
    for (int x = 0; x < display.width(); x += 3) {
      float v = sin((x + frameCount * 2) * 0.05) + 
                sin((y + frameCount * 2) * 0.05);
      uint8_t c = (v + 2) * 63;
      uint16_t rgb = display.color565(c, c / 2, 255 - c);
      display.fillRect(x, y, 3, 3, rgb);
    }
  }
  delay(30);
}

void demoRainbowBars() {
  int offset = (frameCount * 2) % display.height();
  for (int y = 0; y < display.height(); y++) {
    int hue = ((y + offset) * 360 / display.height()) % 360;
    display.drawFastHLine(0, y, display.width(), HSVtoRGB(hue));
  }
  delay(20);
}

void demoStarfield() {
  display.fillScreen(0x0000);
  int cx = display.width() / 2;
  int cy = display.height() / 2;
  
  for (int i = 0; i < 50; i++) {
    stars_z[i] -= 5;
    if (stars_z[i] <= 0) {
      stars_x[i] = random(-1000, 1000);
      stars_y[i] = random(-1000, 1000);
      stars_z[i] = 1000;
    }
    
    int sx = (stars_x[i] * 256) / stars_z[i] + cx;
    int sy = (stars_y[i] * 256) / stars_z[i] + cy;
    
    if (sx >= 0 && sx < display.width() && sy >= 0 && sy < display.height()) {
      uint8_t b = 255 - (stars_z[i] / 4);
      display.fillRect(sx, sy, 2, 2, display.color565(b, b, b));
    }
  }
  delay(30);
}

void demoMatrixRain() {
  // Fade
  for (int y = 0; y < display.height(); y++) {
    for (int x = 0; x < display.width(); x++) {
      uint16_t p = display.getPixel(x, y);
      if (p > 0) {
        uint8_t g = (p >> 5) & 0x3F;
        if (g > 2) g -= 3;
        else g = 0;
        display.drawPixel(x, y, g << 5);
      }
    }
  }
  
  for (int x = 0; x < display.width(); x += 3) {
    if (random(100) < 5) matrix_cols[x] = 0;
    
    int y = matrix_cols[x];
    if (y < display.height()) {
      display.drawPixel(x, y, 0x07E0);
      matrix_cols[x]++;
    }
  }
  delay(40);
}

uint16_t HSVtoRGB(int h) {
  h = h % 360;
  int region = h / 60;
  int remainder = (h % 60) * 255 / 60;
  
  int p = 0, q = 255 - remainder, t = remainder;
  int r, g, b;
  
  switch (region) {
    case 0: r = 255; g = t; b = p; break;
    case 1: r = q; g = 255; b = p; break;
    case 2: r = p; g = 255; b = t; break;
    case 3: r = p; g = q; b = 255; break;
    case 4: r = t; g = p; b = 255; break;
    default: r = 255; g = p; b = q; break;
  }
  
  return display.color565(r, g, b);
}
