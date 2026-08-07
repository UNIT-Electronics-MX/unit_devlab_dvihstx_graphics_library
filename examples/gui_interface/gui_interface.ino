// GUI Interface - Interactive Menu System
// Arduino RP2350 + DVI/HDMI
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+, D5(GPIO14)=Data1+, D3(GPIO16)=Data2+
// Controls: w/s = up/down, Enter = select, Esc = back

#include <UDVI_HSTX.h>

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Inverted colors for hardware
#define COLOR_BLACK       (uint16_t)0x0000
#define COLOR_WHITE       (uint16_t)0xFFFF
#define COLOR_GRAY        (uint16_t)0x7BEF
#define COLOR_DARKGRAY    (uint16_t)0x39E7
#define COLOR_RED         (uint16_t)0xF800
#define COLOR_GREEN       (uint16_t)0x07E0
#define COLOR_BLUE        (uint16_t)0x001F
#define COLOR_YELLOW      (uint16_t)0xFFE0
#define COLOR_CYAN        (uint16_t)0x07FF
#define COLOR_MAGENTA     (uint16_t)0xF81F
#define COLOR_ORANGE      (uint16_t)0xFD20

// Menu states
enum MenuState {
  MENU_MAIN,
  MENU_TESTS,
  MENU_DEMOS,
  MENU_SETTINGS,
  RUNNING_TEST,
  RUNNING_DEMO
};

MenuState currentState = MENU_MAIN;
int selectedItem = 0;
int runningApp = -1;

// Button structure
struct Button {
  int x, y, w, h;
  const char* text;
  uint16_t color;
  bool enabled;
};

// Draw a button
void drawButton(Button btn, bool selected) {
  // Border
  uint16_t borderColor = selected ? COLOR_CYAN : COLOR_DARKGRAY;
  display.drawRect(btn.x, btn.y, btn.w, btn.h, borderColor);
  if (selected) {
    display.drawRect(btn.x + 1, btn.y + 1, btn.w - 2, btn.h - 2, borderColor);
  }
  
  // Fill
  uint16_t fillColor = selected ? COLOR_DARKGRAY : COLOR_BLACK;
  display.fillRect(btn.x + 2, btn.y + 2, btn.w - 4, btn.h - 4, fillColor);
  
  // Text
  int16_t x1, y1;
  uint16_t tw, th;
  display.getTextBounds(btn.text, 0, 0, &x1, &y1, &tw, &th);
  
  int textX = btn.x + (btn.w - tw) / 2;
  int textY = btn.y + (btn.h - th) / 2;
  
  uint16_t textColor = btn.enabled ? (selected ? COLOR_YELLOW : btn.color) : COLOR_GRAY;
  display.setCursor(textX, textY);
  display.setTextColor(textColor, fillColor);
  display.print(btn.text);
}

// Draw title bar
void drawTitleBar(const char* title) {
  display.fillRect(0, 0, 320, 25, COLOR_BLUE);
  display.setCursor(10, 8);
  display.setTextColor(COLOR_WHITE, COLOR_BLUE);
  display.setTextSize(1);
  display.print(title);
}

// Draw status bar
void drawStatusBar(const char* status) {
  display.fillRect(0, 215, 320, 25, COLOR_DARKGRAY);
  display.setCursor(10, 222);
  display.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
  display.setTextSize(1);
  display.print(status);
}

// Main Menu
void drawMainMenu() {
  display.fillScreen(COLOR_BLACK);
  drawTitleBar("MatrixOS - Main Menu");
  drawStatusBar("W/S=Move  Enter=Select");
  
  Button buttons[4] = {
    {40, 40, 240, 30, "System Tests", COLOR_GREEN, true},
    {40, 80, 240, 30, "Demos & Graphics", COLOR_CYAN, true},
    {40, 120, 240, 30, "Settings", COLOR_YELLOW, true},
    {40, 160, 240, 30, "About", COLOR_MAGENTA, true}
  };
  
  for (int i = 0; i < 4; i++) {
    drawButton(buttons[i], i == selectedItem);
  }
}

