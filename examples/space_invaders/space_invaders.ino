// Space Invaders Game - Juego clásico de invasores
// Arduino RP2350 + DVI/HDMI
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+, D5(GPIO14)=Data1+, D3(GPIO16)=Data2+

#include <DevLab_DVIHSTX_Graphics.h>

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Colores invertidos para el hardware
#define COLOR_BLACK    (uint16_t)0x0000
#define COLOR_WHITE    (uint16_t)0xFFFF
#define COLOR_RED      (uint16_t)0xF800
#define COLOR_GREEN    (uint16_t)0x07E0
#define COLOR_BLUE     (uint16_t)0x001F
#define COLOR_YELLOW   (uint16_t)0xFFE0
#define COLOR_CYAN     (uint16_t)0x07FF
#define COLOR_MAGENTA  (uint16_t)0xF81F

// Configuración del juego
#define PLAYER_WIDTH   12
#define PLAYER_HEIGHT  8
#define PLAYER_Y       (display.height() - 20)
#define PLAYER_SPEED   3

#define INVADER_WIDTH  10
#define INVADER_HEIGHT 8
#define INVADERS_X     11
#define INVADERS_Y     5
#define INVADER_SPACING 4
#define INVADER_START_Y 30

#define BULLET_WIDTH   2
#define BULLET_HEIGHT  6
#define BULLET_SPEED   4
#define MAX_BULLETS    3

#define ENEMY_BULLET_SPEED 2
#define MAX_ENEMY_BULLETS  5

#define BARRIER_WIDTH  20
#define BARRIER_HEIGHT 12
#define NUM_BARRIERS   4

// Estructuras
struct Bullet {
  float x, y;
  bool active;
};

struct Barrier {
  int x, y;
  int hits;
};

// Variables del juego
float playerX;
bool invaders[INVADERS_Y][INVADERS_X];
float invadersX, invadersY;
float invaderSpeed;
int invaderDirection; // 1 = derecha, -1 = izquierda

Bullet bullets[MAX_BULLETS];
Bullet enemyBullets[MAX_ENEMY_BULLETS];
Barrier barriers[NUM_BARRIERS];

int score;
int lives;
bool gameOver;
bool gameWon;
unsigned long lastUpdate;
unsigned long lastEnemyShot;
unsigned long lastInvaderMove;
int invaderMoveDelay;
bool invaderFrame; // Para animación

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  Serial.println("╔═══════════════════════════════╗");
  Serial.println("║   SPACE INVADERS - RP2350    ║");
  Serial.println("╚═══════════════════════════════╝");
  Serial.println();
  Serial.println("Controles:");
  Serial.println("  - 'a' = Izquierda");
  Serial.println("  - 'd' = Derecha");
  Serial.println("  - ESPACIO o 'w' = Disparar");
  Serial.println();
  
  initGame();
  showStartScreen();
  delay(2000);
  
  lastUpdate = millis();
  lastEnemyShot = millis();
  lastInvaderMove = millis();
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastUpdate >= 16) { // ~60 FPS
    lastUpdate = currentTime;
    
    if (!gameOver && !gameWon) {
      handleInput();
      updateGame();
      drawGame();
    } else {
      if (gameOver) showGameOver();
      if (gameWon) showGameWon();
      
      if (Serial.available()) {
        Serial.read();
        initGame();
      }
    }
  }
}

void initGame() {
  playerX = (display.width() - PLAYER_WIDTH) / 2;
  
  // Inicializar invasores
  for (int y = 0; y < INVADERS_Y; y++) {
    for (int x = 0; x < INVADERS_X; x++) {
      invaders[y][x] = true;
    }
  }
  
  invadersX = 10;
  invadersY = INVADER_START_Y;
  invaderSpeed = 1.0;
  invaderDirection = 1;
  invaderMoveDelay = 500;
  invaderFrame = false;
  
  // Inicializar balas
  for (int i = 0; i < MAX_BULLETS; i++) {
    bullets[i].active = false;
  }
  for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
    enemyBullets[i].active = false;
  }
  
  // Inicializar barreras
  int barrierSpacing = display.width() / (NUM_BARRIERS + 1);
  for (int i = 0; i < NUM_BARRIERS; i++) {
    barriers[i].x = barrierSpacing * (i + 1) - BARRIER_WIDTH / 2;
    barriers[i].y = PLAYER_Y - 40;
    barriers[i].hits = 0;
  }
  
  score = 0;
  lives = 3;
  gameOver = false;
  gameWon = false;
}

void handleInput() {
  while (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'a' || cmd == 'A') {
      playerX -= PLAYER_SPEED * 2;
    } else if (cmd == 'd' || cmd == 'D') {
      playerX += PLAYER_SPEED * 2;
    } else if (cmd == ' ' || cmd == 'w' || cmd == 'W') {
      shootBullet();
    }
  }
  
  if (playerX < 0) playerX = 0;
  if (playerX > display.width() - PLAYER_WIDTH) playerX = display.width() - PLAYER_WIDTH;
}

