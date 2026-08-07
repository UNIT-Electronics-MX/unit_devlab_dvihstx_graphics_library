// PSRAM + DVI Demo - APS6404L 8MB with HDMI Output
// Arduino RP2350 with external QSPI PSRAM and DVI/HDMI
// 
// Hardware:
// DVI:  D1(GPIO18)=Clock+, D7(GPIO12)=Data0+, D5(GPIO14)=Data1+, D3(GPIO16)=Data2+
// PSRAM: GPIO0=CS, GPIO47-50=QSPI Data, GPIO52=QSPI CLK

#include <UDVI_HSTX.h>
#include "hardware/gpio.h"
#include "hardware/spi.h"

DVHSTXPinout pinConfig = {14, 18, 16, 12};
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// Inverted colors for hardware
#define COLOR_BLACK       (uint16_t)0x0000
#define COLOR_MATRIX      (uint16_t)0x07E0  // Green
#define COLOR_RED         (uint16_t)0xF800
#define COLOR_BLUE        (uint16_t)0x001F
#define COLOR_YELLOW      (uint16_t)0xFFE0
#define COLOR_CYAN        (uint16_t)0x07FF
#define COLOR_MAGENTA     (uint16_t)0xF81F
#define COLOR_WHITE       (uint16_t)0xFFFF

// PSRAM Configuration
#define PSRAM_CS_PIN    0
#define QSPI_D0_PIN     47
#define QSPI_D1_PIN     48
#define QSPI_D2_PIN     49
#define QSPI_D3_PIN     50
#define QSPI_CLK_PIN    52
#define PSRAM_SIZE      (8 * 1024 * 1024)  // 8MB

// PSRAM Commands (APS6404L)
#define PSRAM_CMD_READ          0x03
#define PSRAM_CMD_WRITE         0x02
#define PSRAM_CMD_RESET_ENABLE  0x66
#define PSRAM_CMD_RESET         0x99
#define PSRAM_CMD_READ_ID       0x9F

// PSRAM Class
class PSRAMChip {
private:
  uint8_t csPin;
  bool initialized;
  
  void select() {
    digitalWrite(csPin, LOW);
    delayMicroseconds(1);
  }
  
  void deselect() {
    delayMicroseconds(1);
    digitalWrite(csPin, HIGH);
  }
  
  uint8_t transfer(uint8_t data) {
    // Simple bit-bang SPI
    uint8_t result = 0;
    for (int i = 7; i >= 0; i--) {
      result = (result << 1) | digitalRead(QSPI_D0_PIN);
    }
    return result;
  }
  
public:
  PSRAMChip(uint8_t cs) : csPin(cs), initialized(false) {}
  
  bool begin() {
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH);
    delay(10);
    
    // Reset PSRAM
    select();
    transfer(PSRAM_CMD_RESET_ENABLE);
    deselect();
    delay(1);
    
    select();
    transfer(PSRAM_CMD_RESET);
    deselect();
    delay(10);
    
    // Read ID
    uint32_t id = readID();
    
    if (id == 0x0D5D52 || id == 0x5D0D52) {
      initialized = true;
      return true;
    }
    
    initialized = false;
    return false;
  }
  
  uint32_t readID() {
    select();
    transfer(PSRAM_CMD_READ_ID);
    transfer(0x00);
    transfer(0x00);
    transfer(0x00);
    
    uint32_t id = 0;
    id |= ((uint32_t)transfer(0xFF) << 16);
    id |= ((uint32_t)transfer(0xFF) << 8);
    id |= transfer(0xFF);
    
    deselect();
    return id;
  }
  
  void writeByte(uint32_t address, uint8_t data) {
    if (!initialized || address >= PSRAM_SIZE) return;
    
    select();
    transfer(PSRAM_CMD_WRITE);
    transfer((address >> 16) & 0xFF);
    transfer((address >> 8) & 0xFF);
    transfer(address & 0xFF);
    transfer(data);
    deselect();
  }
  
  uint8_t readByte(uint32_t address) {
    if (!initialized || address >= PSRAM_SIZE) return 0;
    
    select();
    transfer(PSRAM_CMD_READ);
    transfer((address >> 16) & 0xFF);
    transfer((address >> 8) & 0xFF);
    transfer(address & 0xFF);
    uint8_t data = transfer(0xFF);
    deselect();
    
    return data;
  }
  
  void writeBlock(uint32_t address, uint8_t* data, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
      writeByte(address + i, data[i]);
    }
  }
  
  void readBlock(uint32_t address, uint8_t* buffer, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
      buffer[i] = readByte(address + i);
    }
  }
  
  void write16(uint32_t address, uint16_t data) {
    writeByte(address, data >> 8);
    writeByte(address + 1, data & 0xFF);
  }
  
  uint16_t read16(uint32_t address) {
    uint16_t data = readByte(address) << 8;
    data |= readByte(address + 1);
    return data;
  }
  
  bool isInitialized() { return initialized; }
};

