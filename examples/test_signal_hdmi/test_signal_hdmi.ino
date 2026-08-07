// Test de señal HDMI intensivo
// Si el display inicializa pero no hay imagen, es problema de señal física

#include <UDVI_HSTX.h>

// NOTA: Cambia el índice pinConfigIndex para probar diferentes configuraciones
// El pin 18 siempre es el CLOCK (fijo)
// Solo cambia el orden de los pines RGB: 12, 14, 16
// 
// 0 = {18, {12, 14, 16}} - R=12, G=14, B=16
// 1 = {18, {12, 16, 14}} - R=12, G=16, B=14
// 2 = {18, {14, 12, 16}} - R=14, G=12, B=16
// 3 = {18, {14, 16, 12}} - R=14, G=16, B=12
// 4 = {18, {16, 12, 14}} - R=16, G=12, B=14
// 5 = {18, {16, 14, 12}} - R=16, G=14, B=12

int pinConfigIndex = 6;  // CAMBIA ESTE NÚMERO DE 0 A 5 HASTA QUE LOS COLORES SEAN CORRECTOS

DVHSTXPinout pinConfigs[] = {
  {18, {12, 14, 16}},  // 0: RGB = 12,14,16
  {18, {12, 16, 14}},  // 1: RGB = 12,16,14
  {18, {14, 12, 16}},  // 2: RGB = 14,12,16
  {18, {14, 16, 12}},  // 3: RGB = 14,16,12
  {18, {16, 12, 14}},  // 4: RGB = 16,12,14
  {18, {16, 14, 12}},   // 5: RGB = 16,14,12
  {14,  18, 16, 12}
};

DVHSTX16 display(pinConfigs[pinConfigIndex], DVHSTX_RESOLUTION_320x240);

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("\n================================================");
  Serial.println("  TEST DE SEÑAL HDMI - DIAGNOSTICO DE PINES");
  Serial.println("================================================\n");
  
  Serial.print("Probando configuracion #");
  Serial.println(pinConfigIndex);
  Serial.print("Pines: {CLK=");
  Serial.print(pinConfigs[pinConfigIndex].clk_p);
  Serial.print(", RGB=[");
  Serial.print(pinConfigs[pinConfigIndex].rgb_p[0]);
  Serial.print(",");
  Serial.print(pinConfigs[pinConfigIndex].rgb_p[1]);
  Serial.print(",");
  Serial.print(pinConfigs[pinConfigIndex].rgb_p[2]);
  Serial.println("]}");
  Serial.println();
  
  if (!display.begin()) {
    Serial.println("ERROR: No inicializa");
    pinMode(LED_BUILTIN, OUTPUT);
    while(1) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(100);
    }
  }
  
  Serial.println("✓ Display inicializado: 320x240\n");
  Serial.println("APLICANDO INVERSION DE COLORES...\n");
  Serial.println("Tu hardware invierte los bits de color.");
  Serial.println("Aplicando compensacion en software.\n");
  
  // Test completo de patrones
  testPatterns();
  
  Serial.println("\n✓ Tests completados!");
  Serial.println("Mostrando pantalla final...\n");
}

void loop() {
  // Reiniciar tests continuamente
  testPatterns();
}

void testPatterns() {
  // Test de barras de colores con nombres
  Serial.println("Test 1/7: BARRAS DE COLORES");
  testColorBars();
  delay(5000);
  
  // Test de caracteres ASCII
  Serial.println("Test 2/7: CARACTERES ASCII COMPLETOS");
  testASCII();
  delay(4000);
  
  Serial.println("Test 3/7: BLANCO MAXIMO");
  display.fillScreen((uint16_t)0xFFFF);
  drawBigText("BLANCO", (uint16_t)0x0000);
  delay(2000);
  
  Serial.println("Test 4/7: ROJO PURO");
  display.fillScreen((uint16_t)0xF800);
  drawBigText("ROJO", (uint16_t)0xFFFF);
  delay(2000);
  
  Serial.println("Test 5/7: VERDE PURO");
  display.fillScreen((uint16_t)0x07E0);
  drawBigText("VERDE", (uint16_t)0xFFFF);
  delay(2000);
  
  Serial.println("Test 6/7: AZUL PURO");
  display.fillScreen((uint16_t)0x001F);
  drawBigText("AZUL", (uint16_t)0xFFFF);
  delay(2000);
  
  Serial.println("Test 7/7: TABLERO AJEDREZ");
  int sq = 40;
  for(int y = 0; y < display.height(); y += sq) {
    for(int x = 0; x < display.width(); x += sq) {
      uint16_t c = ((x/sq + y/sq) % 2) ? (uint16_t)0xFFFF : (uint16_t)0x0000;
      display.fillRect(x, y, sq, sq, c);
    }
  }
  delay(2000);
  
  Serial.println("\nTests completados! Reiniciando...\n");
}

