// Arkanoid Game - Control con IMU BMI270
// Arduino RP2350 + DVI/HDMI + BMI270
// Mueve la placa para controlar la paleta

#include <Wire.h>
#include <DevLab_DVIHSTX_Graphics.h>
#include "SparkFun_BMI270_Arduino_Library.h"

// Configuración HDMI
DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Sensor BMI270
BMI270 imu;
uint8_t i2cAddress = BMI2_I2C_PRIM_ADDR + 1; // 0x69

// Colores
#define COLOR_BLACK    0x0000
#define COLOR_WHITE    0xFFFF
#define COLOR_RED      0xF800
#define COLOR_GREEN    0x07E0
#define COLOR_BLUE     0x001F
#define COLOR_YELLOW   0xFFE0
#define COLOR_CYAN     0x07FF
#define COLOR_MAGENTA  0xF81F
#define COLOR_ORANGE   0xFD20
#define COLOR_PURPLE   0x780F

// Configuración del juego
#define PADDLE_WIDTH   40
#define PADDLE_HEIGHT  6
#define PADDLE_Y       (display.height() - 15)

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
bool imuAvailable;
unsigned long lastUpdate;

// Control IMU mejorado
float accelOffset = 0;  // Calibración inicial
float paddleVelocity = 0;  // Velocidad de la paleta
#define IMU_SENSITIVITY 80.0  // Sensibilidad aumentada
#define VELOCITY_DAMPING 0.85  // Amortiguación de velocidad
#define MAX_VELOCITY 15.0  // Velocidad máxima

void setup() {
  Serial.begin(115200);
  
  Serial.println("╔═══════════════════════════════╗");
  Serial.println("║  ARKANOID IMU - RP2350 DVI   ║");
  Serial.println("╚═══════════════════════════════╝");
  
  // PRIMERO: Inicializar display
  if (!display.begin()) {
    Serial.println("ERROR: Display no inicializado");
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  Serial.println("Display OK: 320x240");
  
  // SEGUNDO: Inicializar I2C para BMI270
  Wire.setSDA(8);
  Wire.setSCL(9);
  Wire.begin();
  delay(100);
  
  // Inicializar sensor BMI270
  if (imu.beginI2C(i2cAddress) != BMI2_OK) {
    Serial.println("WARNING: BMI270 no conectado");
    Serial.println("Usa controles Serial: a/d");
    imuAvailable = false;
  } else {
    Serial.println("BMI270 OK - Control IMU activado");
    Serial.println();
    Serial.println("INSTRUCCIONES:");
    Serial.println("  - Inclina la placa izquierda/derecha");
    Serial.println("  - para mover la paleta");
    Serial.println("  - Mantén nivel para calibrar");
    imuAvailable = true;
    
    // Calibrar posición inicial
    delay(500);
    imu.getSensorData();
    accelOffset = imu.data.accelY;
    Serial.print("Calibrado: offset = ");
    Serial.println(accelOffset);
  }
  
  Serial.println();
  
  // Inicializar juego
  initGame();
  showStartScreen();
  delay(2000);
  
  lastUpdate = millis();
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastUpdate >= 10) { // ~100 FPS para control más fluido
    lastUpdate = currentTime;
    
    if (!gameOver && !gameWon) {
      handleInput();
      updateGame();
      drawGame();
    } else {
      if (gameOver) showGameOver();
      if (gameWon) showGameWon();
      
      // Reiniciar con cualquier movimiento o tecla
      if (Serial.available()) {
        Serial.read();
        initGame();
      }
      if (imuAvailable) {
        imu.getSensorData();
        if (abs(imu.data.accelX) > 0.3) {
          initGame();
        }
      }
    }
  }
}

