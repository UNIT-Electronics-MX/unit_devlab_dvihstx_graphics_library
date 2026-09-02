// TRON Digital World Demo
// Arduino RP2350 + DVI/HDMI
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+, D5(GPIO14)=Data1+, D3(GPIO16)=Data2+

#include <DevLab_DVIHSTX_Graphics.h>

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Inverted Tron colors
#define COLOR_BLACK       (uint16_t)0x0000
#define COLOR_TRON_BLUE   (uint16_t)0x051F  // Deep blue
#define COLOR_TRON_CYAN   (uint16_t)0x07FF  // Bright cyan
#define COLOR_TRON_ORANGE (uint16_t)0xFD20  // Orange
#define COLOR_TRON_WHITE  (uint16_t)0xFFFF  // White
#define COLOR_TRON_GRID   (uint16_t)0x0410  // Dark cyan grid
#define COLOR_HORIZON     (uint16_t)0x0208  // Very dark blue

// World parameters
#define HORIZON_Y       80
#define GRID_SIZE       20
#define MAX_MOUNTAINS   8
#define MAX_PARTICLES   15
#define MAX_LIGHT_CYCLES 3

// Mountain structure
struct Mountain {
  int x;
  int baseWidth;
  int height;
  uint16_t color;
  bool active;
};

// Light cycle structure
struct LightCycle {
  float x, z;
  float speed;
  uint16_t color;
  bool active;
  int trailLength;
  float trailX[20];
  float trailZ[20];
};

// Particle structure
struct Particle {
  float x, y, z;
  uint16_t color;
  bool active;
};

Mountain mountains[MAX_MOUNTAINS];
LightCycle cycles[MAX_LIGHT_CYCLES];
Particle particles[MAX_PARTICLES];

float cameraZ = 0;
float gridOffset = 0;

// Initialize world
void initWorld() {
  // Create mountains
  for (int i = 0; i < MAX_MOUNTAINS; i++) {
    mountains[i].x = random(-150, 150);
    mountains[i].baseWidth = random(30, 80);
    mountains[i].height = random(40, 100);
    mountains[i].color = (i % 2 == 0) ? COLOR_TRON_CYAN : COLOR_TRON_ORANGE;
    mountains[i].active = true;
  }
  
  // Create light cycles
  for (int i = 0; i < MAX_LIGHT_CYCLES; i++) {
    cycles[i].x = random(-100, 100);
    cycles[i].z = random(50, 200);
    cycles[i].speed = random(10, 30) / 10.0;
    cycles[i].color = (i % 2 == 0) ? COLOR_TRON_CYAN : COLOR_TRON_ORANGE;
    cycles[i].active = true;
    cycles[i].trailLength = 0;
  }
  
  // Initialize particles
  for (int i = 0; i < MAX_PARTICLES; i++) {
    particles[i].active = false;
  }
}

// Draw sky gradient
void drawSky() {
  for (int y = 0; y < HORIZON_Y; y++) {
    uint8_t blue = map(y, 0, HORIZON_Y, 0, 31);
    uint16_t color = (uint16_t)((blue));
    display.drawFastHLine(0, y, 320, color);
  }
}

// Draw horizon line
void drawHorizon() {
  display.drawFastHLine(0, HORIZON_Y, 320, COLOR_TRON_CYAN);
  display.drawFastHLine(0, HORIZON_Y + 1, 320, COLOR_TRON_CYAN);
}

// Draw 3D grid floor with perspective
void drawGrid() {
  int centerX = 160;
  
  // Vertical lines (going into distance)
  for (int x = -160; x <= 160; x += GRID_SIZE) {
    int screenX = centerX + x;
    
    // Draw line from horizon to bottom
    for (int z = 0; z < 200; z += 5) {
      float depth = z + gridOffset;
      if (depth < 0) depth += 200;
      
      // Perspective calculation
      float scale = 200.0 / (200.0 + depth);
      int x1 = centerX + (int)(x * scale);
      int y1 = HORIZON_Y + (int)(depth * 0.8 * scale);
      
      if (y1 >= HORIZON_Y && y1 < 240) {
        // Fade with distance
        uint8_t brightness = map(depth, 0, 200, 31, 4);
        uint16_t gridColor = (uint16_t)(brightness);
        display.drawPixel(x1, y1, gridColor);
      }
    }
  }
  
  // Horizontal lines (perpendicular to view)
  for (int z = 0; z < 200; z += GRID_SIZE) {
    float depth = z + gridOffset;
    if (depth < 0) depth += 200;
    
    float scale = 200.0 / (200.0 + depth);
    int y = HORIZON_Y + (int)(depth * 0.8 * scale);
    
    if (y >= HORIZON_Y && y < 240) {
      for (int x = -160; x <= 160; x += 2) {
        int screenX = centerX + (int)(x * scale);
        if (screenX >= 0 && screenX < 320) {
          uint8_t brightness = map(depth, 0, 200, 31, 4);
          uint16_t gridColor = (uint16_t)(brightness);
          display.drawPixel(screenX, y, gridColor);
        }
      }
    }
  }
}

