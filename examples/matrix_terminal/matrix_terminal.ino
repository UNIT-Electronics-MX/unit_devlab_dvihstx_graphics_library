// Matrix Terminal - Interactive Shell with Matrix Style
// Arduino RP2350 + DVI/HDMI
// Pines: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+, D5(GPIO14)=Data1+, D3(GPIO16)=Data2+

#include <DevLab_DVIHSTX_Graphics.h>

DVHSTXPinout pinConfig = {15, 19, 17, 13};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Inverted colors for hardware
#define COLOR_BLACK       (uint16_t)0x0000
#define COLOR_MATRIX      (uint16_t)0x07E0  // Green
#define COLOR_MATRIX_DIM  (uint16_t)0x0400  // Dark green
#define COLOR_WHITE       (uint16_t)0xFFFF
#define COLOR_RED         (uint16_t)0xF800
#define COLOR_YELLOW      (uint16_t)0xFFE0
#define COLOR_CYAN        (uint16_t)0x07FF

// Terminal configuration
#define CHAR_WIDTH   6
#define CHAR_HEIGHT  8
#define COLS         53   // 320 / 6
#define ROWS         30   // 240 / 8
#define MAX_INPUT    60
#define MAX_HISTORY  10
#define MAX_FILES    20

// File system simulation
struct File {
  char name[20];
  char content[100];
  bool exists;
};

// Screen buffer
struct Line {
  char text[80];
  uint16_t color;
};

// Variables
Line screenBuffer[ROWS];
int bufferWritePos = 0;
char inputBuffer[MAX_INPUT];
int inputPos = 0;
File files[MAX_FILES];
char currentDir[30] = "/home/user";
char history[MAX_HISTORY][MAX_INPUT];
int historyCount = 0;
int historyPos = 0;
bool cursorBlink = true;
unsigned long lastBlink = 0;

// Matrix rain effect (optional)
uint8_t matrixColumns[COLS];
bool showMatrixRain = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  // Initialize file system
  initFileSystem();
  
  // Initialize screen buffer
  for (int i = 0; i < ROWS; i++) {
    screenBuffer[i].text[0] = '\0';
    screenBuffer[i].color = COLOR_MATRIX;
  }
  
  // Clear screen
  display.fillScreen(COLOR_BLACK);
  display.setTextSize(1);
  display.setTextWrap(false);
  
  // Show boot screen
  showBootScreen();
  delay(1500);
  
  // Clear and show prompt
  display.fillScreen(COLOR_BLACK);
  printLine("MatrixOS Terminal v1.0", COLOR_CYAN);
  printLine("Type 'help' for available commands", COLOR_MATRIX_DIM);
  printLine("", COLOR_MATRIX);
  showPrompt();
  
  // Initialize matrix rain
  for (int i = 0; i < COLS; i++) {
    matrixColumns[i] = random(ROWS);
  }
}

void loop() {
  // Handle serial input
  while (Serial.available()) {
    char c = Serial.read();
    handleInput(c);
  }
  
  // Cursor blink
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    cursorBlink = !cursorBlink;
    drawCursor();
  }
  
  // Matrix rain effect (if enabled)
  if (showMatrixRain) {
    updateMatrixRain();
    
    // Check for stop command
    if (Serial.available()) {
      showMatrixRain = false;
      while (Serial.available()) Serial.read();
    }
    
    delay(50);
  }
}

void initFileSystem() {
  // Create some default files
  strcpy(files[0].name, "readme.txt");
  strcpy(files[0].content, "Welcome to MatrixOS! A retro terminal experience.");
  files[0].exists = true;
  
  strcpy(files[1].name, "system.log");
  strcpy(files[1].content, "System initialized. All systems operational.");
  files[1].exists = true;
  
  strcpy(files[2].name, "code.c");
  strcpy(files[2].content, "#include <stdio.h>\nint main() { return 0; }");
  files[2].exists = true;
  
  strcpy(files[3].name, "data.bin");
  strcpy(files[3].content, "01001101 01100001 01110100 01110010");
  files[3].exists = true;
  
  for (int i = 4; i < MAX_FILES; i++) {
    files[i].exists = false;
  }
}

