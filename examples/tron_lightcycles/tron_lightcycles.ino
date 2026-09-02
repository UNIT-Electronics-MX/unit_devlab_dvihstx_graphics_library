// TRON Light Cycles - Motos de luz estilo Tron
// Arduino Nano RP2350 + DVI
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+(Blue), D5(GPIO14)=Data1+(Green), D3(GPIO16)=Data2+(Red)
//
// Dos motos de luz se mueven por una arena en vista cenital dejando una
// estela de luz permanente (como en la película Tron). Si chocan contra
// un muro o una estela (propia o ajena) explotan y comienza una ronda
// nueva. El marcador se conserva entre rondas.

#include <DevLab_DVIHSTX_Graphics.h>

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// ---- Arena / grid ----------------------------------------------------
const int CELL = 4;                       // tamaño de celda en píxeles
const int GRID_W = 320 / CELL;            // 80 columnas
const int GRID_H = 240 / CELL;            // 60 filas

// 0 = libre, 1 = estela moto A, 2 = estela moto B
uint8_t grid[GRID_H][GRID_W];

// ---- Colores estilo Tron ----------------------------------------------
const uint16_t COL_BG        = 0x0000;
const uint16_t COL_GRIDLINE  = 0x0011;   // azul muy tenue
const uint16_t COL_A_TRAIL   = 0x0195;   // cyan oscuro
const uint16_t COL_A_HEAD    = 0x07FF;   // cyan brillante
const uint16_t COL_B_TRAIL   = 0x9200;   // naranja oscuro
const uint16_t COL_B_HEAD    = 0xFC40;   // naranja brillante
const uint16_t COL_TEXT      = 0x07FF;
const uint16_t COL_WHITE     = 0xFFFF;

// Mezcla dos colores RGB565 (factor 0 = todo "a", 1 = todo "b").
uint16_t blend565(uint16_t a, uint16_t b, float factor) {
  if (factor < 0) factor = 0;
  if (factor > 1) factor = 1;
  uint8_t ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
  uint8_t br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
  uint8_t r = ar + (br - ar) * factor;
  uint8_t g = ag + (bg - ag) * factor;
  uint8_t bl = ab + (bb - ab) * factor;
  return (r << 11) | (g << 5) | bl;
}

// ---- Moto de luz --------------------------------------------------------
struct Bike {
  int gx, gy;        // posición actual en celdas
  int dx, dy;         // dirección: (1,0) (-1,0) (0,1) (0,-1)
  uint16_t trailColor;
  uint16_t headColor;
  uint8_t id;         // 1 o 2, usado como valor en grid[][]
  bool alive;
  int turnCooldown;   // celdas antes de poder girar de nuevo
  int score;
};

Bike bikeA, bikeB;
int roundNumber = 1;
int stepCounter = 0;
const int STEPS_PER_MOVE = 2;   // controla la velocidad (frames por celda)

// El generador automático de prototipos del IDE de Arduino los inserta
// justo después del último #include, es decir, antes de que "struct Bike"
// exista todavía. Eso rompe la compilación en las funciones que reciben
// Bike por referencia. Declararlos aquí a mano evita el problema.
void steerBike(Bike &b);
void updateBike(Bike &b);
void drawHead(const Bike &b);

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }

  Serial.println("TRON LIGHT CYCLES - Nano RP2350");
  randomSeed(analogRead(A0));

  bikeA.score = 0;
  bikeB.score = 0;

  startRound();
}

void loop() {
  drawHUD();

  stepCounter++;
  if (stepCounter >= STEPS_PER_MOVE) {
    stepCounter = 0;
    updateBike(bikeA);
    updateBike(bikeB);

    if (!bikeA.alive || !bikeB.alive) {
      resolveRound();
    }
  }

  delay(15);
}

