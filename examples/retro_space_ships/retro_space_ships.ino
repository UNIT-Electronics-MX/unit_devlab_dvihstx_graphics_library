// Retro Space Ships - Vector Graphics Demo
// Arduino RP2350 + DVI/HDMI
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+, D5(GPIO14)=Data1+, D3(GPIO16)=Data2+
// Style: Wireframe vector graphics like Asteroids/Battlezone era

#include <UDVI_HSTX.h>

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Inverted retro colors
#define COLOR_BLACK       (uint16_t)0x0000
#define COLOR_WHITE       (uint16_t)0xFFFF
#define COLOR_GREEN       (uint16_t)0x07E0  // Classic vector green
#define COLOR_CYAN        (uint16_t)0x07FF
#define COLOR_RED         (uint16_t)0xF800
#define COLOR_YELLOW      (uint16_t)0xFFE0
#define COLOR_MAGENTA     (uint16_t)0xF81F

#define MAX_STARS       80
#define MAX_SHIPS       8
#define MAX_PROJECTILES 20
#define MAX_DEBRIS      15

// Star field
struct Star {
  float x, y, z;
  uint16_t color;
};

// Space ship structure (wireframe)
struct SpaceShip {
  float x, y, z;
  float rotX, rotY, rotZ;
  float speedX, speedY, speedZ;
  float rotSpeedY;
  uint16_t color;
  int shipType;  // 0=fighter, 1=cruiser, 2=bomber
  bool active;
};

// Projectile
struct Projectile {
  float x, y, z;
  float vx, vy, vz;
  uint16_t color;
  int life;
  bool active;
};

// Debris particles
struct Debris {
  float x, y, z;
  float vx, vy, vz;
  uint16_t color;
  int life;
  bool active;
};

Star stars[MAX_STARS];
SpaceShip ships[MAX_SHIPS];
Projectile projectiles[MAX_PROJECTILES];
Debris debris[MAX_DEBRIS];

float cameraZ = 0;

// Fighter ship vertices (triangular)
const int fighterVerts = 7;
float fighterModel[fighterVerts][3] = {
  {0, -8, 0},    // Nose
  {-6, 4, -2},   // Left wing back
  {6, 4, -2},    // Right wing back
  {-3, 4, 2},    // Left wing front
  {3, 4, 2},     // Right wing front
  {0, 0, 4},     // Cockpit
  {0, 4, 0}      // Tail
};

const int fighterEdges = 10;
int fighterLines[fighterEdges][2] = {
  {0,3}, {0,4}, {3,1}, {4,2}, {1,6}, {2,6},
  {3,5}, {4,5}, {5,6}, {1,2}
};

// Cruiser ship (larger, box-like)
const int cruiserVerts = 8;
float cruiserModel[cruiserVerts][3] = {
  {-4, -4, -8},  // Front face
  {4, -4, -8},
  {4, 4, -8},
  {-4, 4, -8},
  {-6, -4, 8},   // Back face (wider)
  {6, -4, 8},
  {6, 4, 8},
  {-6, 4, 8}
};

const int cruiserEdges = 12;
int cruiserLines[cruiserEdges][2] = {
  {0,1}, {1,2}, {2,3}, {3,0},  // Front face
  {4,5}, {5,6}, {6,7}, {7,4},  // Back face
  {0,4}, {1,5}, {2,6}, {3,7}   // Connecting edges
};

// Initialize star field
void initStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x = random(-200, 200);
    stars[i].y = random(-120, 120);
    stars[i].z = random(10, 400);
    
    // Different star colors
    int colorType = random(100);
    if (colorType < 70) stars[i].color = COLOR_WHITE;
    else if (colorType < 85) stars[i].color = COLOR_CYAN;
    else stars[i].color = COLOR_YELLOW;
  }
}

