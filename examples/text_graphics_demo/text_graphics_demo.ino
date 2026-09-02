// Demo de texto y gráficos combinados
// Arduino Nano RP2350 + DVI
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+(Blue), D5(GPIO14)=Data1+(Green), D3(GPIO16)=Data2+(Red)

#include <DevLab_DVIHSTX_Graphics.h>

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

struct SystemInfo {
  unsigned long uptime;
  int fps;
  int freeMemory;
};

SystemInfo sysInfo;
unsigned long lastFrame = 0;
int frameCounter = 0;
unsigned long lastFpsUpdate = 0;

void setup() {
  Serial.begin(115200);
  
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  Serial.println("Text & Graphics Demo - Nano RP2350");
  
  display.fillScreen(0x0000);
  display.setTextSize(1);
  display.setTextColor(0xFFFF);
  
  drawUI();
}

void loop() {
  // Actualizar FPS
  frameCounter++;
  if (millis() - lastFpsUpdate >= 1000) {
    sysInfo.fps = frameCounter;
    frameCounter = 0;
    lastFpsUpdate = millis();
  }
  
  sysInfo.uptime = millis() / 1000;
  
  // Actualizar display
  updateDisplay();
  
  delay(16); // ~60 FPS
}

void drawUI() {
  // Título
  display.fillRect(0, 0, display.width(), 20, 0x001F); // Azul
  display.setCursor(5, 6);
  display.setTextColor(0xFFFF);
  display.setTextSize(1);
  display.print("Arduino Nano RP2350 - DVI Demo");
  
  // Líneas divisoras
  display.drawFastHLine(0, 20, display.width(), 0xFFFF);
  display.drawFastHLine(0, 210, display.width(), 0xFFFF);
  
  // Barra de estado
  display.fillRect(0, 211, display.width(), 29, 0x1082); // Gris oscuro
}

void updateDisplay() {
  // Limpiar área de contenido
  display.fillRect(1, 21, display.width() - 2, 188, 0x0000);
  
  // Dibujar información del sistema
  display.setCursor(10, 30);
  display.setTextColor(0x07E0); // Verde
  display.setTextSize(1);
  display.print("SYSTEM INFORMATION");
  
  display.setCursor(10, 50);
  display.setTextColor(0xFFFF);
  display.print("Uptime: ");
  display.print(sysInfo.uptime);
  display.print(" s");
  
  display.setCursor(10, 65);
  display.print("FPS: ");
  display.print(sysInfo.fps);
  
  display.setCursor(10, 80);
  display.print("Resolution: ");
  display.print(display.width());
  display.print("x");
  display.print(display.height());
  
  // Gráfico de barras animado
  display.setCursor(10, 100);
  display.setTextColor(0xF800); // Rojo
  display.print("AUDIO SPECTRUM (DEMO)");
  
  int barWidth = 8;
  int barSpacing = 2;
  int numBars = (display.width() - 20) / (barWidth + barSpacing);
  int baseY = 180;
  
  for (int i = 0; i < numBars; i++) {
    // Simular espectro de audio
    int height = random(10, 80) * sin((millis() / 100.0) + i * 0.5) * 0.5 + 40;
    height = constrain(height, 5, 60);
    
    // Color basado en altura
    uint16_t color;
    if (height < 20) color = 0x07E0; // Verde
    else if (height < 40) color = 0xFFE0; // Amarillo
    else color = 0xF800; // Rojo
    
    int x = 10 + i * (barWidth + barSpacing);
    display.fillRect(x, baseY - height, barWidth, height, color);
  }
  
  // Barra de progreso animada
  display.setCursor(10, 195);
  display.setTextColor(0x07FF); // Cyan
  display.print("PROGRESS");
  
  int progress = (millis() / 100) % 100;
  int barLength = (display.width() - 100) * progress / 100;
  
  display.drawRect(80, 193, display.width() - 90, 12, 0xFFFF);
  display.fillRect(81, 194, barLength, 10, 0x07FF);
  
  // Información en barra de estado
  display.fillRect(1, 212, display.width() - 2, 27, 0x1082);
  display.setCursor(5, 220);
  display.setTextColor(0xFFFF);
  display.setTextSize(1);
  display.print("GPIO: 14,18,16,12 | FPS:");
  display.print(sysInfo.fps);
  display.print(" | CPU:264MHz");
}
