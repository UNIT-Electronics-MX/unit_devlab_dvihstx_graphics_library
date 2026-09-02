// TRON Mountains Advanced - Paisaje 3D personalizable
// Arduino Nano RP2350 + DVI
// Control por Serial: '+'/'-' velocidad, 'c' cambiar color, 'w' wireframe, 'f' filled
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+(Blue), D5(GPIO14)=Data1+(Green), D3(GPIO16)=Data2+(Red)

#include <DevLab_DVIHSTX_Graphics.h>

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Configuración del terreno
const int GRID_W = 32;
const int GRID_D = 32;
float terrain[GRID_D][GRID_W];

// Parámetros de cámara
float camY = 80;
float camZ = -50;
float horizon = 100;
float scrollSpeed = 0.3;
float scrollOffset = 0;

// Modos de visualización
enum RenderMode { WIREFRAME, FILLED, BOTH };
RenderMode renderMode = WIREFRAME;

// Esquemas de color
enum ColorScheme { TRON_BLUE, TRON_ORANGE, MATRIX_GREEN, SYNTHWAVE, CUSTOM };
ColorScheme currentScheme = TRON_BLUE;

struct ColorPalette {
  uint16_t sky1, sky2;
  uint16_t grid;
  uint16_t peak;
  uint16_t valley;
};

ColorPalette palette;

// FPS counter
int fps = 0;
int frameCount = 0;
unsigned long lastFpsTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  Serial.println("╔═══════════════════════════════════════════╗");
  Serial.println("║  TRON MOUNTAINS ADVANCED - Nano RP2350   ║");
  Serial.println("╚═══════════════════════════════════════════╝");
  Serial.println();
  Serial.println("Controles:");
  Serial.println("  + / -  : Aumentar/Disminuir velocidad");
  Serial.println("  c      : Cambiar esquema de color");
  Serial.println("  w      : Modo wireframe");
  Serial.println("  f      : Modo filled");
  Serial.println("  b      : Ambos modos");
  Serial.println("  r      : Reset cámara");
  Serial.println();
  
  setColorScheme(TRON_BLUE);
  generateTerrain();
  
  Serial.println("✓ Listo! Iniciando viaje...");
  Serial.println();
}

void loop() {
  // Procesar comandos
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }
  
  // Actualizar FPS
  frameCount++;
  if (millis() - lastFpsTime >= 1000) {
    fps = frameCount;
    frameCount = 0;
    lastFpsTime = millis();
  }
  
  // Actualizar terreno
  scrollOffset += scrollSpeed;
  if (scrollOffset >= 1.0) {
    scrollOffset -= 1.0;
    shiftTerrain();
  }
  
  // Renderizar
  drawScene();
  
  delay(16); // Target ~60 FPS
}

void handleCommand(char cmd) {
  switch(cmd) {
    case '+':
      scrollSpeed += 0.1;
      scrollSpeed = min(scrollSpeed, 2.0f);
      Serial.print("Velocidad: ");
      Serial.println(scrollSpeed, 1);
      break;
      
    case '-':
      scrollSpeed -= 0.1;
      scrollSpeed = max(scrollSpeed, 0.1f);
      Serial.print("Velocidad: ");
      Serial.println(scrollSpeed, 1);
      break;
      
    case 'c':
      currentScheme = (ColorScheme)((currentScheme + 1) % 5);
      setColorScheme(currentScheme);
      printColorScheme();
      break;
      
    case 'w':
      renderMode = WIREFRAME;
      Serial.println("Modo: Wireframe");
      break;
      
    case 'f':
      renderMode = FILLED;
      Serial.println("Modo: Filled");
      break;
      
    case 'b':
      renderMode = BOTH;
      Serial.println("Modo: Both");
      break;
      
    case 'r':
      camY = 80;
      camZ = -50;
      scrollSpeed = 0.3;
      Serial.println("Cámara reseteada");
      break;
  }
}

void setColorScheme(ColorScheme scheme) {
  currentScheme = scheme;
  
  switch(scheme) {
    case TRON_BLUE:
      palette.sky1 = 0x0000;
      palette.sky2 = 0x001F;
      palette.grid = 0x07FF;
      palette.peak = 0xFFFF;
      palette.valley = 0x0208;
      break;
      
    case TRON_ORANGE:
      palette.sky1 = 0x0000;
      palette.sky2 = 0x4000;
      palette.grid = 0xFD20;
      palette.peak = 0xFFE0;
      palette.valley = 0x4200;
      break;
      
    case MATRIX_GREEN:
      palette.sky1 = 0x0000;
      palette.sky2 = 0x0200;
      palette.grid = 0x07E0;
      palette.peak = 0xFFE0;
      palette.valley = 0x0180;
      break;
      
    case SYNTHWAVE:
      palette.sky1 = 0x4008;
      palette.sky2 = 0xF81F;
      palette.grid = 0xF81F;
      palette.peak = 0xFFE0;
      palette.valley = 0x4810;
      break;
      
    case CUSTOM:
      palette.sky1 = 0x0000;
      palette.sky2 = 0x780F;
      palette.grid = 0x07FF;
      palette.peak = 0xF800;
      palette.valley = 0x0410;
      break;
  }
}

