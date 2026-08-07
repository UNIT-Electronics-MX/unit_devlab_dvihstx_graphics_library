// Universe Simulator - Planets and Space
// Arduino RP2350 + DVI/HDMI
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+, D5(GPIO14)=Data1+, D3(GPIO16)=Data2+
// Style: Retro space simulator with planets, moons, and stars

#include <UDVI_HSTX.h>

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_YELLOW      0xFFE0
#define COLOR_RED         0xF800
#define COLOR_BLUE        0x001F
#define COLOR_GREEN       0x07E0
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F
#define COLOR_ORANGE      0xFD20
#define COLOR_PURPLE      0x780F
#define COLOR_BROWN       0xA145
#define COLOR_GRAY        0x7BEF

#define MAX_STARS       100
#define MAX_PLANETS     8
#define MAX_MOONS       12
#define MAX_ASTEROIDS   15

// Star field
struct Star {
  float x, y, z;
  uint16_t color;
  uint8_t brightness;
};

// Planet structure
struct Planet {
  float x, y, z;            // Position
  float orbitRadius;        // Distance from center
  float orbitSpeed;         // Orbital speed
  float orbitAngle;         // Current angle
  float rotationAngle;      // Self rotation
  float rotationSpeed;      // Rotation speed
  int radius;               // Visual size
  uint16_t color;           // Planet color
  uint16_t ringColor;       // Ring color (0 = no ring)
  bool hasRing;
  bool hasMoons;
  int numMoons;
  const char* name;
  bool active;
};

// Moon structure
struct Moon {
  int parentPlanet;         // Which planet it orbits
  float orbitRadius;
  float orbitSpeed;
  float orbitAngle;
  int radius;
  uint16_t color;
  bool active;
};

// Asteroid
struct Asteroid {
  float x, y, z;
  float vx, vy, vz;
  int size;
  float rotAngle;
  uint16_t color;
  bool active;
};

Star stars[MAX_STARS];
Planet planets[MAX_PLANETS];
Moon moons[MAX_MOONS];
Asteroid asteroids[MAX_ASTEROIDS];

float cameraX = 0, cameraY = 0, cameraZ = -200;
int viewMode = 0;  // 0=orbit view, 1=planet view, 2=explore
bool firstFrame = true;

// Initialize stars
void initStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x = random(-400, 400);
    stars[i].y = random(-300, 300);
    stars[i].z = random(-500, 500);
    stars[i].brightness = random(50, 255);
    
    int colorType = random(100);
    if (colorType < 60) stars[i].color = COLOR_WHITE;
    else if (colorType < 80) stars[i].color = COLOR_CYAN;
    else if (colorType < 90) stars[i].color = COLOR_YELLOW;
    else stars[i].color = COLOR_RED;
  }
}

