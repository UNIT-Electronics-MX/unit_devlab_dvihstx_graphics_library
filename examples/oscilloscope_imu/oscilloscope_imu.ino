// Osciloscopio Digital - Visualización de señales IMU
// Arduino RP2350 + DVI/HDMI + BMI270
// Muestra acelerómetro y giroscopio en tiempo real

#include <Wire.h>
#include <DevLab_DVIHSTX_Graphics.h>
#include "SparkFun_BMI270_Arduino_Library.h"

// Configuración HDMI
DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_640x480);

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
#define COLOR_GRAY     0x7BEF
#define COLOR_DARKGRAY 0x39E7

// Configuración del osciloscopio
#define BUFFER_SIZE 600  // Puntos de datos
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define GRAPH_TOP 60
#define GRAPH_HEIGHT 180
#define GRAPH_SPACING 10
#define SAMPLES_PER_SCREEN 580

// Buffers para datos
float accelXBuffer[BUFFER_SIZE];
float accelYBuffer[BUFFER_SIZE];
float accelZBuffer[BUFFER_SIZE];
float gyroXBuffer[BUFFER_SIZE];
float gyroYBuffer[BUFFER_SIZE];
float gyroZBuffer[BUFFER_SIZE];

int bufferIndex = 0;
bool imuAvailable = false;

// Configuración de visualización
float accelScale = 2.0;   // ±2g
float gyroScale = 250.0;  // ±250 deg/s
int sampleDelay = 5;      // ms entre muestras
bool paused = false;
int viewMode = 0;         // 0=Accel, 1=Gyro, 2=Ambos

unsigned long lastSample = 0;