void printColorScheme() {
  Serial.print("Color Scheme: ");
  switch(currentScheme) {
    case TRON_BLUE: Serial.println("TRON Blue"); break;
    case TRON_ORANGE: Serial.println("TRON Orange"); break;
    case MATRIX_GREEN: Serial.println("Matrix Green"); break;
    case SYNTHWAVE: Serial.println("Synthwave"); break;
    case CUSTOM: Serial.println("Custom"); break;
  }
}

void drawScene() {
  // Cielo con gradiente
  for (int y = 0; y < display.height() / 2; y++) {
    uint16_t color = lerpColor(palette.sky1, palette.sky2, 
                                (float)y / (display.height() / 2));
    display.drawFastHLine(0, y, display.width(), color);
  }
  
  // Suelo negro
  display.fillRect(0, display.height() / 2, display.width(), 
                   display.height() / 2, 0x0000);
  
  // Grid del suelo
  for (int y = display.height() / 2; y < display.height(); y += 10) {
    display.drawFastHLine(0, y, display.width(), 0x0208);
  }
  
  // Dibujar terreno
  drawTerrain();
  
  // HUD
  drawHUD();
}

void drawTerrain() {
  int centerX = display.width() / 2;
  
  for (int z = GRID_D - 1; z > 0; z--) {
    for (int x = 0; x < GRID_W - 1; x++) {
      float z1 = z + scrollOffset;
      
      // Vértices
      float wx1 = (x - GRID_W / 2) * 10;
      float wy1 = terrain[z][x];
      float wz1 = z1 * 10;
      
      float wx2 = (x + 1 - GRID_W / 2) * 10;
      float wy2 = terrain[z][x + 1];
      float wz2 = z1 * 10;
      
      float wx3 = (x - GRID_W / 2) * 10;
      float wy3 = terrain[z - 1][x];
      float wz3 = (z1 - 1) * 10;
      
      float wx4 = (x + 1 - GRID_W / 2) * 10;
      float wy4 = terrain[z - 1][x + 1];
      float wz4 = (z1 - 1) * 10;
      
      int sx1, sy1, sx2, sy2, sx3, sy3, sx4, sy4;
      
      if (project3D(wx1, wy1, wz1, &sx1, &sy1) &&
          project3D(wx2, wy2, wz2, &sx2, &sy2) &&
          project3D(wx3, wy3, wz3, &sx3, &sy3) &&
          project3D(wx4, wy4, wz4, &sx4, &sy4)) {
        
        // Calcular color
        float avgHeight = (wy1 + wy2 + wy3 + wy4) / 4.0;
        float distFactor = (float)z / GRID_D;
        
        uint16_t color;
        if (avgHeight > 15) {
          color = palette.peak;
        } else if (avgHeight > 0) {
          color = palette.grid;
        } else {
          color = palette.valley;
        }
        
        // Atenuar por distancia
        if (distFactor > 0.6) {
          color = dimColor(color, (distFactor - 0.6) / 0.4);
        }
        
        // Renderizar según modo
        if (renderMode == FILLED || renderMode == BOTH) {
          // Rellenar triángulos
          uint16_t fillColor = dimColor(color, 0.5);
          fillTriangle(sx1, sy1, sx2, sy2, sx3, sy3, fillColor);
          fillTriangle(sx2, sy2, sx3, sy3, sx4, sy4, fillColor);
        }
        
        if (renderMode == WIREFRAME || renderMode == BOTH) {
          // Dibujar wireframe
          display.drawLine(sx1, sy1, sx2, sy2, color);
          display.drawLine(sx1, sy1, sx3, sy3, color);
          
          if (x % 2 == 0) {
            display.drawLine(sx1, sy1, sx4, sy4, dimColor(color, 0.3));
          }
        }
      }
    }
  }
}