// Initialize planets (solar system style)
void initPlanets() {
  // Sun (center)
  planets[0].orbitRadius = 0;
  planets[0].orbitSpeed = 0;
  planets[0].orbitAngle = 0;
  planets[0].rotationSpeed = 0.02;
  planets[0].radius = 25;
  planets[0].color = COLOR_YELLOW;
  planets[0].hasRing = false;
  planets[0].hasMoons = false;
  planets[0].name = "SUN";
  planets[0].active = true;
  planets[0].x = 0;
  planets[0].y = 0;
  planets[0].z = 0;
  
  // Mercury
  planets[1].orbitRadius = 60;
  planets[1].orbitSpeed = 0.04;
  planets[1].radius = 4;
  planets[1].rotationSpeed = 0.01;
  planets[1].color = COLOR_GRAY;
  planets[1].hasRing = false;
  planets[1].hasMoons = false;
  planets[1].name = "MERCURY";
  planets[1].active = true;
  
  // Venus
  planets[2].orbitRadius = 90;
  planets[2].orbitSpeed = 0.03;
  planets[2].radius = 8;
  planets[2].rotationSpeed = 0.015;
  planets[2].color = COLOR_ORANGE;
  planets[2].hasRing = false;
  planets[2].hasMoons = false;
  planets[2].name = "VENUS";
  planets[2].active = true;
  
  // Earth
  planets[3].orbitRadius = 120;
  planets[3].orbitSpeed = 0.025;
  planets[3].radius = 9;
  planets[3].rotationSpeed = 0.03;
  planets[3].color = COLOR_CYAN;
  planets[3].hasRing = false;
  planets[3].hasMoons = true;
  planets[3].numMoons = 1;
  planets[3].name = "EARTH";
  planets[3].active = true;
  
  // Mars
  planets[4].orbitRadius = 150;
  planets[4].orbitSpeed = 0.02;
  planets[4].radius = 6;
  planets[4].rotationSpeed = 0.025;
  planets[4].color = COLOR_RED;
  planets[4].hasRing = false;
  planets[4].hasMoons = true;
  planets[4].numMoons = 2;
  planets[4].name = "MARS";
  planets[4].active = true;
  
  // Jupiter
  planets[5].orbitRadius = 200;
  planets[5].orbitSpeed = 0.015;
  planets[5].radius = 18;
  planets[5].rotationSpeed = 0.04;
  planets[5].color = COLOR_BROWN;
  planets[5].hasRing = false;
  planets[5].hasMoons = true;
  planets[5].numMoons = 4;
  planets[5].name = "JUPITER";
  planets[5].active = true;
  
  // Saturn
  planets[6].orbitRadius = 260;
  planets[6].orbitSpeed = 0.012;
  planets[6].radius = 16;
  planets[6].rotationSpeed = 0.035;
  planets[6].color = COLOR_YELLOW;
  planets[6].hasRing = true;
  planets[6].ringColor = COLOR_ORANGE;
  planets[6].hasMoons = true;
  planets[6].numMoons = 3;
  planets[6].name = "SATURN";
  planets[6].active = true;
  
  // Neptune
  planets[7].orbitRadius = 300;
  planets[7].orbitSpeed = 0.01;
  planets[7].radius = 12;
  planets[7].rotationSpeed = 0.03;
  planets[7].color = COLOR_BLUE;
  planets[7].hasRing = false;
  planets[7].hasMoons = true;
  planets[7].numMoons = 2;
  planets[7].name = "NEPTUNE";
  planets[7].active = true;
  
  // Initialize all angles
  for (int i = 0; i < MAX_PLANETS; i++) {
    planets[i].orbitAngle = random(0, 360) * 0.017453;
    planets[i].rotationAngle = 0;
  }
}

// Initialize moons
void initMoons() {
  int moonIndex = 0;
  
  for (int i = 0; i < MAX_PLANETS; i++) {
    if (planets[i].hasMoons) {
      for (int m = 0; m < planets[i].numMoons; m++) {
        if (moonIndex < MAX_MOONS) {
          moons[moonIndex].parentPlanet = i;
          moons[moonIndex].orbitRadius = planets[i].radius + 15 + (m * 8);
          moons[moonIndex].orbitSpeed = 0.05 + (m * 0.02);
          moons[moonIndex].orbitAngle = random(0, 360) * 0.017453;
          moons[moonIndex].radius = 2 + random(0, 2);
          moons[moonIndex].color = COLOR_GRAY;
          moons[moonIndex].active = true;
          moonIndex++;
        }
      }
    }
  }
}

// Initialize asteroids
void initAsteroids() {
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    // Asteroid belt between Mars and Jupiter
    float angle = random(0, 360) * 0.017453;
    float distance = random(170, 190);
    
    asteroids[i].x = cos(angle) * distance;
    asteroids[i].z = sin(angle) * distance;
    asteroids[i].y = random(-10, 10);
    asteroids[i].vx = random(-5, 5) / 100.0;
    asteroids[i].vy = random(-5, 5) / 100.0;
    asteroids[i].vz = random(-5, 5) / 100.0;
    asteroids[i].size = random(1, 4);
    asteroids[i].rotAngle = 0;
    asteroids[i].color = COLOR_BROWN;
    asteroids[i].active = true;
  }
}

