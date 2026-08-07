// Arkanoid Game - Rompe bloques clásico
// Arduino RP2350 + DVI/HDMI
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+, D5(GPIO14)=Data1+, D3(GPIO16)=Data2+

#include <UDVI_HSTX.h>

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
#define COLOR_ORANGE   (uint16_t)0xFD20
#define COLOR_PURPLE   (uint16_t)0x780F

// Configuración del juego
#define PADDLE_WIDTH   40
#define PADDLE_HEIGHT  6
#define PADDLE_Y       (display.height() - 15)
#define PADDLE_SPEED   4

#define BALL_SIZE      4
#define BALL_SPEED     2

#define BLOCK_WIDTH    30
#define BLOCK_HEIGHT   10
#define BLOCKS_X       10
#define BLOCKS_Y       5
#define BLOCKS_OFFSET_Y 20

// Variables del juego
float paddleX;
float ballX, ballY;
float ballVX, ballVY;
bool blocks[BLOCKS_Y][BLOCKS_X];
uint16_t blockColors[BLOCKS_Y];
int score;
int lives;
bool gameOver;
bool gameWon;
unsigned long lastUpdate;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  Serial.println("╔═══════════════════════════════╗");
  Serial.println("║    ARKANOID - RP2350 DVI     ║");
  Serial.println("╚═══════════════════════════════╝");
  Serial.println();
  Serial.println("Controls:");
  Serial.println("  - 'a' or '<' = Left");
  Serial.println("  - 'd' or '>' = Right");
  Serial.println("  - 's' = Center");
  Serial.println();
  Serial.println("IMPORTANT:");
  Serial.println("  1. Open Serial Monitor");
  Serial.println("  2. Set to 115200 baud");
  Serial.println("  3. Type 'a' or 'd' and press ENTER");
  Serial.println("  4. Or enable auto mode (line 126)");
  Serial.println();
  
  // Initialize game
  initGame();
  
  // Start screen
  showStartScreen();
  delay(2000);
  
  lastUpdate = millis();
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
      
      // Reiniciar con cualquier tecla
      if (Serial.available()) {
        Serial.read();
        initGame();
      }
    }
  }
}

void initGame() {
  // Initial paddle position
  paddleX = (display.width() - PADDLE_WIDTH) / 2;
  
  // Initial ball position
  ballX = display.width() / 2;
  ballY = display.height() / 2;
  ballVX = BALL_SPEED;
  ballVY = -BALL_SPEED;
  
  // Initialize blocks
  for (int y = 0; y < BLOCKS_Y; y++) {
    for (int x = 0; x < BLOCKS_X; x++) {
      blocks[y][x] = true;
    }
  }
  
  // Colors per row
  blockColors[0] = COLOR_RED;
  blockColors[1] = COLOR_ORANGE;
  blockColors[2] = COLOR_YELLOW;
  blockColors[3] = COLOR_GREEN;
  blockColors[4] = COLOR_CYAN;
  
  score = 0;
  lives = 3;
  gameOver = false;
  gameWon = false;
}

void handleInput() {
  // Read ALL pending Serial commands
  while (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'a' || cmd == 'A' || cmd == '<') {
      paddleX -= PADDLE_SPEED * 2; // Faster
    } else if (cmd == 'd' || cmd == 'D' || cmd == '>') {
      paddleX += PADDLE_SPEED * 2; // Faster
    } else if (cmd == 's' || cmd == 'S') {
      paddleX = (display.width() - PADDLE_WIDTH) / 2; // Center
    }
  }
  
  // Auto movement for testing (uncomment if Serial doesn't work)
  // static unsigned long autoMoveTimer = 0;
  // if (millis() - autoMoveTimer > 30) {
  //   autoMoveTimer = millis();
  //   paddleX = (display.width() - PADDLE_WIDTH) / 2 + sin(millis() / 500.0) * 80;
  // }
  
  // Limit paddle to screen
  if (paddleX < 0) paddleX = 0;
  if (paddleX > display.width() - PADDLE_WIDTH) paddleX = display.width() - PADDLE_WIDTH;
}

