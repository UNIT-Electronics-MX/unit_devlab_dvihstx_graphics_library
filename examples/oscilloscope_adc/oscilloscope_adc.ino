// Osciloscopio Digital - ADC de 1 Canal
// Arduino RP2350 + DVI/HDMI
// Visualiza señal analógica en tiempo real

#include <DevLab_DVIHSTX_Graphics.h>

// Configuración HDMI - Usar pines que funcionan
DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Pin ADC - GPIO 29 (ADC3)
#define ADC_PIN 29

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
#define BUFFER_SIZE 300  // Reducido para 320px ancho
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define GRAPH_TOP 40
#define GRAPH_HEIGHT 160
#define SAMPLES_PER_SCREEN 280

// Buffer para datos
float adcBuffer[BUFFER_SIZE];
int bufferIndex = 0;

// Configuración de visualización
float voltageMax = 3.3;    // Voltaje máximo (RP2350 es 3.3V)
float voltageMin = 0.0;    // Voltaje mínimo
int sampleDelay = 20;      // ms entre muestras (50 Hz) - más lento = menos parpadeo
bool paused = false;
int triggerMode = 0;       // 0=Free run, 1=Rising edge, 2=Falling edge
float triggerLevel = 1.65; // Nivel de trigger (mitad de escala)

unsigned long lastSample = 0;
unsigned long lastDraw = 0;
bool triggered = false;
bool needsFullRedraw = true;

// Estadísticas
float maxVoltage = 0;
float minVoltage = 3.3;
float avgVoltage = 0;
float frequency = 0;

