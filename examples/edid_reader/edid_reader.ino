#include <Wire.h>
#include <DevLab_DVIHSTX_Graphics.h>

#define EDID_ADDR 0x50

uint8_t edid[128];

// HDMI pinout - Configuración de pines para RP2350 ck do d1 d2
DVHSTXPinout pinConfig = {14, 18, 16, 12};

DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// ============================
// Lectura por bloques de 16B
// ============================
bool readBlock(uint8_t start) {
  Wire.beginTransmission(EDID_ADDR);
  Wire.write(start);
  if (Wire.endTransmission(false) != 0) return false;

  uint8_t received = Wire.requestFrom(EDID_ADDR, 16);
  if (received != 16) return false;

  for (int i = 0; i < 16; i++) {
    edid[start + i] = Wire.read();
  }

  return true;
}

void readEDID() {
  for (int i = 0; i < 128; i += 16) {
    if (!readBlock(i)) {
      Serial.println("Error leyendo EDID.");
      return;
    }
  }
}

// ============================
// Imprimir trama completa
// ============================
void printRaw() {
  Serial.println("\n===== TRAMA COMPLETA EDID (128 bytes) =====");

  for (int i = 0; i < 128; i++) {
    Serial.print("0x");
    if (edid[i] < 16) Serial.print("0");
    Serial.print(edid[i], HEX);
    Serial.print(" ");

    if ((i+1) % 16 == 0) {
      Serial.print(" | ");   // separador ASCII
      for (int j = i-15; j <= i; j++) {
        char c = (edid[j] >= 32 && edid[j] <= 126) ? edid[j] : '.';
        Serial.print(c);
      }
      Serial.println();
    }
  }

  Serial.println("=============================================");
}

// ============================
// Decodificar fabricante
// ============================
String decodeManufacturer(uint16_t code) {
  char c1 = ((code >> 10) & 0x1F) + 'A' - 1;
  char c2 = ((code >> 5) & 0x1F) + 'A' - 1;
  char c3 = (code & 0x1F) + 'A' - 1;
  return String(c1) + c2 + c3;
}

// ============================
// Extraer HACT y VACT
// ============================
void decodeResolution(uint16_t &hact, uint16_t &vact) {
  // Primer descriptor detallado
  uint8_t *dtd = &edid[54];

  uint16_t pixelClock = dtd[0] | (dtd[1] << 8);

  if (pixelClock == 0) {
    hact = vact = 0;
    return;
  }

  hact = dtd[2] | ((dtd[4] & 0xF0) << 4);
  vact = dtd[5] | ((dtd[7] & 0xF0) << 4);
}

// ============================
// Obtener nombre del monitor
// ============================
String getMonitorName() {
  for (int off = 54; off < 126; off += 18) {
    if (edid[off + 3] == 0xFC) {
      String n = "";
      for (int i = 5; i < 18; i++) {
        char c = edid[off + i];
        if (c == 0x0A) break;
        if (c >= 32 && c <= 126) n += c;
      }
      return n;
    }
  }
  return "UNKNOWN";
}