void testASCII() {
  display.fillScreen((uint16_t)0x0000);  // Fondo negro (invertido a blanco)
  display.setTextSize(1);
  
  int x = 5, y = 5;
  int lineHeight = 10;
  
  // Título
  display.setCursor(x, y);
  display.setTextColor((uint16_t)0xFFFF, (uint16_t)0x0000);
  display.print("TEST ASCII RP2350");
  y += lineHeight * 2;
  
  // Números
  display.setCursor(x, y);
  display.setTextColor((uint16_t)0xF81F, (uint16_t)0x0000); // Magenta invertido
  display.print("NUM: 0123456789");
  y += lineHeight;
  
  // Letras mayúsculas
  display.setCursor(x, y);
  display.setTextColor((uint16_t)0x07FF, (uint16_t)0x0000); // Cyan invertido
  display.print("ABC: ABCDEFGHIJKLM");
  y += lineHeight;
  display.setCursor(x, y);
  display.print("     NOPQRSTUVWXYZ");
  y += lineHeight;
  
  // Letras minúsculas
  display.setCursor(x, y);
  display.setTextColor((uint16_t)0xFFE0, (uint16_t)0x0000); // Amarillo invertido
  display.print("abc: abcdefghijklm");
  y += lineHeight;
  display.setCursor(x, y);
  display.print("     nopqrstuvwxyz");
  y += lineHeight * 2;
  
  // Símbolos comunes
  display.setCursor(x, y);
  display.setTextColor((uint16_t)0xF800, (uint16_t)0x0000); // Rojo invertido
  display.print("!@#$%^&*()_+-=[]{}");
  y += lineHeight;
  display.setCursor(x, y);
  display.print("\\|;:'\",.<>?/`~");
  y += lineHeight * 2;
  
  // Tabla ASCII en columnas
  display.setTextColor((uint16_t)0x07E0, (uint16_t)0x0000); // Verde invertido
  display.setCursor(x, y);
  display.print("Tabla ASCII:");
  y += lineHeight;
  
  display.setTextColor((uint16_t)0xFFFF, (uint16_t)0x0000);
  int col = 0;
  for (int i = 33; i <= 126; i++) {
    int xPos = x + (col * 60);
    display.setCursor(xPos, y);
    
    if (i < 100) display.print(" ");
    display.print(i);
    display.print(":");
    display.print((char)i);
    
    col++;
    if (col >= 5) {
      col = 0;
      y += lineHeight;
      if (y > display.height() - lineHeight * 3) break;
    }
  }
  
  // Frases pangrama
  y = display.height() - lineHeight * 2;
  display.setCursor(x, y);
  display.setTextColor((uint16_t)0x07FF, (uint16_t)0x0000);
  display.print("Quick Brown Fox!");
  y += lineHeight;
  display.setCursor(x, y);
  display.print("Pack my 5 dozen jugs.");
}

void drawBigText(String text, uint16_t color) {
  display.setTextSize(3);
  display.setTextColor(color, (uint16_t)0x0000);
  
  // Centrar texto aproximadamente
  int textLen = text.length();
  int charWidth = 18;
  int x = (display.width() - (textLen * charWidth)) / 2;
  int y = (display.height() - 24) / 2;
  
  display.setCursor(x, y);
  display.print(text);
  
  // Marco grueso
  for(int i = 0; i < 10; i++) {
    display.drawRect(i, i, display.width()-i*2, display.height()-i*2, color);
  }
}

void testColorBars() {
  display.fillScreen(0x0000);
  
  // Definir colores INVERTIDOS (porque el hardware invierte los bits)
  struct ColorBar {
    uint16_t color;
    const char* name;
  };
  
  ColorBar bars[] = {
    {(uint16_t)0xFFFF, "BLANCO"},   // Invertido: Negro -> Blanco
    {(uint16_t)0xFFE0, "AMARILLO"}, // Invertido
    {(uint16_t)0x07FF, "CYAN"},     // Invertido
    {(uint16_t)0x07E0, "VERDE"},    // Invertido
    {(uint16_t)0xF81F, "MAGENTA"},  // Invertido
    {(uint16_t)0xF800, "ROJO"},     // Invertido
    {(uint16_t)0x001F, "AZUL"},     // Invertido
    {(uint16_t)0x0000, "NEGRO"}     // Invertido: Blanco -> Negro
  };
  
  int numBars = 8;
  int barWidth = display.width() / numBars;
  int barHeight = display.height();
  
  // Dibujar barras verticales
  for (int i = 0; i < numBars; i++) {
    int x = i * barWidth;
    display.fillRect(x, 0, barWidth, barHeight, bars[i].color);
    
    // Escribir nombre del color
    display.setTextSize(1);
    
    // Color del texto (invertido también)
    uint16_t textColor = (i == 0 || i == 1 || i == 2 || i == 3) ? (uint16_t)0x0000 : (uint16_t)0xFFFF;
    display.setTextColor(textColor, bars[i].color);
    
    // Posición del texto (centrado en la barra)
    int textX = x + (barWidth - (strlen(bars[i].name) * 6)) / 2;
    int textY = barHeight / 2 - 4;
    
    display.setCursor(textX, textY);
    display.print(bars[i].name);
  }
  
  // Título en la parte superior
  display.fillRect(0, 0, display.width(), 12, (uint16_t)0x0000);
  display.setTextSize(1);
  display.setTextColor((uint16_t)0xFFFF, (uint16_t)0x0000);
  display.setCursor(5, 2);
  display.print("COLORES INVERTIDOS");
}
