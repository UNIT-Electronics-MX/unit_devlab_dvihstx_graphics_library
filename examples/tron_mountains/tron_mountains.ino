// TRON Mountains - Paisaje 3D en movimiento
// Arduino Nano RP2350 + DVI
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+(Blue), D5(GPIO14)=Data1+(Green), D3(GPIO16)=Data2+(Red)
// Efecto de montañas wireframe estilo Tron/retro

#include <UDVI_HSTX.h>

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Configuración del terreno
const int GRID_W = 32;  // Ancho de la malla
const int GRID_D = 32;  // Profundidad de la malla
float terrain[GRID_D][GRID_W];

// Parámetros de cámara y proyección
float camX = 0;
float camY = 80;   // Altura de la cámara
float camZ = -50;  // Posición Z
float horizon = 100;

// Offset de animación
float scrollOffset = 0;

// Colores estilo Tron
uint16_t colorHorizon = 0x001F;  // Azul oscuro
uint16_t colorGrid = 0x07FF;     // Cyan brillante
uint16_t colorPeak = 0xF81F;     // Magenta para picos

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║     TRON MOUNTAINS - Nano RP2350      ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  Serial.println("✓ Display inicializado");
  Serial.println("✓ Generando terreno...");
  
  // Generar terreno inicial
  generateTerrain();
  
  Serial.println("✓ Listo! Iniciando viaje...");
  Serial.println();
  
  display.fillScreen(0x0000);
}

void loop() {
  // Limpiar pantalla con gradiente (cielo)
  drawSky();
  
  // Actualizar terreno (scroll infinito)
  scrollOffset += 0.3;
  if (scrollOffset >= 1.0) {
    scrollOffset -= 1.0;
    shiftTerrain();
  }
  
  // Dibujar malla 3D
  drawTerrain();
  
  // Info en pantalla
  display.setCursor(5, 5);
  display.setTextColor(colorGrid, 0x0000);
  display.setTextSize(1);
  display.print("TRON MOUNTAINS");
  
  display.setCursor(5, 225);
  display.setTextColor(0x07E0, 0x0000);
  display.print("SPEED: ");
  display.print((int)(scrollOffset * 100));
  display.print("%");
  
  delay(30); // ~33 FPS
}

void drawSky() {
  // Gradiente de cielo estilo Tron
  for (int y = 0; y < display.height() / 2; y++) {
    uint8_t blue = map(y, 0, display.height() / 2, 0, 31);
    uint16_t color = blue;  // Solo canal azul
    display.drawFastHLine(0, y, display.width(), color);
  }
  
  // Horizonte con líneas de grid
  for (int y = display.height() / 2; y < display.height(); y++) {
    // Grid horizontal cada 10 píxeles
    if ((y - display.height() / 2) % 10 == 0) {
      display.drawFastHLine(0, y, display.width(), 0x0010);
    } else {
      display.drawFastHLine(0, y, display.width(), 0x0000);
    }
  }
}

void generateTerrain() {
  for (int z = 0; z < GRID_D; z++) {
    for (int x = 0; x < GRID_W; x++) {
      // Generar altura con múltiples ondas
      float height = 0;
      
      // Onda principal
      height += sin((x * 0.3 + z * 0.2) * 0.5) * 20;
      
      // Ondas secundarias
      height += sin((x * 0.5 - z * 0.3) * 0.8) * 10;
      height += sin((x * 0.8 + z * 0.5) * 1.2) * 5;
      
      // Ruido adicional
      height += (random(100) - 50) * 0.1;
      
      terrain[z][x] = height;
    }
  }
}

void shiftTerrain() {
  // Mover todas las filas hacia adelante
  for (int z = 0; z < GRID_D - 1; z++) {
    for (int x = 0; x < GRID_W; x++) {
      terrain[z][x] = terrain[z + 1][x];
    }
  }
  
  // Generar nueva fila al final
  for (int x = 0; x < GRID_W; x++) {
    float offset = (millis() / 1000.0) * 2.0;
    
    float height = 0;
    height += sin((x * 0.3 + offset) * 0.5) * 20;
    height += sin((x * 0.5 - offset * 0.5) * 0.8) * 10;
    height += sin((x * 0.8 + offset * 0.3) * 1.2) * 5;
    height += (random(100) - 50) * 0.1;
    
    terrain[GRID_D - 1][x] = height;
  }
}