// ------------------------------------------------------------------------
void startRound() {
  memset(grid, 0, sizeof(grid));

  display.fillScreen(COL_BG);
  drawBackgroundGrid();

  // Moto A entra por la izquierda hacia la derecha
  bikeA.gx = 4;
  bikeA.gy = GRID_H / 3;
  bikeA.dx = 1;
  bikeA.dy = 0;
  bikeA.trailColor = COL_A_TRAIL;
  bikeA.headColor = COL_A_HEAD;
  bikeA.id = 1;
  bikeA.alive = true;
  bikeA.turnCooldown = 6;

  // Moto B entra por la derecha hacia la izquierda
  bikeB.gx = GRID_W - 5;
  bikeB.gy = (GRID_H * 2) / 3;
  bikeB.dx = -1;
  bikeB.dy = 0;
  bikeB.trailColor = COL_B_TRAIL;
  bikeB.headColor = COL_B_HEAD;
  bikeB.id = 2;
  bikeB.alive = true;
  bikeB.turnCooldown = 6;

  grid[bikeA.gy][bikeA.gx] = bikeA.id;
  grid[bikeB.gy][bikeB.gx] = bikeB.id;

  flashText(("RONDA " + String(roundNumber)).c_str());
}

void drawBackgroundGrid() {
  for (int x = 0; x <= display.width(); x += CELL * 5) {
    display.drawFastVLine(x, 0, display.height(), COL_GRIDLINE);
  }
  for (int y = 0; y <= display.height(); y += CELL * 5) {
    display.drawFastHLine(0, y, display.width(), COL_GRIDLINE);
  }
}

bool cellFree(int gx, int gy) {
  if (gx < 0 || gx >= GRID_W || gy < 0 || gy >= GRID_H) return false;
  return grid[gy][gx] == 0;
}

// Intenta seguir recto; si hay obstáculo gira hacia un lado libre;
// también gira aleatoriamente de vez en cuando para que el recorrido
// no sea siempre una línea recta (más parecido al juego original).
void steerBike(Bike &b) {
  int fx = b.gx + b.dx;
  int fy = b.gy + b.dy;
  bool forwardFree = cellFree(fx, fy);

  bool wantsRandomTurn = (b.turnCooldown <= 0) && (random(100) < 6);

  if (forwardFree && !wantsRandomTurn) return;

  // Direcciones perpendiculares a la actual
  int leftDx = b.dy, leftDy = -b.dx;
  int rightDx = -b.dy, rightDy = b.dx;

  bool leftFree = cellFree(b.gx + leftDx, b.gy + leftDy);
  bool rightFree = cellFree(b.gx + rightDx, b.gy + rightDy);

  if (!forwardFree) {
    // Debe girar sí o sí
    if (leftFree && rightFree) {
      if (random(2) == 0) { b.dx = leftDx; b.dy = leftDy; }
      else { b.dx = rightDx; b.dy = rightDy; }
    } else if (leftFree) {
      b.dx = leftDx; b.dy = leftDy;
    } else if (rightFree) {
      b.dx = rightDx; b.dy = rightDy;
    } else {
      b.alive = false;  // callejón sin salida -> choque
      return;
    }
    b.turnCooldown = 5;
  } else if (wantsRandomTurn) {
    // Giro "estético" opcional
    if (leftFree && rightFree) {
      if (random(2) == 0) { b.dx = leftDx; b.dy = leftDy; }
      else { b.dx = rightDx; b.dy = rightDy; }
      b.turnCooldown = 5;
    }
  }
}

void updateBike(Bike &b) {
  if (!b.alive) return;

  if (b.turnCooldown > 0) b.turnCooldown--;

  steerBike(b);
  if (!b.alive) {
    explode(b.gx * CELL + CELL / 2, b.gy * CELL + CELL / 2, b.headColor);
    return;
  }

  int nx = b.gx + b.dx;
  int ny = b.gy + b.dy;

  if (!cellFree(nx, ny)) {
    b.alive = false;
    explode(b.gx * CELL + CELL / 2, b.gy * CELL + CELL / 2, b.headColor);
    return;
  }

  // Deja estela en la celda que abandona: halo tenue + núcleo brillante,
  // como un tubo de neón visto desde arriba.
  drawTrailCell(b.gx, b.gy, b.trailColor, b.headColor);
  grid[b.gy][b.gx] = b.id;

  // Avanza
  b.gx = nx;
  b.gy = ny;
  grid[b.gy][b.gx] = b.id;

  // Dibuja la cabeza: cuerpo brillante + punta blanca que parpadea y
  // apunta hacia la dirección de avance.
  drawHead(b);
}

