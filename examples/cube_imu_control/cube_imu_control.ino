// Demo de cubo 3D controlado por IMU (BMI270)
// Arduino Nano RP2350 + DVI + BMI270
// Control del cubo con acelerómetro/giroscopio

#include <Wire.h>
#include <DevLab_DVIHSTX_Graphics.h>
#include "SparkFun_BMI270_Arduino_Library.h"

// Configuración HDMI
DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Sensor BMI270
BMI270 imu;
uint8_t i2cAddress = BMI2_I2C_PRIM_ADDR + 1; // 0x69

// Vértices del cubo
float vertices[8][3] = {
  {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
  {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}
};

// Aristas del cubo
int edges[12][2] = {
  {0, 1}, {1, 2}, {2, 3}, {3, 0},  // Cara frontal
  {4, 5}, {5, 6}, {6, 7}, {7, 4},  // Cara trasera
  {0, 4}, {1, 5}, {2, 6}, {3, 7}   // Conexiones
};

float angleX = 0, angleY = 0, angleZ = 0;
int centerX, centerY;
float scale = 40;

// Modo de control
bool imuControl = true;  // true = IMU, false = auto-rotación

void setup() {
  Serial.begin(115200);
  
  Serial.println("3D Cube IMU Control - RP2350");
  
  // PRIMERO: Inicializar display HDMI
  if (!display.begin()) {
    Serial.println("ERROR: Display no inicializado");
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  Serial.println("Display OK: 320x240");
  
  centerX = display.width() / 2;
  centerY = display.height() / 2;
  
  // SEGUNDO: Inicializar I2C para BMI270 (pines diferentes)
  Wire.setSDA(8);
  Wire.setSCL(9);
  Wire.begin();
  
  delay(100);
  
  // Inicializar sensor BMI270
  if (imu.beginI2C(i2cAddress) != BMI2_OK) {
    Serial.println("WARNING: BMI270 no conectado");
    Serial.println("Continuando con rotación automática");
    imuControl = false;
  } else {
    Serial.println("BMI270 OK - Control IMU activado");
    imuControl = true;
  }
  
  display.fillScreen(0x0000);
  display.setTextSize(1);
}

void loop() {
  // Limpiar pantalla
  display.fillScreen(0x0000);
  
  // Actualizar ángulos
  if (imuControl) {
    // Modo IMU: Leer sensor y controlar rotación
    imu.getSensorData();
    
    // Usar giroscopio para controlar rotación
    // Invertir/ajustar ejes según orientación deseada
    angleX += imu.data.gyroX * 0.5;  // Escalar velocidad
    angleY += imu.data.gyroY * 0.5;
    angleZ += imu.data.gyroZ * 0.5;
    
    // Mantener ángulos en rango 0-360
    if (angleX >= 360) angleX -= 360;
    if (angleX < 0) angleX += 360;
    if (angleY >= 360) angleY -= 360;
    if (angleY < 0) angleY += 360;
    if (angleZ >= 360) angleZ -= 360;
    if (angleZ < 0) angleZ += 360;
    
    // Título
    display.setCursor(5, 5);
    display.setTextColor(0x07FF); // Cyan
    display.print("IMU Control - BMI270");
    
    // Mostrar datos del sensor
    display.setCursor(5, 220);
    display.setTextColor(0xFFFF);
    display.print("GX:");
    display.print((int)imu.data.gyroX);
    display.print(" GY:");
    display.print((int)imu.data.gyroY);
    display.print(" GZ:");
    display.print((int)imu.data.gyroZ);
    
  } else {
    // Modo auto-rotación
    angleX += 1.5;
    angleY += 2.0;
    angleZ += 1.0;
    
    if (angleX >= 360) angleX -= 360;
    if (angleY >= 360) angleY -= 360;
    if (angleZ >= 360) angleZ -= 360;
    
    // Título
    display.setCursor(5, 5);
    display.setTextColor(0xF800); // Rojo
    display.print("Auto Mode - No IMU");
    
    // Información
    display.setCursor(5, 220);
    display.setTextColor(0xFFFF);
    display.print("RX:");
    display.print((int)angleX);
    display.print(" RY:");
    display.print((int)angleY);
    display.print(" RZ:");
    display.print((int)angleZ);
  }
  
  // Dibujar cubo
  drawCube();
  
  delay(30);
}

void drawCube() {
  // Array para vértices proyectados
  int projected[8][2];
  
  // Proyectar cada vértice
  for (int i = 0; i < 8; i++) {
    float x = vertices[i][0];
    float y = vertices[i][1];
    float z = vertices[i][2];
    
    // Rotar en X
    float rad = angleX * PI / 180;
    float y1 = y * cos(rad) - z * sin(rad);
    float z1 = y * sin(rad) + z * cos(rad);
    
    // Rotar en Y
    rad = angleY * PI / 180;
    float x2 = x * cos(rad) + z1 * sin(rad);
    float z2 = -x * sin(rad) + z1 * cos(rad);
    
    // Rotar en Z
    rad = angleZ * PI / 180;
    float x3 = x2 * cos(rad) - y1 * sin(rad);
    float y3 = x2 * sin(rad) + y1 * cos(rad);
    
    // Proyección ortográfica
    projected[i][0] = centerX + (int)(x3 * scale);
    projected[i][1] = centerY + (int)(y3 * scale);
  }
  
  // Dibujar aristas
  for (int i = 0; i < 12; i++) {
    int v1 = edges[i][0];
    int v2 = edges[i][1];
    
    // Color basado en la profundidad
    uint16_t color;
    if (i < 4) color = 0xF800;      // Rojo - cara frontal
    else if (i < 8) color = 0x001F; // Azul - cara trasera
    else color = 0x07E0;             // Verde - conexiones
    
    display.drawLine(
      projected[v1][0], projected[v1][1],
      projected[v2][0], projected[v2][1],
      color
    );
  }
  
  // Dibujar vértices
  for (int i = 0; i < 8; i++) {
    display.fillCircle(projected[i][0], projected[i][1], 2, 0xFFFF);
  }
}
