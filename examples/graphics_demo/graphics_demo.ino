// Demo gráfico avanzado para Arduino Nano RP2350 + DVI
// Muestra múltiples efectos visuales y animaciones
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+(Blue), D5(GPIO14)=Data1+(Green), D3(GPIO16)=Data2+(Red)

#include <DevLab_DVIHSTX_Graphics.h>

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Estructuras para animación
struct BouncingBall {
  float x, y;
  float vx, vy;
  uint16_t color;
  uint8_t radius;
  
  void update(int width, int height) {
    x += vx;
    y += vy;
    
    if (x - radius < 0 || x + radius >= width) {
      vx = -vx;
      x = constrain(x, radius, width - radius - 1);
      color = random(65536); // Cambiar color al rebotar
    }
    if (y - radius < 0 || y + radius >= height) {
      vy = -vy;
      y = constrain(y, radius, height - radius - 1);
      color = random(65536);
    }
  }
  
  void draw() {
    display.fillCircle((int)x, (int)y, radius, color);
  }
};

BouncingBall balls[5];

// Variables para efectos
uint16_t frameCount = 0;
uint8_t currentDemo = 0;
unsigned long lastDemoChange = 0;
const unsigned long DEMO_DURATION = 10000; // 10 segundos por demo

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("========================================");
  Serial.println("  DVI Graphics Demo - Nano RP2350");
  Serial.println("========================================");
  Serial.println();
  
  if (!display.begin()) {
    Serial.println("ERROR: Display init failed!");
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) {
      digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
    }
  }
  
  Serial.println("✓ Display OK!");
  Serial.print("Resolution: ");
  Serial.print(display.width());
  Serial.print("x");
  Serial.println(display.height());
  Serial.println();
  
  // Inicializar bolas
  for (int i = 0; i < 5; i++) {
    balls[i].x = random(20, display.width() - 20);
    balls[i].y = random(20, display.height() - 20);
    balls[i].vx = random(2, 5) * (random(2) ? 1 : -1);
    balls[i].vy = random(2, 5) * (random(2) ? 1 : -1);
    balls[i].radius = random(5, 15);
    balls[i].color = random(65536);
  }
  
  display.fillScreen(0x0000);
  lastDemoChange = millis();
}

void loop() {
  // Cambiar demo cada 10 segundos
  if (millis() - lastDemoChange > DEMO_DURATION) {
    currentDemo = (currentDemo + 1) % 6;
    display.fillScreen(0x0000);
    lastDemoChange = millis();
    
    Serial.print("Demo ");
    Serial.print(currentDemo + 1);
    Serial.print("/6: ");
    
    switch(currentDemo) {
      case 0: Serial.println("Bouncing Balls"); break;
      case 1: Serial.println("Plasma Effect"); break;
      case 2: Serial.println("Rainbow Bars"); break;
      case 3: Serial.println("Starfield"); break;
      case 4: Serial.println("Circles"); break;
      case 5: Serial.println("Matrix Rain"); break;
    }
  }
  
  switch(currentDemo) {
    case 0: demoBouncingBalls(); break;
    case 1: demoPlasma(); break;
    case 2: demoRainbowBars(); break;
    case 3: demoStarfield(); break;
    case 4: demoCircles(); break;
    case 5: demoMatrixRain(); break;
  }
  
  frameCount++;
}

// Demo 1: Bolas rebotando
void demoBouncingBalls() {
  // Fade efecto
  for (int i = 0; i < display.width() * display.height(); i++) {
    uint16_t pixel = display.getPixel(i % display.width(), i / display.width());
    if (pixel > 0) {
      uint8_t r = ((pixel >> 11) & 0x1F);
      uint8_t g = ((pixel >> 5) & 0x3F);
      uint8_t b = (pixel & 0x1F);
      
      if (r > 0) r--;
      if (g > 1) g -= 2;
      if (b > 0) b--;
      
      display.drawPixel(i % display.width(), i / display.width(),
                       (r << 11) | (g << 5) | b);
    }
  }
  
  // Actualizar y dibujar bolas
  for (int i = 0; i < 5; i++) {
    balls[i].update(display.width(), display.height());
    balls[i].draw();
  }
  
  delay(20);
}

// Demo 2: Efecto plasma
void demoPlasma() {
  for (int y = 0; y < display.height(); y += 2) {
    for (int x = 0; x < display.width(); x += 2) {
      float v = sin((x + frameCount) * 0.05) + 
                sin((y + frameCount) * 0.05) +
                sin((x + y + frameCount) * 0.05);
      
      uint8_t color = (v + 3) * 42;
      uint16_t rgb = display.color565(color, color / 2, 255 - color);
      
      display.fillRect(x, y, 2, 2, rgb);
    }
  }
  delay(30);
}