// Initialize ships
void initShips() {
  for (int i = 0; i < MAX_SHIPS; i++) {
    ships[i].x = random(-150, 150);
    ships[i].y = random(-80, 80);
    ships[i].z = random(50, 300);
    ships[i].rotX = 0;
    ships[i].rotY = random(0, 360) * 0.017453; // to radians
    ships[i].rotZ = 0;
    ships[i].speedX = random(-10, 10) / 10.0;
    ships[i].speedY = random(-10, 10) / 10.0;
    ships[i].speedZ = random(-15, -5) / 10.0;
    ships[i].rotSpeedY = random(-5, 5) / 100.0;
    ships[i].shipType = random(3);  // 0, 1, or 2
    ships[i].active = true;
    
    // Color by type
    if (ships[i].shipType == 0) ships[i].color = COLOR_GREEN;
    else if (ships[i].shipType == 1) ships[i].color = COLOR_CYAN;
    else ships[i].color = COLOR_YELLOW;
  }
  
  for (int i = 0; i < MAX_PROJECTILES; i++) {
    projectiles[i].active = false;
  }
  
  for (int i = 0; i < MAX_DEBRIS; i++) {
    debris[i].active = false;
  }
}

// Draw stars with parallax
void drawStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    // Move star toward camera
    stars[i].z -= 2;
    
    // Respawn if passed camera
    if (stars[i].z < 1) {
      stars[i].z = 400;
      stars[i].x = random(-200, 200);
      stars[i].y = random(-120, 120);
    }
    
    // Project to screen
    float scale = 200.0 / stars[i].z;
    int screenX = 160 + (int)(stars[i].x * scale);
    int screenY = 120 + (int)(stars[i].y * scale);
    
    if (screenX >= 0 && screenX < 320 && screenY >= 0 && screenY < 240) {
      // Size based on depth
      int size = constrain((int)(3 * scale), 1, 3);
      
      if (size == 1) {
        display.drawPixel(screenX, screenY, stars[i].color);
      } else if (size == 2) {
        display.fillRect(screenX, screenY, 2, 2, stars[i].color);
      } else {
        display.drawPixel(screenX, screenY, stars[i].color);
        display.drawPixel(screenX-1, screenY, stars[i].color);
        display.drawPixel(screenX+1, screenY, stars[i].color);
        display.drawPixel(screenX, screenY-1, stars[i].color);
        display.drawPixel(screenX, screenY+1, stars[i].color);
      }
    }
  }
}

// Rotate point in 3D
void rotatePoint(float* x, float* y, float* z, float rx, float ry, float rz) {
  float tempX, tempY, tempZ;
  
  // Rotate around Y axis
  if (ry != 0) {
    tempX = *x * cos(ry) - *z * sin(ry);
    tempZ = *x * sin(ry) + *z * cos(ry);
    *x = tempX;
    *z = tempZ;
  }
  
  // Rotate around X axis
  if (rx != 0) {
    tempY = *y * cos(rx) - *z * sin(rx);
    tempZ = *y * sin(rx) + *z * cos(rx);
    *y = tempY;
    *z = tempZ;
  }
  
  // Rotate around Z axis
  if (rz != 0) {
    tempX = *x * cos(rz) - *y * sin(rz);
    tempY = *x * sin(rz) + *y * cos(rz);
    *x = tempX;
    *y = tempY;
  }
}

// Draw wireframe ship
void drawShip(SpaceShip* ship) {
  if (!ship->active) return;
  if (ship->z < 5 || ship->z > 400) return;
  
  float vertices[8][3];  // Max vertices
  int numVerts;
  int numEdges;
  int (*edges)[2];
  float (*model)[3];
  
  // Select model
  if (ship->shipType == 0) {  // Fighter
    numVerts = fighterVerts;
    numEdges = fighterEdges;
    edges = fighterLines;
    model = fighterModel;
  } else {  // Cruiser or Bomber (use cruiser model)
    numVerts = cruiserVerts;
    numEdges = cruiserEdges;
    edges = cruiserLines;
    model = cruiserModel;
  }
  
  // Transform vertices
  for (int i = 0; i < numVerts; i++) {
    float x = model[i][0];
    float y = model[i][1];
    float z = model[i][2];
    
    // Apply rotation
    rotatePoint(&x, &y, &z, ship->rotX, ship->rotY, ship->rotZ);
    
    // Translate to ship position
    vertices[i][0] = x + ship->x;
    vertices[i][1] = y + ship->y;
    vertices[i][2] = z + ship->z;
  }
  
  // Project and draw edges
  for (int i = 0; i < numEdges; i++) {
    int v1 = edges[i][0];
    int v2 = edges[i][1];
    
    float z1 = vertices[v1][2];
    float z2 = vertices[v2][2];
    
    if (z1 > 5 && z2 > 5) {
      float scale1 = 200.0 / z1;
      float scale2 = 200.0 / z2;
      
      int x1 = 160 + (int)(vertices[v1][0] * scale1);
      int y1 = 120 + (int)(vertices[v1][1] * scale1);
      int x2 = 160 + (int)(vertices[v2][0] * scale2);
      int y2 = 120 + (int)(vertices[v2][1] * scale2);
      
      display.drawLine(x1, y1, x2, y2, ship->color);
    }
  }
}