void handleInput(char c) {
  if (c == '\r' || c == '\n') {
    // Execute command
    inputBuffer[inputPos] = '\0';
    Serial.println(inputBuffer);  // Echo to serial
    
    // Clear cursor
    eraseCursor();
    
    // New line
    printLine("", COLOR_MATRIX);
    
    // Execute
    if (inputPos > 0) {
      executeCommand(inputBuffer);
      
      // Add to history
      if (historyCount < MAX_HISTORY) {
        strcpy(history[historyCount], inputBuffer);
        historyCount++;
      } else {
        // Shift history
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
          strcpy(history[i], history[i + 1]);
        }
        strcpy(history[MAX_HISTORY - 1], inputBuffer);
      }
      historyPos = historyCount;
    }
    
    // Reset input
    inputPos = 0;
    inputBuffer[0] = '\0';
    
    // Show new prompt
    showPrompt();
    
  } else if (c == 127 || c == 8) {  // Backspace
    if (inputPos > 0) {
      eraseCursor();
      inputPos--;
      inputBuffer[inputPos] = '\0';
      redrawInputLine();
    }
  } else if (c >= 32 && c < 127) {  // Printable characters
    if (inputPos < MAX_INPUT - 1) {
      eraseCursor();
      inputBuffer[inputPos] = c;
      inputPos++;
      inputBuffer[inputPos] = '\0';
      redrawInputLine();
    }
  }
}

void executeCommand(char* cmd) {
  // Parse command
  char cmdName[20];
  char arg1[40];
  char arg2[40];
  
  int parsed = sscanf(cmd, "%s %s %s", cmdName, arg1, arg2);
  
  // Execute commands
  if (strcmp(cmdName, "help") == 0) {
    cmdHelp();
  } else if (strcmp(cmdName, "ls") == 0) {
    cmdLs();
  } else if (strcmp(cmdName, "cat") == 0) {
    if (parsed >= 2) cmdCat(arg1);
    else printLine("Usage: cat <filename>", COLOR_RED);
  } else if (strcmp(cmdName, "echo") == 0) {
    if (parsed >= 2) {
      // Print everything after "echo "
      char* text = cmd + 5;  // Skip "echo "
      printLine(text, COLOR_MATRIX);
    }
  } else if (strcmp(cmdName, "clear") == 0) {
    display.fillScreen(COLOR_BLACK);
    for (int i = 0; i < ROWS; i++) {
      screenBuffer[i].text[0] = '\0';
      screenBuffer[i].color = COLOR_MATRIX;
    }
    bufferWritePos = 0;
  } else if (strcmp(cmdName, "pwd") == 0) {
    printLine(currentDir, COLOR_CYAN);
  } else if (strcmp(cmdName, "date") == 0) {
    cmdDate();
  } else if (strcmp(cmdName, "uptime") == 0) {
    cmdUptime();
  } else if (strcmp(cmdName, "matrix") == 0) {
    cmdMatrixRain();
  } else if (strcmp(cmdName, "neofetch") == 0) {
    cmdNeofetch();
  } else if (strcmp(cmdName, "whoami") == 0) {
    printLine("neo", COLOR_CYAN);
  } else if (strcmp(cmdName, "uname") == 0) {
    printLine("MatrixOS 1.0 RP2350 ARM", COLOR_CYAN);
  } else if (strcmp(cmdName, "free") == 0) {
    cmdFree();
  } else if (strcmp(cmdName, "ps") == 0) {
    cmdPs();
  } else if (strcmp(cmdName, "colourbar") == 0 || strcmp(cmdName, "colorbar") == 0) {
    cmdColourBar();
  } else if (strcmp(cmdName, "cube") == 0) {
    cmdTestCube();
  } else if (strcmp(cmdName, "exit") == 0) {
    printLine("There is no escape from the Matrix...", COLOR_RED);
  } else if (strlen(cmdName) > 0) {
    char msg[80];
    sprintf(msg, "Command not found: %s", cmdName);
    printLine(msg, COLOR_RED);
    printLine("Type 'help' for available commands", COLOR_MATRIX_DIM);
  }
}