// Update planet positions
void updatePlanets() {
  for (int i = 1; i < MAX_PLANETS; i++) {  // Skip sun
    if (!planets[i].active) continue;
    
    // Update orbit
    planets[i].orbitAngle += planets[i].orbitSpeed;
    if (planets[i].orbitAngle > 6.28) planets[i].orbitAngle -= 6.28;
    
    // Calculate position
    planets[i].x = cos(planets[i].orbitAngle) * planets[i].orbitRadius;
    planets[i].z = sin(planets[i].orbitAngle) * planets[i].orbitRadius;
    planets[i].y = 0;
    
    // Update rotation
    planets[i].rotationAngle += planets[i].rotationSpeed;
    if (planets[i].rotationAngle > 6.28) planets[i].rotationAngle -= 6.28;
  }
}

// Update moons
void updateMoons() {
  for (int i = 0; i < MAX_MOONS; i++) {
    if (!moons[i].active) continue;
    
    moons[i].orbitAngle += moons[i].orbitSpeed;
    if (moons[i].orbitAngle > 6.28) moons[i].orbitAngle -= 6.28;
  }
}

// Update asteroids
void updateAsteroids() {
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (!asteroids[i].active) continue;
    
    asteroids[i].x += asteroids[i].vx;
    asteroids[i].y += asteroids[i].vy;
    asteroids[i].z += asteroids[i].vz;
    asteroids[i].rotAngle += 0.05;
  }
}

// Draw stars
void drawStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    float dx = stars[i].x - cameraX;
    float dy = stars[i].y - cameraY;
    float dz = stars[i].z - cameraZ;
    
    if (dz <= 0) continue;
    
    float scale = 200.0 / dz;
    int screenX = 160 + (int)(dx * scale);
    int screenY = 120 + (int)(dy * scale);
    
    if (screenX >= 0 && screenX < 320 && screenY >= 0 && screenY < 240) {
      uint8_t bright = map(dz, 0, 500, stars[i].brightness, 50);
      display.drawPixel(screenX, screenY, stars[i].color);
      
      if (bright > 200 && random(100) < 10) {
        display.drawPixel(screenX + 1, screenY, stars[i].color);
      }
    }
  }
}

// Draw filled circle with shading
void drawPlanet(int cx, int cy, int radius, uint16_t color) {
  // Draw filled circle
  for (int r = 0; r <= radius; r++) {
    int h = sqrt(radius * radius - r * r);
    // Fill with gradient
    for (int x = -h; x <= h; x++) {
      // Simple shading based on x position
      if (x < -h/3) {
        display.drawPixel(cx + x, cy + r, COLOR_BLACK);
        display.drawPixel(cx + x, cy - r, COLOR_BLACK);
      } else {
        display.drawPixel(cx + x, cy + r, color);
        display.drawPixel(cx + x, cy - r, color);
      }
    }
  }
  
  // Highlight
  int hlSize = radius / 4;
  for (int r = 0; r <= hlSize; r++) {
    int h = sqrt(hlSize * hlSize - r * r);
    for (int x = -h; x <= h; x++) {
      display.drawPixel(cx - radius/3 + x, cy - radius/3 + r, COLOR_WHITE);
      display.drawPixel(cx - radius/3 + x, cy - radius/3 - r, COLOR_WHITE);
    }
  }
}

// Draw planet with rings
void drawPlanetWithRing(int cx, int cy, int radius, uint16_t planetColor, uint16_t ringColor) {
  // Draw rings first (behind)
  int ringInner = radius + 3;
  int ringOuter = radius + 8;
  
  for (int r = ringInner; r <= ringOuter; r++) {
    display.drawCircle(cx, cy, r, ringColor);
  }
  
  // Draw planet
  drawPlanet(cx, cy, radius, planetColor);
}