void drawHUD() {
  // Panel semi-transparente
  for (int y = 0; y < 20; y++) {
    for (int x = 0; x < display.width(); x++) {
      if ((x + y) % 2 == 0) {
        display.drawPixel(x, y, 0x0000);
      }
    }
  }
  
  // Texto
  display.setCursor(5, 6);
  display.setTextColor(palette.grid, 0x0000);
  display.setTextSize(1);
  display.print("TRON MOUNTAINS");
  
  display.setCursor(display.width() - 60, 6);
  display.print("FPS:");
  display.print(fps);
  
  // Barra de estado inferior
  display.setCursor(5, display.height() - 15);
  display.setTextColor(palette.peak, 0x0000);
  display.print("SPD:");
  display.print(scrollSpeed, 1);
  
  display.setCursor(80, display.height() - 15);
  display.print("MODE:");
  switch(renderMode) {
    case WIREFRAME: display.print("WIRE"); break;
    case FILLED: display.print("FILL"); break;
    case BOTH: display.print("BOTH"); break;
  }
  
  display.setCursor(display.width() - 90, display.height() - 15);
  switch(currentScheme) {
    case TRON_BLUE: display.print("BLUE"); break;
    case TRON_ORANGE: display.print("ORNG"); break;
    case MATRIX_GREEN: display.print("MTRX"); break;
    case SYNTHWAVE: display.print("SYNTH"); break;
    case CUSTOM: display.print("CSTM"); break;
  }
}

void generateTerrain() {
  for (int z = 0; z < GRID_D; z++) {
    for (int x = 0; x < GRID_W; x++) {
      float height = 0;
      height += sin((x * 0.3 + z * 0.2) * 0.5) * 20;
      height += sin((x * 0.5 - z * 0.3) * 0.8) * 10;
      height += sin((x * 0.8 + z * 0.5) * 1.2) * 5;
      terrain[z][x] = height;
    }
  }
}

void shiftTerrain() {
  for (int z = 0; z < GRID_D - 1; z++) {
    for (int x = 0; x < GRID_W; x++) {
      terrain[z][x] = terrain[z + 1][x];
    }
  }
  
  float offset = (millis() / 1000.0) * 2.0;
  for (int x = 0; x < GRID_W; x++) {
    float height = 0;
    height += sin((x * 0.3 + offset) * 0.5) * 20;
    height += sin((x * 0.5 - offset * 0.5) * 0.8) * 10;
    height += sin((x * 0.8 + offset * 0.3) * 1.2) * 5;
    terrain[GRID_D - 1][x] = height;
  }
}

bool project3D(float x, float y, float z, int* sx, int* sy) {
  float pz = z - camZ;
  if (pz <= 0) return false;
  
  float scale = horizon / pz;
  *sx = display.width() / 2 + (int)(x * scale);
  *sy = display.height() / 2 - (int)((y - camY) * scale);
  
  return (*sx >= 0 && *sx < display.width() && 
          *sy >= 0 && *sy < display.height());
}

uint16_t dimColor(uint16_t color, float factor) {
  uint8_t r = ((color >> 11) & 0x1F) * (1.0 - factor);
  uint8_t g = ((color >> 5) & 0x3F) * (1.0 - factor);
  uint8_t b = (color & 0x1F) * (1.0 - factor);
  return (r << 11) | (g << 5) | b;
}

uint16_t lerpColor(uint16_t c1, uint16_t c2, float t) {
  uint8_t r1 = (c1 >> 11) & 0x1F;
  uint8_t g1 = (c1 >> 5) & 0x3F;
  uint8_t b1 = c1 & 0x1F;
  
  uint8_t r2 = (c2 >> 11) & 0x1F;
  uint8_t g2 = (c2 >> 5) & 0x3F;
  uint8_t b2 = c2 & 0x1F;
  
  uint8_t r = r1 + (r2 - r1) * t;
  uint8_t g = g1 + (g2 - g1) * t;
  uint8_t b = b1 + (b2 - b1) * t;
  
  return (r << 11) | (g << 5) | b;
}

void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
  // Simple triangle fill (puede optimizarse)
  int minX = min(x0, min(x1, x2));
  int maxX = max(x0, max(x1, x2));
  int minY = min(y0, min(y1, y2));
  int maxY = max(y0, max(y1, y2));
  
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      if (pointInTriangle(x, y, x0, y0, x1, y1, x2, y2)) {
        display.drawPixel(x, y, color);
      }
    }
  }
}

bool pointInTriangle(int px, int py, int x0, int y0, int x1, int y1, int x2, int y2) {
  float d1 = sign(px, py, x0, y0, x1, y1);
  float d2 = sign(px, py, x1, y1, x2, y2);
  float d3 = sign(px, py, x2, y2, x0, y0);
  
  bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
  bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);
  
  return !(hasNeg && hasPos);
}

float sign(int px, int py, int x1, int y1, int x2, int y2) {
  return (px - x2) * (y1 - y2) - (x1 - x2) * (py - y2);
}
