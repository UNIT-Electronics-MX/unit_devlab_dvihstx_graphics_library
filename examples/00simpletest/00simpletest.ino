// 16-bit Adafruit_GFX-compatible framebuffer for RP2350 HSTX
// Configuración: D1(GPIO18)=Clock+, D7(GPIO12)=Data0+(Blue), D5(GPIO14)=Data1+(Green), D3(GPIO16)=Data2+(Red)

#include <DevLab_DVIHSTX_Graphics.h>


#if defined(ADAFRUIT_FEATHER_RP2350_HSTX)
DVHSTXPinout pinConfig = ADAFRUIT_FEATHER_RP2350_CFG;
#elif defined(ADAFRUIT_METRO_RP2350)
DVHSTXPinout pinConfig = ADAFRUIT_METRO_RP2350_CFG;
#elif defined(ARDUINO_ADAFRUIT_FRUITJAM_RP2350)
DVHSTXPinout pinConfig = ADAFRUIT_FRUIT_JAM_CFG;
#elif defined(ARDUINO_NANO_RP2350_CONNECT)
DVHSTXPinout pinConfig = ARDUINO_NANO_RP2350_DVI_CFG;
#elif (defined(ARDUINO_RASPBERRY_PI_PICO_2) || defined(ARDUINO_RASPBERRY_PI_PICO_2W))
DVHSTXPinout pinConfig = ADAFRUIT_HSTXDVIBELL_CFG;
#else
// Configuración estándar: {Clock+, Data0+(Blue), Data1+(Green), Data2+(Red)}
DVHSTXPinout pinConfig = {14, 18, 16, 12};
#endif


DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);
// If you get the message "error: 'DVHSTX_PINOUT_DEFAULTx' was not declared"
// then you need to give the pins numbers explicitly, like the example below.
// The order is: {CKP, D0P, D1P, D2P} DVHSTX16 display({12, 14, 16, 18},
// DVHSTX_RESOLUTION_320x240);

void setup() {
  Serial.begin(115200);
  // while(!Serial);
  if (!display.begin()) { // Blink LED if insufficient RAM
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;)
      digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  Serial.println("display initialized");
}

void loop() {
  // Draw random lines
  display.drawLine(random(display.width()), random(display.height()),
                   random(display.width()), random(display.height()),
                   random(65536));
  sleep_ms(1);
}