void setup() {
  Serial.begin(115200);
  
  Serial.println("╔════════════════════════════════╗");
  Serial.println("║   OSCILLOSCOPE - ADC CH1      ║");
  Serial.println("╚════════════════════════════════╝");
  
  // PRIMERO: Inicializar display HDMI (crítico para que funcione)
  if (!display.begin()) {
    Serial.println("ERROR: Display no inicializado");
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  Serial.println("Display OK: 320x240");
  
  // SEGUNDO: Configurar ADC después del display
  delay(100);
  analogReadResolution(12);  // 12 bits = 0-4095
  pinMode(ADC_PIN, INPUT);
  
  Serial.print("ADC Pin: GPIO");
  Serial.println(ADC_PIN);
  Serial.println("Resolución: 12 bits (0-4095)");
  
  // Inicializar buffer
  for (int i = 0; i < BUFFER_SIZE; i++) {
    adcBuffer[i] = 0;
  }
  
  Serial.println();
  Serial.println("Controles Serial:");
  Serial.println("  p = Pausa/Continuar");
  Serial.println("  t = Cambiar modo trigger (Free/Rising/Falling)");
  Serial.println("  + = Aumentar escala vertical");
  Serial.println("  - = Reducir escala vertical");
  Serial.println("  f = Más rápido (más muestras/s)");
  Serial.println("  s = Más lento (menos muestras/s)");
  Serial.println("  r = Reset estadísticas");
  Serial.println();
  
  display.fillScreen(COLOR_BLACK);
  drawGrid();
  drawInterface();
  
  lastSample = millis();
  lastDraw = millis();
}

void loop() {
  // Manejar comandos Serial
  handleCommands();
  
  // Adquirir datos continuamente
  if (!paused && (millis() - lastSample >= sampleDelay)) {
    lastSample = millis();
    acquireData();
  }
  
  // Actualizar pantalla solo cada 50ms (20 FPS) para reducir parpadeo
  if (millis() - lastDraw >= 50) {
    lastDraw = millis();
    
    if (!paused) {
      updateStats();
      
      // Solo redibujar todo si es necesario
      if (needsFullRedraw) {
        display.fillScreen(COLOR_BLACK);
        drawGrid();
        needsFullRedraw = false;
      }
      
      drawWaveformOptimized();
      drawInterfaceOptimized();
    }
  }
  
  delay(1);
}

void handleCommands() {
  while (Serial.available()) {
    char cmd = Serial.read();
    
    switch (cmd) {
      case 'p':
      case 'P':
        needsFullRedraw = true;
        Serial.print("Estado: ");
        Serial.println(paused ? "PAUSADO" : "CORRIENDO");
        break;
        
      case 't':
      case 'T':
        triggerMode = (triggerMode + 1) % 3;
        Serial.print("Trigger: ");
        if (triggerMode == 0) Serial.println("FREE RUN");
        else if (triggerMode == 1) Serial.println("RISING EDGE");
        else Serial.println("FALLING EDGE");
        triggered = false;
        needsFullRedraw = true;
        break;
        
      case '+':
        voltageMax *= 0.8;
        if (voltageMax < 0.5) voltageMax = 0.5;
        Serial.print("Escala: 0 - ");
        Serial.print(voltageMax);
        Serial.println(" V");
        needsFullRedraw = true;
        break;
        
      case '-':
        voltageMax *= 1.25;
        if (voltageMax > 3.3) voltageMax = 3.3;
        Serial.print("Escala: 0 - ");
        Serial.print(voltageMax);
        Serial.println(" V");
        needsFullRedraw = true;
        display.fillScreen(COLOR_BLACK);
        break;
        
      case 'f':
      case 'F':
        sampleDelay--;
        if (sampleDelay < 1) sampleDelay = 1;
        Serial.print("Velocidad: ");
        Serial.print(1000 / sampleDelay);
        Serial.println(" Hz");
        break;
        
      case 's':
      case 'S':
        sampleDelay++;
        if (sampleDelay > 100) sampleDelay = 100;
        Serial.print("Velocidad: ");
        Serial.print(1000 / sampleDelay);
        Serial.println(" Hz");
        break;
        
      case 'r':
      case 'R':
        maxVoltage = 0;
        minVoltage = 3.3;
        Serial.println("Estadísticas reseteadas");
        break;
    }
  }
}

bool checkTrigger() {
  static float lastValue = 0;
  
  // Leer valor actual
  int adcValue = analogRead(ADC_PIN);
  float voltage = (adcValue / 4095.0) * 3.3;
  
  bool result = false;
  
  if (triggerMode == 1) {
    // Rising edge: pasar de abajo a arriba del nivel
    if (lastValue < triggerLevel && voltage >= triggerLevel) {
      result = true;
      triggered = true;
    }
  } else if (triggerMode == 2) {
    // Falling edge: pasar de arriba a abajo del nivel
    if (lastValue > triggerLevel && voltage <= triggerLevel) {
      result = true;
      triggered = true;
    }
  }
  
  lastValue = voltage;
  return result;
}

void acquireData() {
  // Leer ADC
  int adcValue = analogRead(ADC_PIN);
  float voltage = (adcValue / 4095.0) * 3.3;
  
  adcBuffer[bufferIndex] = voltage;
  
  bufferIndex++;
  if (bufferIndex >= BUFFER_SIZE) {
    bufferIndex = 0;
  }
}

void updateStats() {
  float sum = 0;
  maxVoltage = 0;
  minVoltage = 3.3;
  
  for (int i = 0; i < BUFFER_SIZE; i++) {
    sum += adcBuffer[i];
    if (adcBuffer[i] > maxVoltage) maxVoltage = adcBuffer[i];
    if (adcBuffer[i] < minVoltage) minVoltage = adcBuffer[i];
  }
  
  avgVoltage = sum / BUFFER_SIZE;
  
  // Calcular frecuencia aproximada (contando cruces por cero)
  int zeroCrossings = 0;
  for (int i = 1; i < BUFFER_SIZE; i++) {
    if ((adcBuffer[i-1] < avgVoltage && adcBuffer[i] >= avgVoltage) ||
        (adcBuffer[i-1] > avgVoltage && adcBuffer[i] <= avgVoltage)) {
      zeroCrossings++;
    }
  }
  
  float timeSpan = (BUFFER_SIZE * sampleDelay) / 1000.0; // segundos
  frequency = (zeroCrossings / 2.0) / timeSpan;
}

void drawInterface() {
  // Barra superior
  display.fillRect(0, 0, SCREEN_WIDTH, 50, COLOR_DARKGRAY);
  
  display.setTextSize(2);
  display.setTextColor(COLOR_YELLOW, COLOR_DARKGRAY);
  display.setCursor(10, 10);
  display.print("ADC OSCILLOSCOPE - CH1");
  
  display.setTextSize(1);
  display.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
  display.setCursor(10, 30);
  display.print("Pin: A");
  display.print(ADC_PIN - A0);
  
  // Estado
  display.setCursor(400, 10);
  display.setTextColor(paused ? COLOR_RED : COLOR_GREEN, COLOR_DARKGRAY);
  display.print(paused ? "PAUSED" : "RUNNING");
  
  // Trigger
  display.setCursor(400, 25);
  display.setTextColor(COLOR_CYAN, COLOR_DARKGRAY);
  if (triggerMode == 0) display.print("FREE");
  else if (triggerMode == 1) display.print("TRIG^");
  else display.print("TRIGv");
  
  // Velocidad y escala
  display.setCursor(500, 10);
  display.setTextColor(COLOR_MAGENTA, COLOR_DARKGRAY);
  display.print(1000 / sampleDelay);
  display.print(" Hz");
  
  display.setCursor(500, 25);
  display.setTextColor(COLOR_YELLOW, COLOR_DARKGRAY);
  display.print(voltageMax, 1);
  display.print(" V/div");
  
  // Estadísticas en la parte inferior
  int statsY = GRAPH_TOP + GRAPH_HEIGHT + 20;
  
  display.fillRect(0, statsY, SCREEN_WIDTH, 50, COLOR_BLACK);
  
  display.setTextSize(1);
  display.setTextColor(COLOR_GREEN, COLOR_BLACK);
  display.setCursor(50, statsY + 5);
  display.print("MAX: ");
  display.print(maxVoltage, 3);
  display.print(" V");
  
  display.setTextColor(COLOR_BLUE, COLOR_BLACK);
  display.setCursor(200, statsY + 5);
  display.print("MIN: ");
  display.print(minVoltage, 3);
  display.print(" V");
  
  display.setTextColor(COLOR_YELLOW, COLOR_BLACK);
  display.setCursor(350, statsY + 5);
  display.print("AVG: ");
  display.print(avgVoltage, 3);
  display.print(" V");
  
  display.setTextColor(COLOR_MAGENTA, COLOR_BLACK);
  display.setCursor(500, statsY + 5);
  display.print("FREQ: ");
  display.print(frequency, 1);
  display.print(" Hz");
  
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.setCursor(50, statsY + 25);
  display.print("Vpp: ");
  display.print(maxVoltage - minVoltage, 3);
  display.print(" V");
}

void drawWaveform() {
  int centerY = GRAPH_TOP + GRAPH_HEIGHT / 2;
  int left = 30;
  int right = SCREEN_WIDTH - 30;
  int graphWidth = right - left;
  
  // Limpiar área de gráfico
  display.fillRect(left, GRAPH_TOP, graphWidth, GRAPH_HEIGHT, COLOR_BLACK);
  
  // Dibujar cuadrícula
  drawGrid();
  
  // Dibujar forma de onda
  int startIdx = bufferIndex;
  
  for (int x = 0; x < graphWidth - 1; x++) {
    int idx1 = (startIdx + x) % BUFFER_SIZE;
    int idx2 = (startIdx + x + 1) % BUFFER_SIZE;
    
    // Mapear voltaje a píxeles
    int y1 = GRAPH_TOP + GRAPH_HEIGHT - (int)((adcBuffer[idx1] / voltageMax) * GRAPH_HEIGHT);
    int y2 = GRAPH_TOP + GRAPH_HEIGHT - (int)((adcBuffer[idx2] / voltageMax) * GRAPH_HEIGHT);
    
    // Limitar a área visible
    y1 = constrain(y1, GRAPH_TOP, GRAPH_TOP + GRAPH_HEIGHT);
    y2 = constrain(y2, GRAPH_TOP, GRAPH_TOP + GRAPH_HEIGHT);
    
    display.drawLine(left + x, y1, left + x + 1, y2, COLOR_YELLOW);
  }
  
  // Dibujar nivel de trigger si está activo
  if (triggerMode > 0) {
    int triggerY = GRAPH_TOP + GRAPH_HEIGHT - (int)((triggerLevel / voltageMax) * GRAPH_HEIGHT);
    if (triggerY >= GRAPH_TOP && triggerY <= GRAPH_TOP + GRAPH_HEIGHT) {
      for (int x = left; x < right; x += 10) {
        display.drawPixel(x, triggerY, COLOR_RED);
        display.drawPixel(x + 1, triggerY, COLOR_RED);
      }
    }
  }
}

void drawGrid() {
  int centerY = GRAPH_TOP + GRAPH_HEIGHT / 2;
  int left = 30;
  int right = SCREEN_WIDTH - 30;
  
  // Línea central (0V o mitad de escala)
  display.drawLine(left, centerY, right, centerY, COLOR_GRAY);
  
  // Líneas horizontales (divisiones de voltaje)
  for (int i = 1; i <= 4; i++) {
    int y1 = GRAPH_TOP + (GRAPH_HEIGHT / 5) * i;
    display.drawLine(left, y1, right, y1, COLOR_DARKGRAY);
  }
  
  // Líneas verticales (divisiones de tiempo)
  for (int x = left; x < right; x += 58) {
    display.drawLine(x, GRAPH_TOP, x, GRAPH_TOP + GRAPH_HEIGHT, COLOR_DARKGRAY);
  }
  
  // Borde
  display.drawRect(left, GRAPH_TOP, right - left, GRAPH_HEIGHT, COLOR_WHITE);
  
  // Etiquetas de voltaje
  display.setTextSize(1);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  
  for (int i = 0; i <= 5; i++) {
    float v = voltageMax - (voltageMax / 5) * i;
    int y = GRAPH_TOP + (GRAPH_HEIGHT / 5) * i;
    display.setCursor(5, y - 4);
    display.print(v, 1);
  }
}

// Variables estáticas para tracking de forma de onda anterior
static int lastWaveformY[300];  // Guardar últimos Y de la forma de onda
static bool firstDraw = true;

void drawWaveformOptimized() {
  int left = 30;
  int right = SCREEN_WIDTH - 30;
  int graphWidth = right - left;
  
  // Borrar forma de onda anterior (solo las líneas, no todo)
  if (!firstDraw) {
    for (int x = 0; x < graphWidth - 1 && x < 300; x++) {
      int y = lastWaveformY[x];
      if (y > 0) {
        // Borrar línea anterior con negro
        display.drawPixel(left + x, y, COLOR_BLACK);
        display.drawPixel(left + x, y-1, COLOR_BLACK);
        display.drawPixel(left + x, y+1, COLOR_BLACK);
      }
    }
  }
  
  // Dibujar nueva forma de onda
  int startIdx = bufferIndex;
  
  for (int x = 0; x < graphWidth - 1 && x < 300; x++) {
    int idx1 = (startIdx + x) % BUFFER_SIZE;
    int idx2 = (startIdx + x + 1) % BUFFER_SIZE;
    
    // Mapear voltaje a píxeles
    int y1 = GRAPH_TOP + GRAPH_HEIGHT - (int)((adcBuffer[idx1] / voltageMax) * GRAPH_HEIGHT);
    int y2 = GRAPH_TOP + GRAPH_HEIGHT - (int)((adcBuffer[idx2] / voltageMax) * GRAPH_HEIGHT);
    
    // Limitar a área visible
    y1 = constrain(y1, GRAPH_TOP, GRAPH_TOP + GRAPH_HEIGHT);
    y2 = constrain(y2, GRAPH_TOP, GRAPH_TOP + GRAPH_HEIGHT);
    
    // Dibujar línea gruesa para mejor visibilidad
    display.drawLine(left + x, y1, left + x + 1, y2, COLOR_YELLOW);
    
    // Guardar para borrar después
    lastWaveformY[x] = y1;
  }
  
  // Dibujar nivel de trigger si está activo
  if (triggerMode > 0) {
    int triggerY = GRAPH_TOP + GRAPH_HEIGHT - (int)((triggerLevel / voltageMax) * GRAPH_HEIGHT);
    if (triggerY >= GRAPH_TOP && triggerY <= GRAPH_TOP + GRAPH_HEIGHT) {
      for (int x = left; x < right; x += 8) {
        display.drawPixel(x, triggerY, COLOR_RED);
      }
    }
  }
  
  firstDraw = false;
}

void drawInterfaceOptimized() {
  // Solo actualizar estadísticas (área pequeña)
  int statsY = GRAPH_TOP + GRAPH_HEIGHT + 15;
  
  // Borrar área de stats
  display.fillRect(0, statsY, SCREEN_WIDTH, 40, COLOR_BLACK);
  
  display.setTextSize(1);
  
  // Estadísticas compactas para 320px
  display.setTextColor(COLOR_GREEN, COLOR_BLACK);
  display.setCursor(5, statsY + 5);
  display.print("MAX:");
  display.print(maxVoltage, 2);
  display.print("V");
  
  display.setTextColor(COLOR_BLUE, COLOR_BLACK);
  display.setCursor(90, statsY + 5);
  display.print("MIN:");
  display.print(minVoltage, 2);
  display.print("V");
  
  display.setTextColor(COLOR_YELLOW, COLOR_BLACK);
  display.setCursor(170, statsY + 5);
  display.print("AVG:");
  display.print(avgVoltage, 2);
  display.print("V");
  
  display.setTextColor(COLOR_MAGENTA, COLOR_BLACK);
  display.setCursor(5, statsY + 20);
  display.print("Vpp:");
  display.print(maxVoltage - minVoltage, 2);
  display.print("V");
  
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.setCursor(90, statsY + 20);
  display.print("F:");
  display.print(frequency, 1);
  display.print("Hz");
  
  // Estado
  display.setCursor(170, statsY + 20);
  display.setTextColor(paused ? COLOR_RED : COLOR_GREEN, COLOR_BLACK);
  display.print(paused ? "PAUSE" : "RUN");
}