// Update ships
void updateShips() {
  for (int i = 0; i < MAX_SHIPS; i++) {
    if (!ships[i].active) continue;
    
    // Update position
    ships[i].x += ships[i].speedX;
    ships[i].y += ships[i].speedY;
    ships[i].z += ships[i].speedZ;
    
    // Update rotation
    ships[i].rotY += ships[i].rotSpeedY;
    
    // Wrap around
    if (ships[i].x < -200) ships[i].x = 200;
    if (ships[i].x > 200) ships[i].x = -200;
    if (ships[i].y < -150) ships[i].y = 150;
    if (ships[i].y > 150) ships[i].y = -150;
    
    // Respawn if too close or far
    if (ships[i].z < -20) {
      ships[i].z = 350;
      ships[i].x = random(-150, 150);
      ships[i].y = random(-80, 80);
    }
    
    // Random projectile fire
    if (random(1000) < 5) {
      fireProjectile(ships[i].x, ships[i].y, ships[i].z, ships[i].color);
    }
  }
}

// Fire projectile
void fireProjectile(float x, float y, float z, uint16_t color) {
  for (int i = 0; i < MAX_PROJECTILES; i++) {
    if (!projectiles[i].active) {
      projectiles[i].x = x;
      projectiles[i].y = y;
      projectiles[i].z = z;
      projectiles[i].vx = random(-5, 5) / 10.0;
      projectiles[i].vy = random(-5, 5) / 10.0;
      projectiles[i].vz = -3;
      projectiles[i].color = color;
      projectiles[i].life = 60;
      projectiles[i].active = true;
      break;
    }
  }
}

// Draw and update projectiles
void updateProjectiles() {
  for (int i = 0; i < MAX_PROJECTILES; i++) {
    if (!projectiles[i].active) continue;
    
    // Update
    projectiles[i].x += projectiles[i].vx;
    projectiles[i].y += projectiles[i].vy;
    projectiles[i].z += projectiles[i].vz;
    projectiles[i].life--;
    
    if (projectiles[i].life <= 0 || projectiles[i].z < 0) {
      projectiles[i].active = false;
      continue;
    }
    
    // Draw
    if (projectiles[i].z > 5) {
      float scale = 200.0 / projectiles[i].z;
      int screenX = 160 + (int)(projectiles[i].x * scale);
      int screenY = 120 + (int)(projectiles[i].y * scale);
      
      if (screenX >= 0 && screenX < 320 && screenY >= 0 && screenY < 240) {
        int size = max(1, (int)(3 * scale));
        display.fillRect(screenX-size/2, screenY-size/2, size, size, projectiles[i].color);
      }
    }
  }
}

// Spawn debris
void spawnDebris(float x, float y, float z, uint16_t color) {
  for (int i = 0; i < MAX_DEBRIS; i++) {
    if (!debris[i].active) {
      debris[i].x = x;
      debris[i].y = y;
      debris[i].z = z;
      debris[i].vx = random(-20, 20) / 10.0;
      debris[i].vy = random(-20, 20) / 10.0;
      debris[i].vz = random(-10, 10) / 10.0;
      debris[i].color = color;
      debris[i].life = 30;
      debris[i].active = true;
      break;
    }
  }
}