// Draw mountains in background
void drawMountains() {
  for (int i = 0; i < MAX_MOUNTAINS; i++) {
    if (!mountains[i].active) continue;
    
    Mountain* m = &mountains[i];
    
    // Calculate screen position with perspective
    float depth = 300;
    float scale = 200.0 / (200.0 + depth);
    
    int screenX = 160 + (int)(m->x * scale);
    int baseY = HORIZON_Y - 10;
    int peakY = baseY - (int)(m->height * scale);
    int halfWidth = (int)(m->baseWidth * scale / 2);
    
    // Draw mountain outline
    display.drawLine(screenX - halfWidth, baseY, screenX, peakY, m->color);
    display.drawLine(screenX, peakY, screenX + halfWidth, baseY, m->color);
    
    // Draw some fill lines
    for (int y = peakY; y < baseY; y += 3) {
      int width = map(y, peakY, baseY, 0, halfWidth);
      display.drawLine(screenX - width, y, screenX + width, y, 
                      (y % 6 == 0) ? m->color : COLOR_TRON_BLUE);
    }
  }
}

// Draw light cycle
void drawLightCycle(LightCycle* cycle) {
  if (!cycle->active) return;
  
  float depth = cycle->z;
  if (depth < 5) return;
  
  float scale = 200.0 / (200.0 + depth);
  int screenX = 160 + (int)(cycle->x * scale);
  int screenY = HORIZON_Y + (int)(depth * 0.8 * scale);
  
  if (screenY >= HORIZON_Y && screenY < 230) {
    int size = max(2, (int)(8 * scale));
    
    // Draw cycle body
    display.fillRect(screenX - size/2, screenY - size, size, size, cycle->color);
    
    // Draw light trail
    for (int i = 0; i < cycle->trailLength - 1; i++) {
      float z1 = cycle->trailZ[i];
      float z2 = cycle->trailZ[i + 1];
      
      if (z1 > 5 && z2 > 5) {
        float s1 = 200.0 / (200.0 + z1);
        float s2 = 200.0 / (200.0 + z2);
        
        int x1 = 160 + (int)(cycle->trailX[i] * s1);
        int y1 = HORIZON_Y + (int)(z1 * 0.8 * s1);
        int x2 = 160 + (int)(cycle->trailX[i + 1] * s2);
        int y2 = HORIZON_Y + (int)(z2 * 0.8 * s2);
        
        // Fading trail
        uint8_t alpha = map(i, 0, cycle->trailLength, 2, 15);
        uint16_t trailColor = (uint16_t)(alpha | (alpha << 5) | (alpha << 11));
        
        display.drawLine(x1, y1, x2, y2, trailColor);
      }
    }
  }
}

// Update light cycles
void updateLightCycles() {
  for (int i = 0; i < MAX_LIGHT_CYCLES; i++) {
    if (!cycles[i].active) continue;
    
    // Update trail
    if (cycles[i].trailLength < 20) {
      cycles[i].trailLength++;
    }
    
    // Shift trail
    for (int j = cycles[i].trailLength - 1; j > 0; j--) {
      cycles[i].trailX[j] = cycles[i].trailX[j - 1];
      cycles[i].trailZ[j] = cycles[i].trailZ[j - 1];
    }
    
    cycles[i].trailX[0] = cycles[i].x;
    cycles[i].trailZ[0] = cycles[i].z;
    
    // Move cycle
    cycles[i].z -= cycles[i].speed;
    
    // Random movement
    if (random(100) < 5) {
      cycles[i].x += random(-5, 6);
      cycles[i].x = constrain(cycles[i].x, -100, 100);
    }
    
    // Respawn if too close
    if (cycles[i].z < -50) {
      cycles[i].z = random(150, 250);
      cycles[i].x = random(-100, 100);
      cycles[i].trailLength = 0;
    }
  }
}

// Spawn particle
void spawnParticle(float x, float y, float z, uint16_t color) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) {
      particles[i].x = x;
      particles[i].y = y;
      particles[i].z = z;
      particles[i].color = color;
      particles[i].active = true;
      break;
    }
  }
}