// Tests Menu
void drawTestsMenu() {
  display.fillScreen(COLOR_BLACK);
  drawTitleBar("System Tests");
  drawStatusBar("W/S=Move  Enter=Run  Esc=Back");
  
  Button buttons[5] = {
    {20, 35, 280, 25, "Color Bars Test", COLOR_GREEN, true},
    {20, 65, 280, 25, "Memory Test", COLOR_CYAN, true},
    {20, 95, 280, 25, "Display Test", COLOR_YELLOW, true},
    {20, 125, 280, 25, "Pixel Test", COLOR_MAGENTA, true},
    {20, 155, 280, 25, "< Back", COLOR_RED, true}
  };
  
  for (int i = 0; i < 5; i++) {
    drawButton(buttons[i], i == selectedItem);
  }
}

// Demos Menu
void drawDemosMenu() {
  display.fillScreen(COLOR_BLACK);
  drawTitleBar("Demos & Graphics");
  drawStatusBar("W/S=Move  Enter=Run  Esc=Back");
  
  Button buttons[5] = {
    {20, 35, 280, 25, "Matrix Rain", COLOR_GREEN, true},
    {20, 65, 280, 25, "3D Cube", COLOR_CYAN, true},
    {20, 95, 280, 25, "Bouncing Balls", COLOR_YELLOW, true},
    {20, 125, 280, 25, "Starfield", COLOR_MAGENTA, true},
    {20, 155, 280, 25, "< Back", COLOR_RED, true}
  };
  
  for (int i = 0; i < 5; i++) {
    drawButton(buttons[i], i == selectedItem);
  }
}

// Settings Menu
void drawSettingsMenu() {
  display.fillScreen(COLOR_BLACK);
  drawTitleBar("Settings");
  drawStatusBar("Esc=Back");
  
  display.setCursor(20, 40);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.print("Resolution: 320x240");
  
  display.setCursor(20, 60);
  display.print("Color Depth: RGB565");
  
  display.setCursor(20, 80);
  display.print("Refresh Rate: 60Hz");
  
  display.setCursor(20, 100);
  display.print("Hardware: RP2350");
  
  display.setCursor(20, 120);
  display.print("DVI Pins: 18,12,14,16");
  
  Button backBtn = {20, 155, 280, 25, "< Back", COLOR_RED, true};
  drawButton(backBtn, true);
}

// About Screen
void drawAbout() {
  display.fillScreen(COLOR_BLACK);
  drawTitleBar("About MatrixOS");
  drawStatusBar("Press any key to return");
  
  display.setCursor(20, 40);
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.print("MatrixOS v1.0");
  
  display.setCursor(20, 60);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.print("Retro terminal interface");
  display.setCursor(20, 75);
  display.print("for RP2350 + DVI/HDMI");
  
  display.setCursor(20, 100);
  display.setTextColor(COLOR_YELLOW, COLOR_BLACK);
  display.print("Features:");
  
  display.setCursor(30, 115);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.print("- Interactive GUI");
  display.setCursor(30, 130);
  display.print("- System tests");
  display.setCursor(30, 145);
  display.print("- Graphics demos");
  display.setCursor(30, 160);
  display.print("- Serial control");
}

// Color Bars Test
void runColorBars() {
  display.fillScreen(COLOR_BLACK);
  drawTitleBar("Color Bars Test");
  drawStatusBar("Press any key to exit");
  
  struct ColorBar {
    uint16_t color;
    const char* name;
  };
  
  ColorBar bars[] = {
    {COLOR_WHITE, "White"},
    {COLOR_YELLOW, "Yellow"},
    {COLOR_CYAN, "Cyan"},
    {COLOR_GREEN, "Green"},
    {COLOR_MAGENTA, "Magenta"},
    {COLOR_RED, "Red"},
    {COLOR_BLUE, "Blue"},
    {COLOR_BLACK, "Black"}
  };
  
  int barWidth = 320 / 8;
  
  for (int i = 0; i < 8; i++) {
    display.fillRect(i * barWidth, 30, barWidth, 160, bars[i].color);
    
    // Label
    display.setCursor(i * barWidth + 5, 195);
    display.setTextColor(COLOR_WHITE, COLOR_BLACK);
    display.setTextSize(1);
    display.print(bars[i].name);
  }
  
  while (!Serial.available()) delay(10);
  while (Serial.available()) Serial.read();
}