// Update debris
void updateDebris() {
  for (int i = 0; i < MAX_DEBRIS; i++) {
    if (!debris[i].active) continue;
    
    debris[i].x += debris[i].vx;
    debris[i].y += debris[i].vy;
    debris[i].z += debris[i].vz;
    debris[i].life--;
    
    if (debris[i].life <= 0) {
      debris[i].active = false;
      continue;
    }
    
    // Draw
    if (debris[i].z > 5 && debris[i].z < 300) {
      float scale = 200.0 / debris[i].z;
      int screenX = 160 + (int)(debris[i].x * scale);
      int screenY = 120 + (int)(debris[i].y * scale);
      
      if (screenX >= 1 && screenX < 319 && screenY >= 1 && screenY < 239) {
        display.drawLine(screenX-1, screenY-1, screenX+1, screenY+1, debris[i].color);
        display.drawLine(screenX-1, screenY+1, screenX+1, screenY-1, debris[i].color);
      }
    }
  }
}

// Draw HUD (retro vector style)
void drawHUD() {
  // Frame
  display.drawRect(0, 0, 320, 240, COLOR_GREEN);
  display.drawRect(1, 1, 318, 238, COLOR_GREEN);
  
  // Title
  display.setCursor(5, 5);
  display.setTextColor(COLOR_GREEN, COLOR_BLACK);
  display.setTextSize(1);
  display.print("VECTOR SPACE");
  
  // Ship count
  int activeShips = 0;
  for (int i = 0; i < MAX_SHIPS; i++) {
    if (ships[i].active) activeShips++;
  }
  
  display.setCursor(230, 5);
  display.print("SHIPS:");
  display.print(activeShips);
  
  // Corner brackets
  display.drawLine(10, 10, 20, 10, COLOR_GREEN);
  display.drawLine(10, 10, 10, 20, COLOR_GREEN);
  
  display.drawLine(300, 10, 310, 10, COLOR_GREEN);
  display.drawLine(310, 10, 310, 20, COLOR_GREEN);
  
  display.drawLine(10, 230, 20, 230, COLOR_GREEN);
  display.drawLine(10, 230, 10, 220, COLOR_GREEN);
  
  display.drawLine(300, 230, 310, 230, COLOR_GREEN);
  display.drawLine(310, 230, 310, 220, COLOR_GREEN);
  
  // Crosshair
  display.drawLine(158, 120, 162, 120, COLOR_RED);
  display.drawLine(160, 118, 160, 122, COLOR_RED);
  display.drawRect(155, 115, 10, 10, COLOR_RED);
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
  
  // Intro
  display.fillScreen(COLOR_BLACK);
  
  display.setCursor(60, 80);
  display.setTextColor(COLOR_GREEN, COLOR_BLACK);
  display.setTextSize(2);
  display.print("VECTOR SPACE");
  
  display.setCursor(50, 120);
  display.setTextSize(1);
  display.print("Retro Wireframe Graphics");
  
  display.setCursor(80, 150);
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.print("Initializing...");
  
  delay(2000);
  
  initStars();
  initShips();
  
  Serial.println("=== Vector Space Demo ===");
  Serial.println("Retro wireframe graphics");
  Serial.println("Press 'q' to quit");
  Serial.println("Press 'e' for explosion");
}

void loop() {
  // Check for commands
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'q' || c == 'Q') {
      display.fillScreen(COLOR_BLACK);
      display.setCursor(100, 120);
      display.setTextColor(COLOR_GREEN, COLOR_BLACK);
      display.print("GAME OVER");
      while(1);
    }
    if (c == 'e' || c == 'E') {
      // Create explosion
      for (int i = 0; i < 10; i++) {
        spawnDebris(0, 0, 100, random(2) ? COLOR_YELLOW : COLOR_RED);
      }
    }
  }
  
  // Clear screen
  display.fillScreen(COLOR_BLACK);
  
  // Draw everything
  drawStars();
  
  updateShips();
  for (int i = 0; i < MAX_SHIPS; i++) {
    drawShip(&ships[i]);
  }
  
  updateProjectiles();
  updateDebris();
  
  drawHUD();
  
  delay(33);  // ~30 FPS
}