void cmdHelp() {
  printLine("Available commands:", COLOR_YELLOW);
  printLine("  help      - Show this help", COLOR_MATRIX);
  printLine("  ls        - List files", COLOR_MATRIX);
  printLine("  cat <file>- Show file content", COLOR_MATRIX);
  printLine("  echo <txt>- Print text", COLOR_MATRIX);
  printLine("  clear     - Clear screen", COLOR_MATRIX);
  printLine("  pwd       - Print working directory", COLOR_MATRIX);
  printLine("  date      - Show date/time", COLOR_MATRIX);
  printLine("  uptime    - Show system uptime", COLOR_MATRIX);
  printLine("  whoami    - Show current user", COLOR_MATRIX);
  printLine("  uname     - Show system info", COLOR_MATRIX);
  printLine("  neofetch  - System information", COLOR_MATRIX);
  printLine("  free      - Show memory info", COLOR_MATRIX);
  printLine("  ps        - Show processes", COLOR_MATRIX);
  printLine("  matrix    - Toggle matrix rain", COLOR_MATRIX);
  printLine("  colourbar - Display color test bars", COLOR_MATRIX);
  printLine("  cube      - Display 3D rotating cube", COLOR_MATRIX);
}

void cmdLs() {
  char line[80];
  int fileCount = 0;
  
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].exists) {
      sprintf(line, "  %s", files[i].name);
      printLine(line, COLOR_CYAN);
      fileCount++;
    }
  }
  
  if (fileCount == 0) {
    printLine("  (empty directory)", COLOR_MATRIX_DIM);
  }
}

void cmdCat(char* filename) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].exists && strcmp(files[i].name, filename) == 0) {
      printLine(files[i].content, COLOR_MATRIX);
      return;
    }
  }
  
  char msg[80];
  sprintf(msg, "File not found: %s", filename);
  printLine(msg, COLOR_RED);
}

void cmdDate() {
  char line[80];
  unsigned long secs = millis() / 1000;
  unsigned long mins = secs / 60;
  unsigned long hours = mins / 60;
  sprintf(line, "Uptime: %02luh %02lum %02lus", hours, mins % 60, secs % 60);
  printLine(line, COLOR_CYAN);
}

void cmdUptime() {
  char line[80];
  sprintf(line, "System uptime: %lu ms", millis());
  printLine(line, COLOR_CYAN);
}

void cmdFree() {
  printLine("Memory:", COLOR_YELLOW);
  printLine("  Total:  512 KB", COLOR_MATRIX);
  printLine("  Used:   234 KB", COLOR_MATRIX);
  printLine("  Free:   278 KB", COLOR_MATRIX);
}

void cmdPs() {
  printLine("PID  COMMAND", COLOR_YELLOW);
  printLine("  1  init", COLOR_MATRIX);
  printLine("  2  systemd", COLOR_MATRIX);
  printLine("  3  matrix_shell", COLOR_CYAN);
  printLine("  4  display_manager", COLOR_MATRIX);
}

void cmdNeofetch() {
  printLine("   ___  ___      _       _     ", COLOR_MATRIX);
  printLine("  |  \\/  |     | |     (_)    ", COLOR_MATRIX);
  printLine("  | .  . | __ _| |_ _ __ ___  __", COLOR_MATRIX);
  printLine("  | |\\/| |/ _` | __| '__| \\ \\/ /", COLOR_MATRIX);
  printLine("  | |  | | (_| | |_| |  | |>  < ", COLOR_MATRIX);
  printLine("  \\_|  |_/\\__,_|\\__|_|  |_/_/\\_\\", COLOR_MATRIX);
  printLine("", COLOR_MATRIX);
  printLine("OS: MatrixOS 1.0", COLOR_CYAN);
  printLine("Kernel: RP2350-ARM", COLOR_CYAN);
  printLine("Shell: matrix-sh", COLOR_CYAN);
  printLine("CPU: Dual-Core ARM Cortex-M33", COLOR_CYAN);
  printLine("Memory: 512 KB", COLOR_CYAN);
  printLine("Display: 320x240 HDMI", COLOR_CYAN);
}