// Draw planets and moons
void drawPlanets() {
  // Draw orbit lines (optional, thin)
  for (int i = 1; i < MAX_PLANETS; i++) {
    if (!planets[i].active) continue;
    
    float radius = planets[i].orbitRadius;
    float scale = 200.0 / abs(cameraZ);
    int screenRadius = (int)(radius * scale);
    
    if (screenRadius > 0 && screenRadius < 400) {
      for (int angle = 0; angle < 360; angle += 10) {
        float rad = angle * 0.017453;
        float x1 = cos(rad) * radius;
        float z1 = sin(rad) * radius;
        
        float dx = x1 - cameraX;
        float dz = z1 - cameraZ;
        
        if (dz > 0) {
          float s = 200.0 / dz;
          int sx = 160 + (int)(dx * s);
          int sy = 120;
          
          if (sx >= 0 && sx < 320 && sy >= 0 && sy < 240) {
            display.drawPixel(sx, sy, COLOR_GRAY);
          }
        }
      }
    }
  }
  
  // Draw planets
  for (int i = 0; i < MAX_PLANETS; i++) {
    if (!planets[i].active) continue;
    
    float dx = planets[i].x - cameraX;
    float dy = planets[i].y - cameraY;
    float dz = planets[i].z - cameraZ;
    
    if (dz <= 0) continue;
    
    float scale = 200.0 / dz;
    int screenX = 160 + (int)(dx * scale);
    int screenY = 120 + (int)(dy * scale);
    int screenRadius = max(2, (int)(planets[i].radius * scale));
    
    if (screenX > -50 && screenX < 370 && screenY > -50 && screenY < 290) {
      if (planets[i].hasRing) {
        drawPlanetWithRing(screenX, screenY, screenRadius, planets[i].color, planets[i].ringColor);
      } else {
        drawPlanet(screenX, screenY, screenRadius, planets[i].color);
      }
      
      // Draw label
      if (screenRadius > 5) {
        display.setCursor(screenX - 15, screenY + screenRadius + 3);
        display.setTextColor(COLOR_WHITE, COLOR_BLACK);
        display.setTextSize(1);
        display.print(planets[i].name);
      }
    }
    
    // Draw moons
    if (planets[i].hasMoons) {
      for (int m = 0; m < MAX_MOONS; m++) {
        if (moons[m].active && moons[m].parentPlanet == i) {
          float moonX = planets[i].x + cos(moons[m].orbitAngle) * moons[m].orbitRadius;
          float moonY = planets[i].y;
          float moonZ = planets[i].z + sin(moons[m].orbitAngle) * moons[m].orbitRadius;
          
          float mdx = moonX - cameraX;
          float mdy = moonY - cameraY;
          float mdz = moonZ - cameraZ;
          
          if (mdz > 0) {
            float ms = 200.0 / mdz;
            int msx = 160 + (int)(mdx * ms);
            int msy = 120 + (int)(mdy * ms);
            int msr = max(1, (int)(moons[m].radius * ms));
            
            if (msx >= 0 && msx < 320 && msy >= 0 && msy < 240) {
              display.fillCircle(msx, msy, msr, moons[m].color);
            }
          }
        }
      }
    }
  }
}