void drawTrailCell(int gx, int gy, uint16_t trailColor, uint16_t coreColor) {
  int px = gx * CELL, py = gy * CELL;
  display.fillRect(px, py, CELL, CELL, trailColor);
  uint16_t core = blend565(trailColor, coreColor, 0.6);
  if (CELL >= 3) {
    display.fillRect(px + 1, py + 1, CELL - 2, CELL - 2, core);
  } else {
    display.drawPixel(px, py, core);
  }
}

void drawHead(const Bike &b) {
  int px = b.gx * CELL, py = b.gy * CELL;
  display.fillRect(px, py, CELL, CELL, b.headColor);

  // Faro delantero: parpadeo suave hacia blanco para dar sensación de brillo.
  float pulse = (sin(millis() * 0.02) + 1.0) * 0.25;  // 0.0 .. 0.5
  uint16_t hot = blend565(b.headColor, COL_WHITE, pulse + 0.3);
  int nosePx = px + b.dx * (CELL / 2);
  int nosePy = py + b.dy * (CELL / 2);
  display.fillRect(nosePx, nosePy, max(1, CELL / 2), max(1, CELL / 2), hot);
}

void explode(int px, int py, uint16_t color) {
  // Flash inicial blanco.
  display.fillCircle(px, py, 6, COL_WHITE);
  delay(20);
  display.fillCircle(px, py, 6, COL_BG);

  // Chispas radiales que se alejan y se apagan.
  const int NUM_SPARKS = 10;
  float angle[NUM_SPARKS];
  for (int i = 0; i < NUM_SPARKS; i++) {
    angle[i] = (TWO_PI / NUM_SPARKS) * i + random(-10, 10) * 0.01;
  }

  for (int step = 1; step <= 6; step++) {
    int r = step * 3;
    uint16_t fade = blend565(color, COL_BG, (float)step / 6.0);
    for (int i = 0; i < NUM_SPARKS; i++) {
      int sx = px + (int)(cos(angle[i]) * r);
      int sy = py + (int)(sin(angle[i]) * r);
      display.fillCircle(sx, sy, 2, fade);
    }
    display.drawCircle(px, py, r, fade);
    delay(15);
  }

  // Borra el resplandor residual dejando solo el punto del choque.
  display.fillCircle(px, py, 20, COL_BG);
  display.fillRect(px - 2, py - 2, 4, 4, color);
}

void resolveRound() {
  if (!bikeA.alive && bikeB.alive) bikeB.score++;
  if (!bikeB.alive && bikeA.alive) bikeA.score++;
  // Si ambas mueren en el mismo paso, ronda empatada (sin punto)

  delay(600);
  roundNumber++;
  startRound();
}

void flashText(const char *msg) {
  display.setTextSize(2);
  display.setTextColor(COL_TEXT, COL_BG);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
  int cx = (display.width() - w) / 2;
  int cy = (display.height() - h) / 2;
  display.setCursor(cx, cy);
  display.print(msg);
  delay(700);
  display.fillRect(cx - 4, cy - 4, w + 8, h + 8, COL_BG);
}

void drawHUD() {
  display.setTextSize(1);
  display.setTextColor(COL_A_HEAD, COL_BG);
  display.setCursor(4, 2);
  display.print("P1: ");
  display.print(bikeA.score);

  display.setTextColor(COL_B_HEAD, COL_BG);
  display.setCursor(display.width() - 60, 2);
  display.print("P2: ");
  display.print(bikeB.score);

  display.setTextColor(COL_TEXT, COL_BG);
  display.setCursor(display.width() / 2 - 40, 2);
  display.print("TRON CYCLES");
}