void showPrompt() {
  char prompt[80];
  sprintf(prompt, "neo@matrix:%s$ ", currentDir);
  
  addToBuffer(prompt, COLOR_MATRIX);
  
  inputPos = 0;
  inputBuffer[0] = '\0';
}

void printLine(const char* text, uint16_t color) {
  addToBuffer(text, color);
}

void addToBuffer(const char* text, uint16_t color) {
  // Add line to buffer
  strncpy(screenBuffer[bufferWritePos].text, text, 79);
  screenBuffer[bufferWritePos].text[79] = '\0';
  screenBuffer[bufferWritePos].color = color;
  
  bufferWritePos++;
  
  // Scroll if needed
  if (bufferWritePos >= ROWS) {
    scrollUp();
  }
  
  redrawScreen();
}

void scrollUp() {
  // Shift buffer up
  for (int i = 0; i < ROWS - 1; i++) {
    strcpy(screenBuffer[i].text, screenBuffer[i + 1].text);
    screenBuffer[i].color = screenBuffer[i + 1].color;
  }
  
  // Clear last line
  screenBuffer[ROWS - 1].text[0] = '\0';
  screenBuffer[ROWS - 1].color = COLOR_MATRIX;
  
  bufferWritePos = ROWS - 1;
}

void redrawScreen() {
  display.fillScreen(COLOR_BLACK);
  
  for (int i = 0; i < ROWS; i++) {
    if (strlen(screenBuffer[i].text) > 0) {
      display.setCursor(0, i * CHAR_HEIGHT);
      display.setTextColor(screenBuffer[i].color, COLOR_BLACK);
      display.print(screenBuffer[i].text);
    }
  }
}

void redrawInputLine() {
  char prompt[80];
  sprintf(prompt, "neo@matrix:%s$ %s", currentDir, inputBuffer);
  
  // Update last line in buffer
  strncpy(screenBuffer[bufferWritePos].text, prompt, 79);
  screenBuffer[bufferWritePos].text[79] = '\0';
  screenBuffer[bufferWritePos].color = COLOR_MATRIX;
  
  // Redraw only last line
  int y = bufferWritePos * CHAR_HEIGHT;
  display.fillRect(0, y, display.width(), CHAR_HEIGHT, COLOR_BLACK);
  display.setCursor(0, y);
  display.setTextColor(COLOR_MATRIX, COLOR_BLACK);
  display.print(prompt);
}

void drawCursor() {
  char prompt[80];
  sprintf(prompt, "neo@matrix:%s$ %s", currentDir, inputBuffer);
  
  int cursorX = strlen(prompt) * CHAR_WIDTH;
  int cursorY = bufferWritePos * CHAR_HEIGHT;
  
  uint16_t color = cursorBlink ? COLOR_MATRIX : COLOR_BLACK;
  display.fillRect(cursorX, cursorY + CHAR_HEIGHT - 2, CHAR_WIDTH, 2, color);
}

void eraseCursor() {
  char prompt[80];
  sprintf(prompt, "neo@matrix:%s$ %s", currentDir, inputBuffer);
  
  int cursorX = strlen(prompt) * CHAR_WIDTH;
  int cursorY = bufferWritePos * CHAR_HEIGHT;
  
  display.fillRect(cursorX, cursorY + CHAR_HEIGHT - 2, CHAR_WIDTH, 2, COLOR_BLACK);
}