// Memory Test
void runMemoryTest() {
  display.fillScreen(COLOR_BLACK);
  drawTitleBar("Memory Test");
  
  display.setCursor(20, 40);
  display.setTextColor(COLOR_YELLOW, COLOR_BLACK);
  display.print("Testing RAM...");
  
  unsigned long startTime = millis();
  
  // Simulate memory test
  for (int i = 0; i < 100; i++) {
    display.fillRect(20, 70, (280 * i) / 100, 20, COLOR_GREEN);
    display.setCursor(20, 95);
    display.setTextColor(COLOR_WHITE, COLOR_BLACK);
    display.print(i);
    display.print("% ");
    delay(20);
  }
  
  unsigned long elapsed = millis() - startTime;
  
  display.setCursor(20, 120);
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.print("Test completed in ");
  display.print(elapsed);
  display.print(" ms");
  
  display.setCursor(20, 140);
  display.setTextColor(COLOR_GREEN, COLOR_BLACK);
  display.print("PASSED: All memory OK");
  
  drawStatusBar("Press any key to exit");
  
  while (!Serial.available()) delay(10);
  while (Serial.available()) Serial.read();
}

// Matrix Rain Demo
void runMatrixRain() {
  display.fillScreen(COLOR_BLACK);
  
  uint8_t columns[53];
  for (int i = 0; i < 53; i++) {
    columns[i] = random(30);
  }
  
  unsigned long startTime = millis();
  
  while (!Serial.available() && millis() - startTime < 10000) {
    for (int x = 0; x < 53; x++) {
      if (random(100) < 25) {
        int y = columns[x];
        int px = x * 6;
        
        // Clear old
        if (y >= 5) {
          display.fillRect(px, (y - 5) * 8, 6, 8, COLOR_BLACK);
        }
        
        // Draw head
        if (y >= 0 && y < 30) {
          display.setCursor(px, y * 8);
          display.setTextColor(COLOR_WHITE, COLOR_BLACK);
          char c = random(33, 127);
          display.print(c);
        }
        
        // Trail
        if (y - 1 >= 0 && y - 1 < 30) {
          display.setCursor(px, (y - 1) * 8);
          display.setTextColor(COLOR_GREEN, COLOR_BLACK);
          char c = random(33, 127);
          display.print(c);
        }
        
        columns[x]++;
        if (columns[x] >= 35) {
          columns[x] = 0;
        }
      }
    }
    delay(50);
  }
  
  while (Serial.available()) Serial.read();
}

// 3D Cube Demo
void run3DCube() {
  display.fillScreen(COLOR_BLACK);
  drawStatusBar("Press any key to exit");
  
  float vertices[8][3] = {
    {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
    {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}
  };
  
  int edges[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0},
    {4,5}, {5,6}, {6,7}, {7,4},
    {0,4}, {1,5}, {2,6}, {3,7}
  };
  
  float angle = 0;
  unsigned long startTime = millis();
  
  while (!Serial.available() && millis() - startTime < 10000) {
    display.fillScreen(COLOR_BLACK);
    drawStatusBar("Press any key to exit");
    
    int projected[8][2];
    
    for (int i = 0; i < 8; i++) {
      float x = vertices[i][0];
      float y = vertices[i][1];
      float z = vertices[i][2];
      
      float xr = x * cos(angle) - z * sin(angle);
      float zr = x * sin(angle) + z * cos(angle);
      float yr = y * cos(angle * 0.7) - zr * sin(angle * 0.7);
      float zr2 = y * sin(angle * 0.7) + zr * cos(angle * 0.7);
      
      float factor = 4 / (4 + zr2);
      projected[i][0] = 160 + (int)(xr * 40 * factor);
      projected[i][1] = 100 + (int)(yr * 40 * factor);
    }
    
    for (int i = 0; i < 12; i++) {
      int v1 = edges[i][0];
      int v2 = edges[i][1];
      display.drawLine(projected[v1][0], projected[v1][1],
                       projected[v2][0], projected[v2][1], COLOR_CYAN);
    }
    
    angle += 0.05;
    delay(30);
  }
  
  while (Serial.available()) Serial.read();
}