PSRAMChip psram(PSRAM_CS_PIN);

// Store sprite/image data in PSRAM
void storeSpriteInPSRAM(uint32_t address, const uint16_t* sprite, int width, int height) {
  for (int i = 0; i < width * height; i++) {
    psram.write16(address + (i * 2), sprite[i]);
  }
}

// Load and draw sprite from PSRAM
void drawSpriteFromPSRAM(uint32_t address, int x, int y, int width, int height) {
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      uint16_t color = psram.read16(address + ((row * width + col) * 2));
      display.drawPixel(x + col, y + row, color);
    }
  }
}

// Create test pattern in PSRAM
void createTestPattern() {
  display.fillScreen(COLOR_BLACK);
  display.setCursor(10, 10);
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.print("Creating pattern in PSRAM...");
  
  // Create a 32x32 gradient sprite
  uint32_t spriteAddr = 0x100000;  // Store at 1MB offset
  for (int y = 0; y < 32; y++) {
    for (int x = 0; x < 32; x++) {
      // Create gradient (inverted for hardware)
      uint8_t r = (x * 8) & 0x1F;
      uint8_t g = (y * 4) & 0x3F;
      uint8_t b = ((x + y) * 4) & 0x1F;
      uint16_t color = (uint16_t)((r << 11) | (g << 5) | b);
      
      psram.write16(spriteAddr + ((y * 32 + x) * 2), color);
    }
  }
  
  delay(1000);
}

// Demo: Draw multiple sprites from PSRAM
void demoSprites() {
  display.fillScreen(COLOR_BLACK);
  
  uint32_t spriteAddr = 0x100000;
  
  // Draw sprites at different positions
  for (int i = 0; i < 6; i++) {
    int x = (i % 3) * 80 + 20;
    int y = (i / 3) * 80 + 40;
    drawSpriteFromPSRAM(spriteAddr, x, y, 32, 32);
  }
  
  // Info text
  display.setCursor(10, 10);
  display.setTextColor(COLOR_YELLOW, COLOR_BLACK);
  display.print("Sprites loaded from PSRAM");
}

// Memory test visualization
void memoryTestVisual() {
  display.fillScreen(COLOR_BLACK);
  display.setCursor(10, 10);
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.print("PSRAM Memory Test");
  
  int y = 30;
  
  // Test 1: Write/Read bytes
  display.setCursor(10, y);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.print("Testing bytes...");
  
  psram.writeByte(0x0000, 0xAA);
  psram.writeByte(0x0001, 0x55);
  
  if (psram.readByte(0x0000) == 0xAA && psram.readByte(0x0001) == 0x55) {
    display.setTextColor(COLOR_MATRIX, COLOR_BLACK);
    display.print(" OK");
  } else {
    display.setTextColor(COLOR_RED, COLOR_BLACK);
    display.print(" FAIL");
  }
  y += 20;
  
  // Test 2: Pattern test
  display.setCursor(10, y);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.print("Testing pattern...");
  
  uint8_t pattern[] = {0x00, 0xFF, 0xAA, 0x55, 0x0F, 0xF0};
  psram.writeBlock(0x1000, pattern, 6);
  
  uint8_t readback[6];
  psram.readBlock(0x1000, readback, 6);
  
  bool ok = true;
  for (int i = 0; i < 6; i++) {
    if (readback[i] != pattern[i]) {
      ok = false;
      break;
    }
  }
  
  display.setTextColor(ok ? COLOR_MATRIX : COLOR_RED, COLOR_BLACK);
  display.print(ok ? " OK" : " FAIL");
  y += 20;
  
  // Test 3: Large block
  display.setCursor(10, y);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.print("Testing 1KB block...");
  
  for (int i = 0; i < 1024; i++) {
    psram.writeByte(0x2000 + i, i & 0xFF);
  }
  
  ok = true;
  for (int i = 0; i < 1024; i++) {
    if (psram.readByte(0x2000 + i) != (i & 0xFF)) {
      ok = false;
      break;
    }
  }
  
  display.setTextColor(ok ? COLOR_MATRIX : COLOR_RED, COLOR_BLACK);
  display.print(ok ? " OK" : " FAIL");
  y += 30;
  
  // Show memory info
  display.setCursor(10, y);
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.print("PSRAM: 8 MB Available");
  y += 15;
  
  display.setCursor(10, y);
  display.setTextColor(COLOR_YELLOW, COLOR_BLACK);
  display.print("Address: 0x000000-0x7FFFFF");
}