void updateMatrixRain() {
  for (int x = 0; x < COLS; x++) {
    if (random(100) < 10) {  // 10% chance to update column
      int y = matrixColumns[x];
      
      // Draw character
      char c = random(33, 127);  // Random ASCII
      int px = x * CHAR_WIDTH;
      int py = y * CHAR_HEIGHT;
      
      if (y > 0) {
        // Dim previous character
        display.setCursor(px, (y - 1) * CHAR_HEIGHT);
        display.setTextColor(COLOR_MATRIX_DIM, COLOR_BLACK);
        display.print(c);
      }
      
      // Bright current character
      display.setCursor(px, py);
      display.setTextColor(COLOR_MATRIX, COLOR_BLACK);
      display.print(c);
      
      // Move down
      matrixColumns[x]++;
      if (matrixColumns[x] >= ROWS) {
        matrixColumns[x] = 0;
      }
    }
  }
}

void cmdColourBar() {
  display.fillScreen(COLOR_BLACK);
  
  // Define colors with names
  struct ColorBar {
    uint16_t color;
    const char* name;
  };
  
  ColorBar bars[] = {
    {(uint16_t)0xF800, "Red"},
    {(uint16_t)0x07E0, "Green"},
    {(uint16_t)0x001F, "Blue"},
    {(uint16_t)0xFFE0, "Yellow"},
    {(uint16_t)0xF81F, "Magenta"},
    {(uint16_t)0x07FF, "Cyan"},
    {(uint16_t)0xFFFF, "White"},
    {(uint16_t)0x0000, "Black"}
  };
  
  int barWidth = display.width() / 8;
  
  for (int i = 0; i < 8; i++) {
    display.fillRect(i * barWidth, 0, barWidth, display.height() - 40, bars[i].color);
  }
  
  // Draw labels at bottom
  display.fillRect(0, display.height() - 40, display.width(), 40, COLOR_BLACK);
  for (int i = 0; i < 8; i++) {
    display.setCursor(i * barWidth + 2, display.height() - 30);
    display.setTextColor(COLOR_MATRIX, COLOR_BLACK);
    display.print(bars[i].name);
  }
  
  display.setCursor(10, display.height() - 15);
  display.setTextColor(COLOR_YELLOW, COLOR_BLACK);
  display.print("Press any key to return...");
  
  // Wait for key
  while (!Serial.available()) {
    delay(10);
  }
  while (Serial.available()) Serial.read();
  
  // Restore terminal
  redrawScreen();
}

void cmdTestCube() {
  display.fillScreen(COLOR_BLACK);
  
  // 3D cube vertices
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
  int centerX = display.width() / 2;
  int centerY = display.height() / 2;
  int scale = 40;
  
  unsigned long startTime = millis();
  
  while (millis() - startTime < 5000) {  // Run for 5 seconds
    if (Serial.available()) break;
    
    display.fillScreen(COLOR_BLACK);
    
    // Rotate and project vertices
    int projected[8][2];
    
    for (int i = 0; i < 8; i++) {
      float x = vertices[i][0];
      float y = vertices[i][1];
      float z = vertices[i][2];
      
      // Rotate around Y axis
      float xr = x * cos(angle) - z * sin(angle);
      float zr = x * sin(angle) + z * cos(angle);
      
      // Rotate around X axis
      float yr = y * cos(angle * 0.7) - zr * sin(angle * 0.7);
      float zr2 = y * sin(angle * 0.7) + zr * cos(angle * 0.7);
      
      // Simple perspective projection
      float distance = 4;
      float factor = distance / (distance + zr2);
      
      projected[i][0] = centerX + (int)(xr * scale * factor);
      projected[i][1] = centerY + (int)(yr * scale * factor);
    }
    
    // Draw edges
    for (int i = 0; i < 12; i++) {
      int v1 = edges[i][0];
      int v2 = edges[i][1];
      
      display.drawLine(
        projected[v1][0], projected[v1][1],
        projected[v2][0], projected[v2][1],
        COLOR_CYAN
      );
    }
    
    // Draw title
    display.setCursor(10, 10);
    display.setTextColor(COLOR_MATRIX, COLOR_BLACK);
    display.print("3D Cube Test");
    
    angle += 0.05;
    delay(30);
  }
  
  // Clear serial buffer
  while (Serial.available()) Serial.read();
  
  // Restore terminal
  redrawScreen();
}