void setup() {
  Serial.begin(115200);
  
  Serial.println("╔════════════════════════════════╗");
  Serial.println("║  OSCILLOSCOPE - RP2350 + IMU  ║");
  Serial.println("╚════════════════════════════════╝");
  
  // Inicializar display
  if (!display.begin()) {
    Serial.println("ERROR: Display no inicializado");
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  Serial.println("Display OK: 640x480");
  
  // Inicializar I2C para BMI270
  Wire.setSDA(8);
  Wire.setSCL(9);
  Wire.begin();
  delay(100);
  
  // Inicializar sensor
  if (imu.beginI2C(i2cAddress) != BMI2_OK) {
    Serial.println("WARNING: BMI270 no conectado");
    Serial.println("Generando señal de prueba...");
    imuAvailable = false;
  } else {
    Serial.println("BMI270 OK");
    imuAvailable = true;
  }
  
  // Inicializar buffers
  for (int i = 0; i < BUFFER_SIZE; i++) {
    accelXBuffer[i] = 0;
    accelYBuffer[i] = 0;
    accelZBuffer[i] = 0;
    gyroXBuffer[i] = 0;
    gyroYBuffer[i] = 0;
    gyroZBuffer[i] = 0;
  }
  
  Serial.println();
  Serial.println("Controles Serial:");
  Serial.println("  p = Pausa/Continuar");
  Serial.println("  1 = Ver Acelerómetro");
  Serial.println("  2 = Ver Giroscopio");
  Serial.println("  3 = Ver Ambos");
  Serial.println("  + = Aumentar escala");
  Serial.println("  - = Reducir escala");
  Serial.println();
  
  display.fillScreen(COLOR_BLACK);
  drawInterface();
  
  lastSample = millis();
}

void loop() {
  // Manejar comandos Serial
  handleCommands();
  
  // Adquirir datos
  if (!paused && (millis() - lastSample >= sampleDelay)) {
    lastSample = millis();
    acquireData();
    drawInterface();
    drawWaveforms();
  }
  
  delay(1);
}

void handleCommands() {
  while (Serial.available()) {
    char cmd = Serial.read();
    
    switch (cmd) {
      case 'p':
      case 'P':
        paused = !paused;
        Serial.print("Estado: ");
        Serial.println(paused ? "PAUSADO" : "CORRIENDO");
        break;
        
      case '1':
        viewMode = 0;
        Serial.println("Modo: Acelerómetro");
        display.fillScreen(COLOR_BLACK);
        break;
        
      case '2':
        viewMode = 1;
        Serial.println("Modo: Giroscopio");
        display.fillScreen(COLOR_BLACK);
        break;
        
      case '3':
        viewMode = 2;
        Serial.println("Modo: Ambos");
        display.fillScreen(COLOR_BLACK);
        break;
        
      case '+':
        accelScale *= 0.5;
        gyroScale *= 0.5;
        Serial.print("Escala aumentada: Accel=±");
        Serial.print(accelScale);
        Serial.print("g, Gyro=±");
        Serial.print(gyroScale);
        Serial.println(" deg/s");
        display.fillScreen(COLOR_BLACK);
        break;
        
      case '-':
        accelScale *= 2.0;
        gyroScale *= 2.0;
        Serial.print("Escala reducida: Accel=±");
        Serial.print(accelScale);
        Serial.print("g, Gyro=±");
        Serial.print(gyroScale);
        Serial.println(" deg/s");
        display.fillScreen(COLOR_BLACK);
        break;
    }
  }
}

void acquireData() {
  if (imuAvailable) {
    // Leer datos reales del IMU
    imu.getSensorData();
    
    accelXBuffer[bufferIndex] = imu.data.accelX;
    accelYBuffer[bufferIndex] = imu.data.accelY;
    accelZBuffer[bufferIndex] = imu.data.accelZ;
    gyroXBuffer[bufferIndex] = imu.data.gyroX;
    gyroYBuffer[bufferIndex] = imu.data.gyroY;
    gyroZBuffer[bufferIndex] = imu.data.gyroZ;
    
  } else {
    // Generar señales de prueba
    float t = millis() / 1000.0;
    accelXBuffer[bufferIndex] = sin(t * 2) * 0.8;
    accelYBuffer[bufferIndex] = sin(t * 3) * 0.6;
    accelZBuffer[bufferIndex] = sin(t * 1.5) * 0.4;
    gyroXBuffer[bufferIndex] = sin(t * 4) * 50;
    gyroYBuffer[bufferIndex] = sin(t * 2.5) * 80;
    gyroZBuffer[bufferIndex] = sin(t * 1.8) * 60;
  }
  
  bufferIndex++;
  if (bufferIndex >= BUFFER_SIZE) {
    bufferIndex = 0;
  }
}

void drawInterface() {
  // Barra superior
  display.fillRect(0, 0, SCREEN_WIDTH, 50, COLOR_DARKGRAY);
  
  display.setTextSize(2);
  display.setTextColor(COLOR_YELLOW, COLOR_DARKGRAY);
  display.setCursor(10, 10);
  display.print("OSCILLOSCOPE");
  
  display.setTextSize(1);
  display.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
  display.setCursor(10, 30);
  if (imuAvailable) {
    display.print("BMI270 Active");
  } else {
    display.print("Test Signal");
  }
  
  // Estado
  display.setCursor(400, 10);
  display.setTextColor(paused ? COLOR_RED : COLOR_GREEN, COLOR_DARKGRAY);
  display.print(paused ? "PAUSED" : "RUNNING");
  
  // Escala
  display.setCursor(400, 25);
  display.setTextColor(COLOR_CYAN, COLOR_DARKGRAY);
  if (viewMode == 0 || viewMode == 2) {
    display.print("A:");
    display.print(accelScale, 1);
    display.print("g ");
  }
  if (viewMode == 1 || viewMode == 2) {
    display.print("G:");
    display.print(gyroScale, 0);
    display.print("d/s");
  }
  
  // Velocidad
  display.setCursor(550, 10);
  display.setTextColor(COLOR_MAGENTA, COLOR_DARKGRAY);
  display.print(1000 / sampleDelay);
  display.print(" Hz");
}

void drawWaveforms() {
  int graphTop1 = GRAPH_TOP;
  int graphTop2 = GRAPH_TOP + GRAPH_HEIGHT + GRAPH_SPACING + 30;
  
  if (viewMode == 0) {
    // Solo acelerómetro
    drawGrid(GRAPH_TOP, GRAPH_HEIGHT, "ACCELEROMETER (g)");
    drawChannel(accelXBuffer, GRAPH_TOP, GRAPH_HEIGHT, accelScale, COLOR_RED, "X");
    drawChannel(accelYBuffer, GRAPH_TOP, GRAPH_HEIGHT, accelScale, COLOR_GREEN, "Y");
    drawChannel(accelZBuffer, GRAPH_TOP, GRAPH_HEIGHT, accelScale, COLOR_BLUE, "Z");
    
  } else if (viewMode == 1) {
    // Solo giroscopio
    drawGrid(GRAPH_TOP, GRAPH_HEIGHT, "GYROSCOPE (deg/s)");
    drawChannel(gyroXBuffer, GRAPH_TOP, GRAPH_HEIGHT, gyroScale, COLOR_RED, "X");
    drawChannel(gyroYBuffer, GRAPH_TOP, GRAPH_HEIGHT, gyroScale, COLOR_GREEN, "Y");
    drawChannel(gyroZBuffer, GRAPH_TOP, GRAPH_HEIGHT, gyroScale, COLOR_BLUE, "Z");
    
  } else {
    // Ambos (mitad de altura cada uno)
    int halfHeight = 80;
    
    drawGrid(graphTop1, halfHeight, "ACCEL (g)");
    drawChannel(accelXBuffer, graphTop1, halfHeight, accelScale, COLOR_RED, "");
    drawChannel(accelYBuffer, graphTop1, halfHeight, accelScale, COLOR_GREEN, "");
    drawChannel(accelZBuffer, graphTop1, halfHeight, accelScale, COLOR_BLUE, "");
    
    drawGrid(graphTop2, halfHeight, "GYRO (d/s)");
    drawChannel(gyroXBuffer, graphTop2, halfHeight, gyroScale, COLOR_RED, "");
    drawChannel(gyroYBuffer, graphTop2, halfHeight, gyroScale, COLOR_GREEN, "");
    drawChannel(gyroZBuffer, graphTop2, halfHeight, gyroScale, COLOR_BLUE, "");
  }
}

void drawGrid(int top, int height, const char* label) {
  int centerY = top + height / 2;
  int left = 30;
  int right = SCREEN_WIDTH - 30;
  
  // Fondo
  display.fillRect(left, top, right - left, height, COLOR_BLACK);
  
  // Línea central
  display.drawLine(left, centerY, right, centerY, COLOR_GRAY);
  
  // Líneas horizontales
  for (int i = 1; i <= 2; i++) {
    int y1 = centerY - (height / 4) * i;
    int y2 = centerY + (height / 4) * i;
    display.drawLine(left, y1, right, y1, COLOR_DARKGRAY);
    display.drawLine(left, y2, right, y2, COLOR_DARKGRAY);
  }
  
  // Líneas verticales
  for (int x = left; x < right; x += 58) {
    display.drawLine(x, top, x, top + height, COLOR_DARKGRAY);
  }
  
  // Borde
  display.drawRect(left, top, right - left, height, COLOR_WHITE);
  
  // Etiqueta
  display.setTextSize(1);
  display.setTextColor(COLOR_YELLOW, COLOR_BLACK);
  display.setCursor(left + 5, top + 5);
  display.print(label);
}

void drawChannel(float* buffer, int top, int height, float scale, uint16_t color, const char* label) {
  int centerY = top + height / 2;
  int left = 30;
  int right = SCREEN_WIDTH - 30;
  int graphWidth = right - left;
  
  int startIdx = bufferIndex;
  
  for (int x = 0; x < graphWidth - 1; x++) {
    int idx1 = (startIdx + x) % BUFFER_SIZE;
    int idx2 = (startIdx + x + 1) % BUFFER_SIZE;
    
    // Mapear valores a píxeles
    int y1 = centerY - (int)((buffer[idx1] / scale) * (height / 2));
    int y2 = centerY - (int)((buffer[idx2] / scale) * (height / 2));
    
    // Limitar a área visible
    y1 = constrain(y1, top, top + height);
    y2 = constrain(y2, top, top + height);
    
    display.drawLine(left + x, y1, left + x + 1, y2, color);
  }
  
  // Etiqueta del canal (solo si se proporciona)
  if (label[0] != '\0') {
    display.setTextSize(1);
    display.setTextColor(color, COLOR_BLACK);
    display.setCursor(right - 15, top + 5);
    display.print(label);
  }
}