void drawTerrain() {
  int centerX = display.width() / 2;
  int centerY = display.height() / 2 + 20;
  
  // Dibujar la malla de atrás hacia adelante
  for (int z = GRID_D - 1; z > 0; z--) {
    for (int x = 0; x < GRID_W - 1; x++) {
      // Calcular posición 3D con interpolación para scroll suave
      float z1 = z + scrollOffset;
      float z2 = z + scrollOffset;
      float z3 = z - 1 + scrollOffset;
      float z4 = z - 1 + scrollOffset;
      
      // Vértices del quad
      float x1 = (x - GRID_W / 2) * 10;
      float y1 = terrain[z][x];
      
      float x2 = (x + 1 - GRID_W / 2) * 10;
      float y2 = terrain[z][x + 1];
      
      float x3 = (x - GRID_W / 2) * 10;
      float y3 = terrain[z - 1][x];
      
      float x4 = (x + 1 - GRID_W / 2) * 10;
      float y4 = terrain[z - 1][x + 1];
      
      // Proyectar a 2D
      int sx1, sy1, sx2, sy2, sx3, sy3, sx4, sy4;
      
      if (project3D(x1, y1, z1 * 10, &sx1, &sy1) &&
          project3D(x2, y2, z2 * 10, &sx2, &sy2) &&
          project3D(x3, y3, z3 * 10, &sx3, &sy3) &&
          project3D(x4, y4, z4 * 10, &sx4, &sy4)) {
        
        // Color basado en altura y distancia
        float avgHeight = (y1 + y2 + y3 + y4) / 4.0;
        uint16_t color;
        
        if (avgHeight > 15) {
          color = colorPeak;  // Magenta para picos altos
        } else if (avgHeight > 5) {
          color = colorGrid;  // Cyan para altura media
        } else {
          color = 0x0410;     // Cyan oscuro para valles
        }
        
        // Atenuar por distancia
        float distFactor = (float)z / GRID_D;
        if (distFactor > 0.7) {
          color = dimColor(color, distFactor);
        }
        
        // Dibujar líneas del wireframe
        display.drawLine(sx1, sy1, sx2, sy2, color);  // Horizontal
        display.drawLine(sx1, sy1, sx3, sy3, color);  // Vertical
        
        // Dibujar diagonales cada 4 cuadros para detalle
        if (x % 4 == 0 && z % 4 == 0) {
          display.drawLine(sx1, sy1, sx4, sy4, dimColor(color, 0.5));
        }
      }
    }
  }
  
  // Dibujar líneas verticales de grid en el horizonte
  for (int x = 0; x < GRID_W; x += 4) {
    int sx1, sy1, sx2, sy2;
    float worldX = (x - GRID_W / 2) * 10;
    
    if (project3D(worldX, 0, GRID_D * 10, &sx1, &sy1) &&
        project3D(worldX, -50, GRID_D * 10, &sx2, &sy2)) {
      display.drawLine(sx1, sy1, sx2, sy2, 0x0208);
    }
  }
}

bool project3D(float x, float y, float z, int* sx, int* sy) {
  // Transformar al espacio de cámara
  float px = x - camX;
  float py = y - camY;
  float pz = z - camZ;
  
  // Verificar que está delante de la cámara
  if (pz <= 0) return false;
  
  // Proyección en perspectiva
  float scale = horizon / pz;
  
  *sx = display.width() / 2 + (int)(px * scale);
  *sy = display.height() / 2 - (int)(py * scale);
  
  // Verificar que está en pantalla
  if (*sx < 0 || *sx >= display.width() || 
      *sy < 0 || *sy >= display.height()) {
    return false;
  }
  
  return true;
}

uint16_t dimColor(uint16_t color, float factor) {
  // Reducir brillo multiplicando cada componente
  uint8_t r = ((color >> 11) & 0x1F) * (1.0 - factor);
  uint8_t g = ((color >> 5) & 0x3F) * (1.0 - factor);
  uint8_t b = (color & 0x1F) * (1.0 - factor);
  
  return (r << 11) | (g << 5) | b;
}