void initGame() {
  paddleX = (display.width() - PADDLE_WIDTH) / 2;
  
  ballX = display.width() / 2;
  ballY = display.height() / 2;
  ballVX = BALL_SPEED;
  ballVY = -BALL_SPEED;
  
  // Inicializar bloques
  for (int y = 0; y < BLOCKS_Y; y++) {
    for (int x = 0; x < BLOCKS_X; x++) {
      blocks[y][x] = true;
    }
  }
  
  // Colores por fila
  blockColors[0] = COLOR_RED;
  blockColors[1] = COLOR_ORANGE;
  blockColors[2] = COLOR_YELLOW;
  blockColors[3] = COLOR_GREEN;
  blockColors[4] = COLOR_CYAN;
  
  score = 0;
  lives = 3;
  gameOver = false;
  gameWon = false;
  
  // Recalibrar IMU al inicio
  if (imuAvailable) {
    delay(200);
    imu.getSensorData();
    accelOffset = imu.data.accelY;
  }
}

void handleInput() {
  if (imuAvailable) {
    // Control con IMU - Sistema de física mejorado
    imu.getSensorData();
    
    // Usar acelerómetro Y para control horizontal
    float accelY = imu.data.accelY - accelOffset;
    
    // Control basado en velocidad (más natural)
    // La inclinación controla la velocidad, no la posición directa
    paddleVelocity += accelY * 2.0;  // Aceleración
    paddleVelocity *= VELOCITY_DAMPING;  // Fricción
    
    // Limitar velocidad máxima
    if (paddleVelocity > MAX_VELOCITY) paddleVelocity = MAX_VELOCITY;
    if (paddleVelocity < -MAX_VELOCITY) paddleVelocity = -MAX_VELOCITY;
    
    // Aplicar velocidad a posición
    paddleX += paddleVelocity;
    
  } else {
    // Fallback: Control Serial
    while (Serial.available()) {
      char cmd = Serial.read();
      if (cmd == 'a' || cmd == 'A' || cmd == '<') {
        paddleX -= 8;
      } else if (cmd == 'd' || cmd == 'D' || cmd == '>') {
        paddleX += 8;
      } else if (cmd == 's' || cmd == 'S') {
        paddleX = (display.width() - PADDLE_WIDTH) / 2;
      }
    }
  }
  
  // Limitar paleta a pantalla con rebote suave
  if (paddleX < 0) {
    paddleX = 0;
    paddleVelocity = 0;  // Detener velocidad al tocar borde
  }
  if (paddleX > display.width() - PADDLE_WIDTH) {
    paddleX = display.width() - PADDLE_WIDTH;
    paddleVelocity = 0;  // Detener velocidad al tocar borde
  }
}

void updateGame() {
  // Mover pelota
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
      ballVY = -abs(ballVY);
      
      // Cambiar ángulo según donde golpee
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
  
  // Perder vida si sale por abajo
  if (ballY >= display.height()) {
    lives--;
    if (lives <= 0) {
      gameOver = true;
    } else {
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
          
          if (checkWin()) {
            gameWon = true;
          }
          return;
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
  
  // UI
  display.setTextSize(1);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.setCursor(5, 5);
  display.print("Score:");
  display.print(score);
  
  display.setCursor(display.width() - 60, 5);
  display.print("Lives:");
  display.print(lives);
  
  // Indicador IMU
  if (imuAvailable) {
    display.setCursor(120, 5);
    display.setTextColor(COLOR_GREEN, COLOR_BLACK);
    display.print("[IMU]");
  }
}

void showStartScreen() {
  display.fillScreen(COLOR_BLACK);
  display.setTextSize(2);
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.setCursor(50, 70);
  display.print("ARKANOID");
  
  display.setTextSize(1);
  display.setTextColor(COLOR_YELLOW, COLOR_BLACK);
  display.setCursor(100, 100);
  display.print("IMU CONTROL");
  
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.setCursor(40, 130);
  display.print("Tilt to move paddle!");
  
  if (imuAvailable) {
    display.setTextColor(COLOR_GREEN, COLOR_BLACK);
    display.setCursor(70, 150);
    display.print("BMI270 Ready");
  } else {
    display.setTextColor(COLOR_RED, COLOR_BLACK);
    display.setCursor(60, 150);
    display.print("No IMU - Use a/d");
  }
  
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
  
  display.setCursor(60, 170);
  if (imuAvailable) {
    display.print("Tilt to restart");
  } else {
    display.print("Press key to restart");
  }
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
  display.setCursor(60, 170);
  if (imuAvailable) {
    display.print("Tilt to restart");
  } else {
    display.print("Press key to restart");
  }
}