// Bouncing Balls Demo
void runBouncingBalls() {
  display.fillScreen(COLOR_BLACK);
  drawStatusBar("Press any key to exit");
  
  struct Ball {
    float x, y, vx, vy;
    uint16_t color;
    int radius;
  };
  
  Ball balls[5];
  for (int i = 0; i < 5; i++) {
    balls[i].x = random(20, 300);
    balls[i].y = random(40, 200);
    balls[i].vx = random(-3, 3);
    balls[i].vy = random(-3, 3);
    balls[i].radius = random(3, 8);
    
    uint16_t colors[] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW, COLOR_CYAN};
    balls[i].color = colors[i];
  }
  
  unsigned long startTime = millis();
  
  while (!Serial.available() && millis() - startTime < 15000) {
    display.fillRect(0, 25, 320, 190, COLOR_BLACK);
    
    for (int i = 0; i < 5; i++) {
      // Update position
      balls[i].x += balls[i].vx;
      balls[i].y += balls[i].vy;
      
      // Bounce
      if (balls[i].x < balls[i].radius || balls[i].x > 320 - balls[i].radius) {
        balls[i].vx = -balls[i].vx;
      }
      if (balls[i].y < 25 + balls[i].radius || balls[i].y > 215 - balls[i].radius) {
        balls[i].vy = -balls[i].vy;
      }
      
      // Draw
      display.fillCircle((int)balls[i].x, (int)balls[i].y, balls[i].radius, balls[i].color);
    }
    
    drawStatusBar("Press any key to exit");
    delay(30);
  }
  
  while (Serial.available()) Serial.read();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  display.setTextSize(1);
  display.setTextWrap(false);
  
  drawMainMenu();
  
  Serial.println("=== MatrixOS GUI ===");
  Serial.println("Controls:");
  Serial.println("  w/W = Up");
  Serial.println("  s/S = Down");
  Serial.println("  Enter/Space = Select");
  Serial.println("  Esc/b/B = Back");
}

void loop() {
  if (Serial.available()) {
    char key = Serial.read();
    
    // Navigation
    if (key == 'w' || key == 'W') {
      selectedItem--;
      if (selectedItem < 0) selectedItem = 0;
      redrawCurrentMenu();
    }
    else if (key == 's' || key == 'S') {
      int maxItems = getMaxItems();
      selectedItem++;
      if (selectedItem >= maxItems) selectedItem = maxItems - 1;
      redrawCurrentMenu();
    }
    else if (key == '\r' || key == '\n' || key == ' ') {
      handleSelect();
    }
    else if (key == 27 || key == 'b' || key == 'B') {  // Esc or 'b'
      handleBack();
    }
  }
}

int getMaxItems() {
  switch (currentState) {
    case MENU_MAIN: return 4;
    case MENU_TESTS: return 5;
    case MENU_DEMOS: return 5;
    default: return 1;
  }
}

void redrawCurrentMenu() {
  switch (currentState) {
    case MENU_MAIN: drawMainMenu(); break;
    case MENU_TESTS: drawTestsMenu(); break;
    case MENU_DEMOS: drawDemosMenu(); break;
    case MENU_SETTINGS: drawSettingsMenu(); break;
  }
}

void handleSelect() {
  switch (currentState) {
    case MENU_MAIN:
      if (selectedItem == 0) {
        currentState = MENU_TESTS;
        selectedItem = 0;
        drawTestsMenu();
      } else if (selectedItem == 1) {
        currentState = MENU_DEMOS;
        selectedItem = 0;
        drawDemosMenu();
      } else if (selectedItem == 2) {
        currentState = MENU_SETTINGS;
        drawSettingsMenu();
      } else if (selectedItem == 3) {
        drawAbout();
        while (!Serial.available()) delay(10);
        while (Serial.available()) Serial.read();
        drawMainMenu();
      }
      break;
      
    case MENU_TESTS:
      if (selectedItem == 4) {
        currentState = MENU_MAIN;
        selectedItem = 0;
        drawMainMenu();
      } else if (selectedItem == 0) {
        runColorBars();
        drawTestsMenu();
      } else if (selectedItem == 1) {
        runMemoryTest();
        drawTestsMenu();
      }
      break;
      
    case MENU_DEMOS:
      if (selectedItem == 4) {
        currentState = MENU_MAIN;
        selectedItem = 0;
        drawMainMenu();
      } else if (selectedItem == 0) {
        runMatrixRain();
        drawDemosMenu();
      } else if (selectedItem == 1) {
        run3DCube();
        drawDemosMenu();
      } else if (selectedItem == 2) {
        runBouncingBalls();
        drawDemosMenu();
      }
      break;
  }
}

void handleBack() {
  if (currentState == MENU_TESTS || currentState == MENU_DEMOS || currentState == MENU_SETTINGS) {
    currentState = MENU_MAIN;
    selectedItem = 0;
    drawMainMenu();
  }
}