void shootBullet() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      bullets[i].x = playerX + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2;
      bullets[i].y = PLAYER_Y - BULLET_HEIGHT;
      bullets[i].active = true;
      break;
    }
  }
}

void updateGame() {
  unsigned long currentTime = millis();
  
  // Mover invasores
  if (currentTime - lastInvaderMove >= invaderMoveDelay) {
    lastInvaderMove = currentTime;
    invaderFrame = !invaderFrame;
    
    invadersX += invaderSpeed * invaderDirection;
    
    // Verificar si tocan el borde
    bool hitEdge = false;
    if (invaderDirection == 1 && invadersX + INVADERS_X * (INVADER_WIDTH + INVADER_SPACING) >= display.width()) {
      hitEdge = true;
    } else if (invaderDirection == -1 && invadersX <= 0) {
      hitEdge = true;
    }
    
    if (hitEdge) {
      invaderDirection = -invaderDirection;
      invadersY += 8;
      invaderMoveDelay = max(100, invaderMoveDelay - 20); // Acelerar
    }
  }
  
  // Verificar si invasores llegaron al fondo
  if (invadersY + INVADERS_Y * (INVADER_HEIGHT + 2) >= PLAYER_Y) {
    gameOver = true;
  }
  
  // Mover balas del jugador
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      bullets[i].y -= BULLET_SPEED;
      
      if (bullets[i].y < 0) {
        bullets[i].active = false;
      }
      
      // Colisión con invasores
      for (int y = 0; y < INVADERS_Y; y++) {
        for (int x = 0; x < INVADERS_X; x++) {
          if (invaders[y][x]) {
            int invX = invadersX + x * (INVADER_WIDTH + INVADER_SPACING);
            int invY = invadersY + y * (INVADER_HEIGHT + 2);
            
            if (bullets[i].x + BULLET_WIDTH >= invX && bullets[i].x <= invX + INVADER_WIDTH &&
                bullets[i].y <= invY + INVADER_HEIGHT && bullets[i].y + BULLET_HEIGHT >= invY) {
              invaders[y][x] = false;
              bullets[i].active = false;
              score += (INVADERS_Y - y) * 10;
              
              if (checkWin()) gameWon = true;
              break;
            }
          }
        }
      }
      
      // Colisión con barreras
      for (int b = 0; b < NUM_BARRIERS; b++) {
        if (barriers[b].hits < 4 && 
            bullets[i].x + BULLET_WIDTH >= barriers[b].x && bullets[i].x <= barriers[b].x + BARRIER_WIDTH &&
            bullets[i].y <= barriers[b].y + BARRIER_HEIGHT && bullets[i].y + BULLET_HEIGHT >= barriers[b].y) {
          bullets[i].active = false;
          barriers[b].hits++;
        }
      }
    }
  }
  
  // Disparos enemigos
  if (currentTime - lastEnemyShot >= 1000) {
    lastEnemyShot = currentTime;
    enemyShoot();
  }
  
  // Mover balas enemigas
  for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
    if (enemyBullets[i].active) {
      enemyBullets[i].y += ENEMY_BULLET_SPEED;
      
      if (enemyBullets[i].y >= display.height()) {
        enemyBullets[i].active = false;
      }
      
      // Colisión con jugador
      if (enemyBullets[i].x + BULLET_WIDTH >= playerX && enemyBullets[i].x <= playerX + PLAYER_WIDTH &&
          enemyBullets[i].y + BULLET_HEIGHT >= PLAYER_Y && enemyBullets[i].y <= PLAYER_Y + PLAYER_HEIGHT) {
        enemyBullets[i].active = false;
        lives--;
        if (lives <= 0) gameOver = true;
      }
      
      // Colisión con barreras
      for (int b = 0; b < NUM_BARRIERS; b++) {
        if (barriers[b].hits < 4 && 
            enemyBullets[i].x + BULLET_WIDTH >= barriers[b].x && enemyBullets[i].x <= barriers[b].x + BARRIER_WIDTH &&
            enemyBullets[i].y + BULLET_HEIGHT >= barriers[b].y && enemyBullets[i].y <= barriers[b].y + BARRIER_HEIGHT) {
          enemyBullets[i].active = false;
          barriers[b].hits++;
        }
      }
    }
  }
}

void enemyShoot() {
  // Buscar un invasor aleatorio en la fila inferior
  for (int y = INVADERS_Y - 1; y >= 0; y--) {
    for (int x = 0; x < INVADERS_X; x++) {
      if (invaders[y][x] && random(100) < 10) {
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
          if (!enemyBullets[i].active) {
            int invX = invadersX + x * (INVADER_WIDTH + INVADER_SPACING);
            int invY = invadersY + y * (INVADER_HEIGHT + 2);
            enemyBullets[i].x = invX + INVADER_WIDTH / 2;
            enemyBullets[i].y = invY + INVADER_HEIGHT;
            enemyBullets[i].active = true;
            return;
          }
        }
      }
    }
  }
}