// Draw asteroids
void drawAsteroids() {
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (!asteroids[i].active) continue;
    
    float dx = asteroids[i].x - cameraX;
    float dy = asteroids[i].y - cameraY;
    float dz = asteroids[i].z - cameraZ;
    
    if (dz <= 0 || dz > 400) continue;
    
    float scale = 200.0 / dz;
    int screenX = 160 + (int)(dx * scale);
    int screenY = 120 + (int)(dy * scale);
    int size = max(1, (int)(asteroids[i].size * scale));
    
    if (screenX >= 0 && screenX < 320 && screenY >= 0 && screenY < 240) {
      // Draw as small irregular shape
      display.drawPixel(screenX, screenY, asteroids[i].color);
      display.drawPixel(screenX + 1, screenY, asteroids[i].color);
      display.drawPixel(screenX, screenY + 1, asteroids[i].color);
      if (size > 2) {
        display.drawPixel(screenX - 1, screenY, asteroids[i].color);
        display.drawPixel(screenX, screenY - 1, asteroids[i].color);
      }
    }
  }
}

// Draw HUD
void drawHUD() {
  display.fillRect(0, 0, 320, 20, COLOR_BLACK);
  display.drawFastHLine(0, 19, 320, COLOR_CYAN);
  
  display.setCursor(5, 5);
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.setTextSize(1);
  display.print("UNIVERSE SIM");
  
  display.setCursor(120, 5);
  display.print("CAM Z:");
  display.print((int)cameraZ);
  
  display.setCursor(220, 5);
  if (viewMode == 0) display.print("ORBIT VIEW");
  else if (viewMode == 1) display.print("PLANET VIEW");
  else display.print("EXPLORE");
  
  // Bottom info
  display.fillRect(0, 220, 320, 20, COLOR_BLACK);
  display.drawFastHLine(0, 220, 320, COLOR_CYAN);
  
  display.setCursor(5, 225);
  display.print("W/S:Zoom A/D:Rotate Q:Quit");
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
  display.setTextColor(COLOR_YELLOW, COLOR_BLACK);
  display.setTextSize(2);
  display.print("UNIVERSE");
  
  display.setCursor(70, 110);
  display.setTextSize(1);
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.print("Solar System Simulator");
  
  display.setCursor(80, 140);
  display.print("Loading planets...");
  
  delay(1500);
  
  initStars();
  initPlanets();
  initMoons();
  initAsteroids();
  
  Serial.println("=== Universe Simulator ===");
  Serial.println("Controls:");
  Serial.println("  w/s = Zoom in/out");
  Serial.println("  a/d = Rotate view");
  Serial.println("  v = Change view mode");
  Serial.println("  q = Quit");
}

void loop() {
  // Controls
  if (Serial.available()) {
    char c = Serial.read();
    
    if (c == 'w' || c == 'W') {
      cameraZ += 10;
      if (cameraZ > -50) cameraZ = -50;
      firstFrame = true;  // Force redraw
    }
    else if (c == 's' || c == 'S') {
      cameraZ -= 10;
      if (cameraZ < -500) cameraZ = -500;
      firstFrame = true;
    }
    else if (c == 'a' || c == 'A') {
      cameraX -= 10;
      firstFrame = true;
    }
    else if (c == 'd' || c == 'D') {
      cameraX += 10;
      firstFrame = true;
    }
    else if (c == 'v' || c == 'V') {
      viewMode = (viewMode + 1) % 3;
      firstFrame = true;
    }
    else if (c == 'q' || c == 'Q') {
      display.fillScreen(COLOR_BLACK);
      display.setCursor(100, 120);
      display.setTextColor(COLOR_CYAN, COLOR_BLACK);
      display.print("SIMULATION END");
      while(1);
    }
  }
  
  // Update
  updatePlanets();
  updateMoons();
  updateAsteroids();
  
  // Render - solo limpiar el área central, no toda la pantalla
  if (firstFrame) {
    display.fillScreen(COLOR_BLACK);
    firstFrame = false;
  } else {
    // Solo limpiar el área donde se dibujan los planetas (no HUD ni estrellas)
    display.fillRect(0, 20, 320, 200, COLOR_BLACK);
  }
  
  drawStars();
  drawPlanets();
  drawAsteroids();
  drawHUD();
  
  delay(50);  // Aumentar delay para reducir parpadeo (20 FPS)
}
