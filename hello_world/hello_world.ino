#include "LGFX_ESP32_3248S035R.h"

LGFX tft;

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(0);  // portrait, 320 x 480 (use 2 if upside down)
  tft.setBrightness(255);
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(middle_center);
  tft.setFont(&fonts::FreeSansBold24pt7b);
  tft.drawString("Hello World!", tft.width() / 2, tft.height() / 2);
}

void loop() {
  Serial.println("Hello World!");
  delay(1000);
}