void cmdMatrixRain() {
  display.fillScreen(COLOR_BLACK);
  
  // Reset matrix columns with staggered starts
  for (int i = 0; i < COLS; i++) {
    matrixColumns[i] = random(ROWS);
  }
  
  // Show instruction
  display.setCursor(10, 10);
  display.setTextColor(COLOR_MATRIX, COLOR_BLACK);
  display.print("MATRIX RAIN - Press any key to exit");
  delay(1500);
  
  display.fillScreen(COLOR_BLACK);
  
  // Color cycle for RGB background effect
  uint16_t bgColors[3] = {
    (uint16_t)0xF800,  // Red
    (uint16_t)0x07E0,  // Green
    (uint16_t)0x001F   // Blue
  };
  
  unsigned long lastUpdate = 0;
  
  // Run matrix rain loop until key press
  while (!Serial.available()) {
    // Limit update rate
    if (millis() - lastUpdate < 80) {
      delay(10);
      continue;
    }
    lastUpdate = millis();
    
    // Update only some columns per frame for efficiency
    for (int x = 0; x < COLS; x++) {
      if (random(100) < 25) {  // 25% chance to update column
        int y = matrixColumns[x];
        int px = x * CHAR_WIDTH;
        
        // Use color based on column for RGB effect
        int colIndex = (x + (millis() / 2000)) % 3;
        uint16_t mainColor = bgColors[colIndex];
        
        // Clear old position (5 chars back)
        if (y >= 5) {
          display.fillRect(px, (y - 5) * CHAR_HEIGHT, CHAR_WIDTH, CHAR_HEIGHT, COLOR_BLACK);
        }
        
        // Draw new head (bright white)
        if (y >= 0 && y < ROWS) {
          display.setCursor(px, y * CHAR_HEIGHT);
          display.setTextColor(COLOR_WHITE, COLOR_BLACK);
          char c = random(33, 127);
          display.print(c);
        }
        
        // Draw trail
        if (y - 1 >= 0 && y - 1 < ROWS) {
          display.setCursor(px, (y - 1) * CHAR_HEIGHT);
          display.setTextColor(mainColor, COLOR_BLACK);
          char c = random(33, 127);
          display.print(c);
        }
        
        if (y - 2 >= 0 && y - 2 < ROWS) {
          display.setCursor(px, (y - 2) * CHAR_HEIGHT);
          display.setTextColor((uint16_t)(mainColor | 0x4208), COLOR_BLACK);
          char c = random(33, 127);
          display.print(c);
        }
        
        // Move down
        matrixColumns[x]++;
        if (matrixColumns[x] >= ROWS + 5) {
          matrixColumns[x] = 0;
        }
      }
    }
  }
  
  // Clear serial buffer
  while (Serial.available()) Serial.read();
  
  // Restore terminal
  redrawScreen();
}

void showBootScreen() {
  display.fillScreen(COLOR_BLACK);
  
  printLine("", COLOR_MATRIX);
  printLine(" Initializing MatrixOS...", COLOR_MATRIX);
  printLine("", COLOR_MATRIX);
  printLine(" [OK] Loading kernel modules", COLOR_MATRIX);
  delay(200);
  printLine(" [OK] Mounting filesystems", COLOR_MATRIX);
  delay(200);
  printLine(" [OK] Starting network", COLOR_MATRIX);
  delay(200);
  printLine(" [OK] Initializing display", COLOR_MATRIX);
  delay(200);
  printLine(" [OK] Starting shell", COLOR_MATRIX);
  delay(300);
  printLine("", COLOR_MATRIX);
  printLine(" Welcome to the Matrix.", COLOR_CYAN);
}