void updateGame() {
  // Move ball
  ballX += ballVX;
  ballY += ballVY;
  
  // Rebote en paredes laterales
  if (ballX <= 0 || ballX >= display.width() - BALL_SIZE) {
    ballVX = -ballVX;
    ballX = constrain(ballX, 0, display.width() - BALL_SIZE);
  }
  
  // Rebote en techo
  if (ballY <= 0) {
    ballVY = -ballVY;
    ballY = 0;
  }
  
  // Rebote en paleta
  if (ballY + BALL_SIZE >= PADDLE_Y && ballY + BALL_SIZE <= PADDLE_Y + PADDLE_HEIGHT) {
    if (ballX + BALL_SIZE >= paddleX && ballX <= paddleX + PADDLE_WIDTH) {
      ballVY = -abs(ballVY); // Siempre hacia arriba
      
      // Cambiar ángulo según donde golpee en la paleta
      float hitPos = (ballX + BALL_SIZE/2) - (paddleX + PADDLE_WIDTH/2);
      ballVX = hitPos / 8;
      
      // Normalizar velocidad
      float speed = sqrt(ballVX * ballVX + ballVY * ballVY);
      if (speed > 0) {
        ballVX = (ballVX / speed) * BALL_SPEED;
        ballVY = (ballVY / speed) * BALL_SPEED;
      }
    }
  }
  
  // Perder vida si la pelota sale por abajo
  if (ballY >= display.height()) {
    lives--;
    if (lives <= 0) {
      gameOver = true;
    } else {
      // Resetear pelota
      ballX = display.width() / 2;
      ballY = display.height() / 2;
      ballVX = BALL_SPEED;
      ballVY = -BALL_SPEED;
      delay(500);
    }
  }
  
  // Colisión con bloques
  for (int y = 0; y < BLOCKS_Y; y++) {
    for (int x = 0; x < BLOCKS_X; x++) {
      if (blocks[y][x]) {
        int blockX = x * BLOCK_WIDTH + 5;
        int blockY = y * BLOCK_HEIGHT + BLOCKS_OFFSET_Y;
        
        if (ballX + BALL_SIZE >= blockX && ballX <= blockX + BLOCK_WIDTH - 2 &&
            ballY + BALL_SIZE >= blockY && ballY <= blockY + BLOCK_HEIGHT - 2) {
          blocks[y][x] = false;
          score += 10;
          ballVY = -ballVY;
          
          // Verificar si ganó
          if (checkWin()) {
            gameWon = true;
          }
          return; // Solo destruir un bloque por frame
        }
      }
    }
  }
}

bool checkWin() {
  for (int y = 0; y < BLOCKS_Y; y++) {
    for (int x = 0; x < BLOCKS_X; x++) {
      if (blocks[y][x]) return false;
    }
  }
  return true;
}

void drawGame() {
  // Fondo negro
  display.fillScreen(COLOR_BLACK);
  
  // Dibujar bloques
  for (int y = 0; y < BLOCKS_Y; y++) {
    for (int x = 0; x < BLOCKS_X; x++) {
      if (blocks[y][x]) {
        int blockX = x * BLOCK_WIDTH + 5;
        int blockY = y * BLOCK_HEIGHT + BLOCKS_OFFSET_Y;
        display.fillRect(blockX, blockY, BLOCK_WIDTH - 2, BLOCK_HEIGHT - 2, blockColors[y]);
      }
    }
  }
  
  // Dibujar paleta
  display.fillRect(paddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_CYAN);
  display.drawRect(paddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_WHITE);
  
  // Dibujar pelota
  display.fillRect(ballX, ballY, BALL_SIZE, BALL_SIZE, COLOR_WHITE);
  
  // Dibujar UI
  display.setTextSize(1);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.setCursor(5, 5);
  display.print("Score:");
  display.print(score);
  
  display.setCursor(display.width() - 60, 5);
  display.print("Lives:");
  display.print(lives);
}

void showStartScreen() {
  display.fillScreen(COLOR_BLACK);
  display.setTextSize(2);
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.setCursor(80, 80);
  display.print("ARKANOID");
  
  display.setTextSize(1);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.setCursor(20, 120);
  display.print("a = Left | d = Right | s = Center");
  display.setCursor(30, 140);
  display.print("Destroy all the blocks!");
  display.setCursor(30, 160);
  display.print("(Use Serial Monitor)");
  
  display.setCursor(80, 180);
  display.setTextColor(COLOR_YELLOW, COLOR_BLACK);
  display.print("Starting...");
}

void showGameOver() {
  display.setTextSize(3);
  display.setTextColor(COLOR_RED, COLOR_BLACK);
  display.setCursor(40, 100);
  display.print("GAME OVER");
  
  display.setTextSize(1);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.setCursor(80, 150);
  display.print("Score: ");
  display.print(score);
  
  display.setCursor(50, 170);
  display.print("Press any key to restart");
}

void showGameWon() {
  display.setTextSize(2);
  display.setTextColor(COLOR_GREEN, COLOR_BLACK);
  display.setCursor(50, 100);
  display.print("YOU WIN!");
  
  display.setTextSize(1);
  display.setTextColor(COLOR_YELLOW, COLOR_BLACK);
  display.setCursor(80, 140);
  display.print("Score: ");
  display.print(score);
  
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.setCursor(40, 170);
  display.print("Press any key to restart");
}