// Draw particles
void drawParticles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) continue;
    
    float depth = particles[i].z;
    if (depth < 5 || depth > 300) {
      particles[i].active = false;
      continue;
    }
    
    float scale = 200.0 / (200.0 + depth);
    int screenX = 160 + (int)(particles[i].x * scale);
    int screenY = (int)(particles[i].y - depth * 0.3);
    
    if (screenX >= 0 && screenX < 320 && screenY >= 0 && screenY < 240) {
      display.drawPixel(screenX, screenY, particles[i].color);
      display.drawPixel(screenX + 1, screenY, particles[i].color);
    }
    
    // Update particle
    particles[i].y += 1;
    particles[i].z -= 2;
    
    if (particles[i].y > 240 || particles[i].z < 0) {
      particles[i].active = false;
    }
  }
}

// Draw scan lines effect
void drawScanLines() {
  for (int y = 0; y < 240; y += 4) {
    display.drawFastHLine(0, y, 320, COLOR_TRON_BLUE);
  }
}

// Draw HUD
void drawHUD() {
  // Top bar
  display.drawFastHLine(0, 0, 320, COLOR_TRON_CYAN);
  display.drawFastHLine(0, 1, 320, COLOR_TRON_CYAN);
  
  // Title
  display.setCursor(5, 5);
  display.setTextColor(COLOR_TRON_CYAN, COLOR_BLACK);
  display.setTextSize(1);
  display.print("TRON GRID SYSTEM");
  
  // Speed indicator
  display.setCursor(220, 5);
  display.print("SPEED: ");
  display.print((int)(gridOffset * 10) % 100);
  
  // Corner decorations
  display.drawRect(2, 2, 15, 15, COLOR_TRON_CYAN);
  display.drawRect(303, 2, 15, 15, COLOR_TRON_CYAN);
  display.drawRect(2, 223, 15, 15, COLOR_TRON_CYAN);
  display.drawRect(303, 223, 15, 15, COLOR_TRON_CYAN);
  
  // Center crosshair
  display.drawLine(160, 118, 160, 122, COLOR_TRON_ORANGE);
  display.drawLine(158, 120, 162, 120, COLOR_TRON_ORANGE);
}

// Main render
void renderTronWorld() {
  // Sky
  drawSky();
  
  // Horizon
  drawHorizon();
  
  // Mountains
  drawMountains();
  
  // Grid floor
  drawGrid();
  
  // Light cycles
  for (int i = 0; i < MAX_LIGHT_CYCLES; i++) {
    drawLightCycle(&cycles[i]);
  }
  
  // Particles
  drawParticles();
  
  // HUD overlay
  drawHUD();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  display.setTextSize(1);
  display.setTextWrap(false);
  
  // Intro screen
  display.fillScreen(COLOR_BLACK);
  
  display.setCursor(80, 80);
  display.setTextColor(COLOR_TRON_CYAN, COLOR_BLACK);
  display.setTextSize(2);
  display.print("TRON");
  
  display.setCursor(40, 110);
  display.setTextSize(1);
  display.print("DIGITAL WORLD SYSTEM");
  
  display.setCursor(60, 140);
  display.setTextColor(COLOR_TRON_ORANGE, COLOR_BLACK);
  display.print("Initializing Grid...");
  
  delay(2000);
  
  // Initialize world
  initWorld();
  
  Serial.println("=== TRON Digital World ===");
  Serial.println("Press any key to exit");
  Serial.println("Rendering infinite grid...");
}

void loop() {
  // Check for exit
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'q' || c == 'Q' || c == 27) {
      display.fillScreen(COLOR_BLACK);
      display.setCursor(80, 120);
      display.setTextColor(COLOR_TRON_CYAN, COLOR_BLACK);
      display.print("SYSTEM HALT");
      delay(1000);
      while(1);
    }
  }
  
  // Render world
  renderTronWorld();
  
  // Update animations
  gridOffset += 1.5;
  if (gridOffset >= GRID_SIZE) {
    gridOffset -= GRID_SIZE;
  }
  
  cameraZ += 1.0;
  
  // Update light cycles
  updateLightCycles();
  
  // Spawn random particles
  if (random(100) < 5) {
    spawnParticle(random(-100, 100), HORIZON_Y + 20, random(50, 150), 
                  random(2) ? COLOR_TRON_CYAN : COLOR_TRON_ORANGE);
  }
  
  delay(33);  // ~30 FPS
}
