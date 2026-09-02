// Demo de animación 3D simple - Cubo giratorio
// Arduino Nano RP2350 + DVI
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+(Blue), D5(GPIO14)=Data1+(Green), D3(GPIO16)=Data2+(Red)

#include <DevLab_DVIHSTX_Graphics.h>

// DVHSTXPinout pinConfig = {15, 19, 17, 13};
DVHSTXPinout pinConfig = {14, 18, 16, 12};

DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Vértices del cubo
float vertices[8][3] = {
  {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
  {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}
};

// Aristas del cubo (índices de vértices)
int edges[12][2] = {
  {0, 1}, {1, 2}, {2, 3}, {3, 0},  // Cara frontal
  {4, 5}, {5, 6}, {6, 7}, {7, 4},  // Cara trasera
  {0, 4}, {1, 5}, {2, 6}, {3, 7}   // Conexiones
};

float angleX = 0, angleY = 0, angleZ = 0;
int centerX, centerY;
float scale = 40;

void setup() {
  Serial.begin(115200);
  
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  Serial.println("3D Cube Demo - Nano RP2350");
  
  centerX = display.width() / 2;
  centerY = display.height() / 2;
  
  display.fillScreen(0x0000);
  display.setTextColor(0x07E0);
  display.setTextSize(1);
}

void loop() {
  // Limpiar pantalla
  display.fillScreen(0x0000);
  
  // Título
  display.setCursor(5, 5);
  display.setTextColor(0x07E0);
  display.print("3D Rotating Cube Demo");
  
  // Información
  display.setCursor(5, 220);
  display.setTextColor(0xFFFF);
  display.setTextSize(1);
  display.print("RX:");
  display.print((int)angleX);
  display.print(" RY:");
  display.print((int)angleY);
  display.print(" RZ:");
  display.print((int)angleZ);
  
  // Dibujar cubo
  drawCube();
  
  // Actualizar ángulos
  angleX += 1.5;
  angleY += 2.0;
  angleZ += 1.0;
  
  if (angleX >= 360) angleX -= 360;
  if (angleY >= 360) angleY -= 360;
  if (angleZ >= 360) angleZ -= 360;
  
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
    
    // Color basado en la profundidad (simulación simple)
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