// ============================
// Leer otro dispositivo I2C
// ============================
void readI2CDevice(uint8_t addr) {
  Serial.print("\n------ LEYENDO I2C 0x");
  Serial.print(addr, HEX);
  Serial.println(" ------");

  uint8_t buf[32];

  Wire.beginTransmission(addr);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0) {
    Serial.println("   NO RESPONDE");
    return;
  }

  uint8_t r = Wire.requestFrom(addr, 32);
  if (r == 0) {
    Serial.println("   SIN DATOS");
    return;
  }

  Serial.println("   TRAMA RECIBIDA:");
  for (int i = 0; i < r; i++) {
    buf[i] = Wire.read();
    Serial.print("0x");
    if (buf[i] < 16) Serial.print("0");
    Serial.print(buf[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

// ============================
// Imprimir EDID
// ============================
void printEDID() {
  Serial.println("\n===== EDID DECODIFICADA =====");

  String manufacturer = decodeManufacturer((edid[8] << 8) | edid[9]);
  uint16_t product    = (edid[11] << 8) | edid[10];
  uint8_t week        = edid[16];
  uint16_t year       = edid[17] + 1990;

  uint16_t h, v;
  decodeResolution(h, v);

  String name = getMonitorName();

  Serial.println("Fabricante: " + manufacturer);
  Serial.print("Producto ID: 0x"); Serial.println(product, HEX);
  Serial.print("Fabricado en Semana "); Serial.println(week);
  Serial.print("Año: "); Serial.println(year);
  Serial.print("Resolución nativa: "); Serial.print(h); Serial.print(" x "); Serial.println(v);
  Serial.println("Monitor Name: " + name);

  Serial.println("====================================");
}

// ============================
// Barras de Colores HDMI
// ============================
void drawColorBars() {
  int w = display.width();
  int h = display.height();
  int barWidth = w / 8;
  
  // Colores RGB565
  uint16_t colors[8] = {
    0xFFFF,  // Blanco
    0xFFE0,  // Amarillo
    0x07FF,  // Cyan
    0x07E0,  // Verde
    0xF81F,  // Magenta
    0xF800,  // Rojo
    0x001F,  // Azul
    0x0000   // Negro
  };

  const char* names[8] = {
    "WHT", "YEL", "CYN", "GRN", "MAG", "RED", "BLU", "BLK"
  };

  // Dibujar barras verticales
  for (int i = 0; i < 8; i++) {
    display.fillRect(i * barWidth, 0, barWidth, h - 30, colors[i]);
    
    // Etiqueta del color
    display.setTextSize(2);
    uint16_t textColor = (i == 7) ? 0xFFFF : 0x0000;
    display.setTextColor(textColor);
    
    int textX = i * barWidth + (barWidth / 2) - 18;
    int textY = (h / 2) - 8;
    display.setCursor(textX, textY);
    display.print(names[i]);
  }
  
  // Información EDID en la parte inferior
  display.fillRect(0, h - 30, w, 30, 0x0000);
  display.setTextSize(1);
  display.setTextColor(0xFFFF);
  display.setCursor(5, h - 25);
  
  String manufacturer = decodeManufacturer((edid[8] << 8) | edid[9]);
  uint16_t hres, vres;
  decodeResolution(hres, vres);
  
  display.print("EDID: ");
  display.print(manufacturer);
  display.print(" | ");
  display.print(hres);
  display.print("x");
  display.print(vres);
  
  display.setCursor(5, h - 15);
  display.print("Display: ");
  display.print(w);
  display.print("x");
  display.print(h);
  display.print(" | Test Pattern");
}

void setup() {
  Serial.begin(115200);
  
  Serial.println("RP2350 HDMI EDID Reader + Color Bars Test");

  // PRIMERO: Inicializar salida HDMI (como en el cubo que funciona)
  Serial.println("\n=== INICIANDO SALIDA HDMI ===");
  Serial.print("Pines: CLK=");
  Serial.print(pinConfig.clk_p);
  Serial.print(", D0=");
  Serial.print(pinConfig.rgb_p[0]);
  Serial.print(", D1=");
  Serial.print(pinConfig.rgb_p[1]);
  Serial.print(", D2=");
  Serial.println(pinConfig.rgb_p[2]);
  
  if (!display.begin()) {
    Serial.println("ERROR: No se pudo inicializar HDMI");
    pinMode(LED_BUILTIN, OUTPUT);
    while (1) {
      digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
    }
  }

  Serial.println("HDMI inicializado correctamente");
  Serial.print("Resolución de salida: ");
  Serial.print(display.width());
  Serial.print(" x ");
  Serial.println(display.height());

  // SEGUNDO: Configurar I2C para leer EDID (después del display)
  Serial.println("\n=== CONFIGURANDO I2C ===");
  Wire.setSDA(24);
  Wire.setSCL(25);
  Wire.begin();
  delay(500);

  // Leer y mostrar EDID por Serial
  Serial.println("\n=== LECTURA EDID POR I2C ===");
  readEDID();
  printRaw();
  printEDID();

  // Leer otros dispositivos I2C
  readI2CDevice(0x37);
  readI2CDevice(0x4A);
  readI2CDevice(0x4B);

  // Dibujar barras de colores con info EDID
  Serial.println("\n=== DIBUJANDO BARRAS DE COLORES ===");
  display.fillScreen(0x0000);
  drawColorBars();

  Serial.println("Barras de colores mostradas en pantalla");
  Serial.println("Información EDID incluida en pantalla");
}

void loop() {
  // Patrón estático - no necesita actualización
}