// Demo 3: Barras arcoíris
void demoRainbowBars() {
  int barHeight = 20;
  int offset = (frameCount * 2) % display.height();
  
  for (int y = 0; y < display.height(); y++) {
    int hue = ((y + offset) * 360 / display.height()) % 360;
    uint16_t color = HSVtoRGB565(hue, 255, 255);
    display.drawFastHLine(0, y, display.width(), color);
  }
  delay(20);
}

// Demo 4: Campo de estrellas
static int stars_x[100], stars_y[100], stars_z[100];
static bool stars_init = false;

void demoStarfield() {
  if (!stars_init) {
    for (int i = 0; i < 100; i++) {
      stars_x[i] = random(-1000, 1000);
      stars_y[i] = random(-1000, 1000);
      stars_z[i] = random(1, 1000);
    }
    stars_init = true;
  }
  
  display.fillScreen(0x0000);
  
  int cx = display.width() / 2;
  int cy = display.height() / 2;
  
  for (int i = 0; i < 100; i++) {
    stars_z[i] -= 5;
    if (stars_z[i] <= 0) {
      stars_x[i] = random(-1000, 1000);
      stars_y[i] = random(-1000, 1000);
      stars_z[i] = 1000;
    }
    
    int sx = (stars_x[i] * 256) / stars_z[i] + cx;
    int sy = (stars_y[i] * 256) / stars_z[i] + cy;
    
    if (sx >= 0 && sx < display.width() && sy >= 0 && sy < display.height()) {
      uint8_t brightness = 255 - (stars_z[i] / 4);
      uint16_t color = display.color565(brightness, brightness, brightness);
      int size = 1 + (1000 - stars_z[i]) / 500;
      display.fillRect(sx, sy, size, size, color);
    }
  }
  delay(30);
}

// Demo 5: Círculos concéntricos
void demoCircles() {
  display.fillScreen(0x0000);
  
  int cx = display.width() / 2;
  int cy = display.height() / 2;
  int maxRadius = min(display.width(), display.height()) / 2;
  
  for (int r = maxRadius; r > 0; r -= 10) {
    int phase = (frameCount + r) % 360;
    uint16_t color = HSVtoRGB565(phase, 255, 255);
    display.drawCircle(cx, cy, r, color);
  }
  
  delay(30);
}

// Demo 6: Matrix rain
static uint8_t matrix_cols[320];
static bool matrix_init = false;

void demoMatrixRain() {
  if (!matrix_init) {
    for (int i = 0; i < display.width(); i++) {
      matrix_cols[i] = random(display.height());
    }
    matrix_init = true;
  }
  
  // Fade
  for (int y = 0; y < display.height(); y++) {
    for (int x = 0; x < display.width(); x++) {
      uint16_t pixel = display.getPixel(x, y);
      if (pixel > 0) {
        uint8_t g = (pixel >> 5) & 0x3F;
        if (g > 2) g -= 3;
        else g = 0;
        display.drawPixel(x, y, g << 5);
      }
    }
  }
  
  // Nuevas gotas
  for (int x = 0; x < display.width(); x += 2) {
    if (random(100) < 5) {
      matrix_cols[x] = 0;
    }
    
    int y = matrix_cols[x];
    if (y < display.height()) {
      uint16_t color = display.color565(0, 255, 0);
      display.drawPixel(x, y, color);
      matrix_cols[x]++;
    }
  }
  
  delay(40);
}

// Convertir HSV a RGB565
uint16_t HSVtoRGB565(int h, int s, int v) {
  h = h % 360;
  s = constrain(s, 0, 255);
  v = constrain(v, 0, 255);
  
  int r, g, b;
  
  if (s == 0) {
    r = g = b = v;
  } else {
    int region = h / 60;
    int remainder = (h % 60) * 255 / 60;
    
    int p = (v * (255 - s)) / 255;
    int q = (v * (255 - (s * remainder) / 255)) / 255;
    int t = (v * (255 - (s * (255 - remainder)) / 255)) / 255;
    
    switch (region) {
      case 0: r = v; g = t; b = p; break;
      case 1: r = q; g = v; b = p; break;
      case 2: r = p; g = v; b = t; break;
      case 3: r = p; g = q; b = v; break;
      case 4: r = t; g = p; b = v; break;
      default: r = v; g = p; b = q; break;
    }
  }
  
  return display.color565(r, g, b);
}