// Benchmark visualization
void benchmarkVisual() {
  display.fillScreen(COLOR_BLACK);
  display.setCursor(10, 10);
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.print("PSRAM Benchmark");
  
  const uint32_t testSize = 4096;  // 4KB
  uint8_t testData[256];
  
  // Fill test data
  for (int i = 0; i < 256; i++) {
    testData[i] = i;
  }
  
  // Write benchmark
  display.setCursor(10, 40);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.print("Write test...");
  
  unsigned long startTime = micros();
  for (uint32_t addr = 0; addr < testSize; addr += 256) {
    psram.writeBlock(addr, testData, 256);
  }
  unsigned long writeTime = micros() - startTime;
  
  float writeSpeed = (testSize / 1024.0) / (writeTime / 1000000.0);
  
  display.setCursor(10, 60);
  display.setTextColor(COLOR_MATRIX, COLOR_BLACK);
  display.print("Write: ");
  display.print(writeSpeed, 1);
  display.print(" KB/s");
  
  // Read benchmark
  display.setCursor(10, 90);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.print("Read test...");
  
  startTime = micros();
  for (uint32_t addr = 0; addr < testSize; addr += 256) {
    psram.readBlock(addr, testData, 256);
  }
  unsigned long readTime = micros() - startTime;
  
  float readSpeed = (testSize / 1024.0) / (readTime / 1000000.0);
  
  display.setCursor(10, 110);
  display.setTextColor(COLOR_MATRIX, COLOR_BLACK);
  display.print("Read:  ");
  display.print(readSpeed, 1);
  display.print(" KB/s");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize display
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  
  display.fillScreen(COLOR_BLACK);
  display.setTextSize(1);
  display.setTextWrap(true);
  
  // Show boot message
  display.setCursor(10, 10);
  display.setTextColor(COLOR_CYAN, COLOR_BLACK);
  display.print("PSRAM + DVI Demo");
  
  display.setCursor(10, 30);
  display.setTextColor(COLOR_WHITE, COLOR_BLACK);
  display.print("Initializing PSRAM...");
  
  Serial.println("=== PSRAM + DVI Demo ===");
  Serial.println("Initializing PSRAM APS6404L...");
  
  // Initialize PSRAM
  if (psram.begin()) {
    uint32_t id = psram.readID();
    
    display.setCursor(10, 50);
    display.setTextColor(COLOR_MATRIX, COLOR_BLACK);
    display.print("PSRAM OK! ID: 0x");
    display.print(id, HEX);
    
    Serial.print("PSRAM initialized! ID: 0x");
    Serial.println(id, HEX);
    
    delay(2000);
    
    // Run demos
    memoryTestVisual();
    delay(3000);
    
    benchmarkVisual();
    delay(3000);
    
    createTestPattern();
    demoSprites();
    
    Serial.println("\nDemos complete. Commands:");
    Serial.println("  t = Memory test");
    Serial.println("  b = Benchmark");
    Serial.println("  s = Sprite demo");
    Serial.println("  c = Clear screen");
    
  } else {
    display.setCursor(10, 50);
    display.setTextColor(COLOR_RED, COLOR_BLACK);
    display.print("PSRAM FAILED!");
    
    Serial.println("ERROR: PSRAM initialization failed!");
    
    while (true) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(250);
    }
  }
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    
    switch (cmd) {
      case 't':
      case 'T':
        Serial.println("Running memory test...");
        memoryTestVisual();
        break;
        
      case 'b':
      case 'B':
        Serial.println("Running benchmark...");
        benchmarkVisual();
        break;
        
      case 's':
      case 'S':
        Serial.println("Running sprite demo...");
        createTestPattern();
        demoSprites();
        break;
        
      case 'c':
      case 'C':
        display.fillScreen(COLOR_BLACK);
        Serial.println("Screen cleared");
        break;
        
      default:
        Serial.println("Commands: t=test, b=bench, s=sprites, c=clear");
        break;
    }
  }
  
  // Blink LED
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    lastBlink = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}