bool checkWin() {
  for (int y = 0; y < INVADERS_Y; y++) {
    for (int x = 0; x < INVADERS_X; x++) {
      if (invaders[y][x]) return false;
    }
  }
  return true;
}

void drawGame() {
  display.fillScreen(COLOR_BLACK);
  
  // Dibujar invasores
  for (int y = 0; y < INVADERS_Y; y++) {
    uint16_t color = (y < 2) ? COLOR_RED : (y < 4) ? COLOR_CYAN : COLOR_GREEN;
    for (int x = 0; x < INVADERS_X; x++) {
      if (invaders[y][x]) {
        int invX = invadersX + x * (INVADER_WIDTH + INVADER_SPACING);
        int invY = invadersY + y * (INVADER_HEIGHT + 2);
        drawInvader(invX, invY, color);
      }
    }
  }
  
  // Dibujar jugador
  drawPlayer(playerX, PLAYER_Y);
  
  // Dibujar balas
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      display.fillRect(bullets[i].x, bullets[i].y, BULLET_WIDTH, BULLET_HEIGHT, COLOR_YELLOW);
    }
  }
  
  // Dibujar balas enemigas
  for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
    if (enemyBullets[i].active) {
      display.fillRect(enemyBullets[i].x, enemyBullets[i].y, BULLET_WIDTH, BULLET_HEIGHT, COLOR_RED);
    }
  }
  
  // Dibujar barreras
  for (int i = 0; i < NUM_BARRIERS; i++) {
    if (barriers[i].hits < 4) {
      uint16_t color = barriers[i].hits == 0 ? COLOR_GREEN : 
                       barriers[i].hits == 1 ? COLOR_YELLOW :
                       barriers[i].hits == 2 ? COLOR_MAGENTA : COLOR_RED;
      display.fillRect(barriers[i].x, barriers[i].y, BARRIER_WIDTH, BARRIER_HEIGHT, color);
    }
  }
  
  // UI
  display.setTextSize(1);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.setCursor(5, 5);
  display.print("Score:");
  display.print(score);
  
  display.setCursor(display.width() - 60, 5);
  display.print("Lives:");
  display.print(lives);
}

void drawInvader(int x, int y, uint16_t color) {
  // Sprite simple del invasor
  if (invaderFrame) {
    display.fillRect(x + 2, y, 6, 2, color);
    display.fillRect(x + 1, y + 2, 8, 4, color);
    display.fillRect(x, y + 6, 10, 2, color);
  } else {
    display.fillRect(x + 2, y, 6, 2, color);
    display.fillRect(x, y + 2, 10, 4, color);
    display.fillRect(x + 1, y + 6, 8, 2, color);
  }
}

void drawPlayer(int x, int y) {
  display.fillRect(x + 5, y, 2, 4, COLOR_CYAN);
  display.fillRect(x, y + 4, 12, 4, COLOR_CYAN);
  display.drawRect(x, y + 4, 12, 4, COLOR_WHITE);
}

void showStartScreen() {
  display.fillScreen(COLOR_BLACK);
  display.setTextSize(2);
  display.setTextColor(COLOR_GREEN, COLOR_BLACK);
  display.setCursor(40, 60);
  display.print("SPACE");
  display.setCursor(40, 80);
  display.print("INVADERS");
  
  display.setTextSize(1);
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.setCursor(70, 120);
  display.print("a = Left");
  display.setCursor(70, 135);
  display.print("d = Right");
  display.setCursor(70, 150);
  display.print("w = Shoot");
  
  display.setTextColor(COLOR_YELLOW, COLOR_BLACK);
  display.setCursor(70, 180);
  display.print("Starting...");
}

void showGameOver() {
  display.setTextSize(3);
  display.setTextColor(COLOR_RED, COLOR_BLACK);
  display.setCursor(40, 100);
  display.print("GAME OVER");
  
  display.setTextSize(1);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.setCursor(100, 150);
  display.print("Score: ");
  display.print(score);
  
  display.setCursor(60, 180);
  display.print("Press any key");
}

void showGameWon() {
  display.setTextSize(2);
  display.setTextColor(COLOR_GREEN, COLOR_BLACK);
  display.setCursor(50, 90);
  display.print("VICTORY!");
  display.setCursor(30, 115);
  display.print("EARTH SAVED!");
  
  display.setTextSize(1);
  display.setTextColor(COLOR_YELLOW, COLOR_BLACK);
  display.setCursor(100, 150);
  display.print("Score: ");
  display.print(score);
  
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.setCursor(60, 180);
  display.print("Presiona cualquier tecla");
}
