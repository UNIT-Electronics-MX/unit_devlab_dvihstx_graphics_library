// Demo de juego simple - Snake
// Arduino Nano RP2350 + DVI
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+(Blue), D5(GPIO14)=Data1+(Green), D3(GPIO16)=Data2+(Red)
// Control: Usa el monitor serial para enviar comandos (w/a/s/d)

#include <DevLab_DVIHSTX_Graphics.h>

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Configuración del juego
const int GRID_SIZE = 8;
const int GRID_W = 40;
const int GRID_H = 30;
const int OFFSET_X = 0;
const int OFFSET_Y = 0;

struct Point {
  int x, y;
};

Point snake[100];
int snakeLength = 5;
Point food;
int direction = 0; // 0=right, 1=down, 2=left, 3=up
int score = 0;
bool gameOver = false;
unsigned long lastMove = 0;
int moveDelay = 200;

void setup() {
  Serial.begin(115200);
  
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  Serial.println("========================================");
  Serial.println("      SNAKE GAME - Nano RP2350");
  Serial.println("========================================");
  Serial.println();
  Serial.println("Controls (send via Serial Monitor):");
  Serial.println("  w = up");
  Serial.println("  s = down");
  Serial.println("  a = left");
  Serial.println("  d = right");
  Serial.println();
  Serial.println("Game starting...");
  
  initGame();
  drawGame();
}

void loop() {
  if (!gameOver) {
    // Leer comando serial
    if (Serial.available() > 0) {
      char cmd = Serial.read();
      handleInput(cmd);
    }
    
    // Mover snake
    if (millis() - lastMove > moveDelay) {
      moveSnake();
      checkCollision();
      checkFood();
      drawGame();
      lastMove = millis();
    }
  } else {
    // Mostrar game over
    if (millis() % 1000 < 500) {
      display.setCursor(GRID_W * GRID_SIZE / 2 - 40, GRID_H * GRID_SIZE / 2);
      display.setTextColor(0xF800);
      display.setTextSize(2);
      display.print("GAME OVER");
    }
    
    // Reiniciar con cualquier tecla
    if (Serial.available() > 0) {
      Serial.read();
      initGame();
      gameOver = false;
      Serial.println("Game restarted!");
    }
  }
}

void initGame() {
  snakeLength = 5;
  score = 0;
  direction = 0;
  
  // Inicializar snake en el centro
  for (int i = 0; i < snakeLength; i++) {
    snake[i].x = GRID_W / 2 - i;
    snake[i].y = GRID_H / 2;
  }
  
  spawnFood();
  display.fillScreen(0x0000);
}

void spawnFood() {
  bool valid = false;
  while (!valid) {
    food.x = random(GRID_W);
    food.y = random(GRID_H);
    
    valid = true;
    for (int i = 0; i < snakeLength; i++) {
      if (snake[i].x == food.x && snake[i].y == food.y) {
        valid = false;
        break;
      }
    }
  }
}

void handleInput(char cmd) {
  switch(cmd) {
    case 'w': if (direction != 1) direction = 3; break; // up
    case 's': if (direction != 3) direction = 1; break; // down
    case 'a': if (direction != 0) direction = 2; break; // left
    case 'd': if (direction != 2) direction = 0; break; // right
  }
}

void moveSnake() {
  // Mover cuerpo
  for (int i = snakeLength - 1; i > 0; i--) {
    snake[i] = snake[i - 1];
  }
  
  // Mover cabeza
  switch(direction) {
    case 0: snake[0].x++; break; // right
    case 1: snake[0].y++; break; // down
    case 2: snake[0].x--; break; // left
    case 3: snake[0].y--; break; // up
  }
}

void checkCollision() {
  // Colisión con paredes
  if (snake[0].x < 0 || snake[0].x >= GRID_W ||
      snake[0].y < 0 || snake[0].y >= GRID_H) {
    gameOver = true;
    Serial.println("Game Over! Hit wall.");
    Serial.print("Final Score: ");
    Serial.println(score);
    return;
  }
  
  // Colisión consigo mismo
  for (int i = 1; i < snakeLength; i++) {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
      gameOver = true;
      Serial.println("Game Over! Hit yourself.");
      Serial.print("Final Score: ");
      Serial.println(score);
      return;
    }
  }
}

void checkFood() {
  if (snake[0].x == food.x && snake[0].y == food.y) {
    score++;
    snakeLength++;
    spawnFood();
    
    // Aumentar velocidad
    if (moveDelay > 100) moveDelay -= 5;
    
    Serial.print("Score: ");
    Serial.println(score);
  }
}

void drawGame() {
  display.fillScreen(0x0000);
  
  // Dibujar borde
  display.drawRect(OFFSET_X - 1, OFFSET_Y - 1, 
                   GRID_W * GRID_SIZE + 2, GRID_H * GRID_SIZE + 2, 0xFFFF);
  
  // Dibujar snake
  for (int i = 0; i < snakeLength; i++) {
    uint16_t color;
    if (i == 0) color = 0x07E0; // Cabeza verde brillante
    else color = 0x0400;         // Cuerpo verde oscuro
    
    display.fillRect(
      OFFSET_X + snake[i].x * GRID_SIZE + 1,
      OFFSET_Y + snake[i].y * GRID_SIZE + 1,
      GRID_SIZE - 2, GRID_SIZE - 2,
      color
    );
  }
  
  // Dibujar comida
  display.fillCircle(
    OFFSET_X + food.x * GRID_SIZE + GRID_SIZE / 2,
    OFFSET_Y + food.y * GRID_SIZE + GRID_SIZE / 2,
    GRID_SIZE / 2 - 1,
    0xF800 // Rojo
  );
  
  // Dibujar score
  display.setCursor(5, 5);
  display.setTextColor(0xFFFF);
  display.setTextSize(1);
  display.print("Score: ");
  display.print(score);
  display.print("  Speed: ");
  display.print(300 - moveDelay);
}
