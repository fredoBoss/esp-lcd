// Type a name on the on-screen keyboard, press OK, get a welcome screen.
// First boot asks you to calibrate the touch screen (tap the 4 corner arrows).
// To recalibrate later, touch the screen while "Touch screen now..." is shown at startup.
// You can also type a name in the Serial Monitor (115200 baud) and press Enter.
#include <Preferences.h>
#include "LGFX_ESP32_3248S035R.h"

LGFX tft;
Preferences prefs;

// Special key codes
const char KEY_SHIFT = 1;
const char KEY_DEL = '\b';
const char KEY_OK = '\n';

struct Key {
  char code;
  int16_t x, y, w, h;
};

// Keyboard layout (portrait 320x480): text box on top, 5 rows of keys at the bottom
const int ROW_H = 52;
const int KEY_W = 32;
const int KB_TOP = 480 - 5 * ROW_H;
const int MAX_LEN = 20;

Key keys[40];
int keyCount = 0;

// Back button on the welcome screen
const int BACK_X = 85, BACK_Y = 380, BACK_W = 150, BACK_H = 50;

const lgfx::IFont* const FONTS[] = {
  &fonts::FreeSansBold24pt7b,
  &fonts::FreeSansBold18pt7b,
  &fonts::FreeSansBold12pt7b,
  &fonts::FreeSansBold9pt7b,
  &fonts::Font2,
};
const int FONT_COUNT = sizeof(FONTS) / sizeof(FONTS[0]);

String text;
bool shift = true;  // capitalize the first letter, like a phone keyboard
bool onWelcome = false;
bool touching = false;
int releaseCount = 0;
int pressedKey = -1;

void addKey(char code, int x, int y, int w) {
  keys[keyCount++] = { code, (int16_t)x, (int16_t)y, (int16_t)w, (int16_t)ROW_H };
}

void addRow(const char* chars, int x, int y) {
  for (const char* c = chars; *c; c++) {
    addKey(*c, x, y, KEY_W);
    x += KEY_W;
  }
}

void buildKeyboard() {
  int y = KB_TOP;
  addRow("1234567890", 0, y);
  y += ROW_H;
  addRow("qwertyuiop", 0, y);
  y += ROW_H;
  addRow("asdfghjkl", KEY_W / 2, y);
  y += ROW_H;
  addKey(KEY_SHIFT, 0, y, KEY_W * 3 / 2);
  addRow("zxcvbnm", KEY_W * 3 / 2, y);
  addKey(KEY_DEL, KEY_W * 17 / 2, y, KEY_W * 3 / 2);
  y += ROW_H;
  addKey(' ', 0, y, KEY_W * 6);
  addKey(KEY_OK, KEY_W * 6, y, KEY_W * 4);
}

String keyLabel(char code) {
  switch (code) {
    case KEY_SHIFT: return "Shift";
    case KEY_DEL: return "Del";
    case KEY_OK: return "OK";
    case ' ': return "Space";
    default: return String(shift ? (char)toupper(code) : code);
  }
}

// Use the biggest font (starting at FONTS[first]) that fits s in maxWidth pixels
void setFittingFont(const String& s, int maxWidth, int first = 0) {
  for (int i = first; i < FONT_COUNT; i++) {
    tft.setFont(FONTS[i]);
    if (tft.textWidth(s.c_str()) <= maxWidth) return;
  }
}

void drawKey(const Key& k, bool pressed) {
  uint16_t bg;  // uint16_t = RGB565 in LovyanGFX
  if (pressed) bg = TFT_WHITE;
  else if (k.code == KEY_OK) bg = TFT_DARKGREEN;
  else if (k.code == KEY_SHIFT && shift) bg = TFT_BLUE;
  else bg = TFT_DARKGREY;

  String label = keyLabel(k.code);
  tft.fillRoundRect(k.x + 2, k.y + 2, k.w - 4, k.h - 4, 6, bg);
  setFittingFont(label, k.w - 6, 2);  // 12pt, smaller for "Shift" on narrow keys
  tft.setTextColor(pressed ? TFT_BLACK : TFT_WHITE);
  tft.setTextDatum(middle_center);
  tft.drawString(label.c_str(), k.x + k.w / 2, k.y + k.h / 2);
}

void drawKeys() {
  for (int i = 0; i < keyCount; i++) drawKey(keys[i], false);
}

void drawTextBox() {
  String shown = text + "_";
  tft.fillRoundRect(10, 110, 300, 60, 8, TFT_WHITE);
  setFittingFont(shown, 280, 1);
  tft.setTextColor(TFT_BLACK);
  tft.setTextDatum(middle_left);
  tft.drawString(shown.c_str(), 20, 140);
}

void drawInputScreen() {
  onWelcome = false;
  tft.fillScreen(TFT_BLACK);
  tft.setFont(&fonts::FreeSans12pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(top_center);
  tft.drawString("Type your name,", tft.width() / 2, 25);
  tft.drawString("then press OK", tft.width() / 2, 55);
  drawTextBox();
  drawKeys();
}

void drawWelcomeScreen() {
  onWelcome = true;
  int cx = tft.width() / 2;
  tft.fillScreen(TFT_NAVY);
  tft.setTextDatum(middle_center);

  tft.setFont(&fonts::FreeSans18pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Welcome,", cx, 120);

  String name = text + "!";
  setFittingFont(name, 300);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString(name.c_str(), cx, 190);

  setFittingFont("Hello World!", 300);
  tft.setTextColor(TFT_GREEN);
  tft.drawString("Hello World!", cx, 270);

  tft.fillRoundRect(BACK_X, BACK_Y, BACK_W, BACK_H, 8, TFT_DARKGREY);
  tft.setFont(&fonts::FreeSansBold12pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Back", BACK_X + BACK_W / 2, BACK_Y + BACK_H / 2);

  Serial.printf("Welcome, %s! Hello World!\n", text.c_str());
}

void handleKey(char code) {
  bool oldShift = shift;
  if (code == KEY_OK) {
    if (text.length() > 0) drawWelcomeScreen();
    return;
  } else if (code == KEY_SHIFT) {
    shift = !shift;
  } else if (code == KEY_DEL) {
    if (text.length() > 0) text.remove(text.length() - 1);
  } else if (text.length() < MAX_LEN) {
    text += shift ? (char)toupper(code) : code;
    shift = (code == ' ');  // capitalize the start of each word
  }
  drawTextBox();
  if (shift != oldShift) drawKeys();  // letter labels change case
}

int findKey(int32_t x, int32_t y) {
  for (int i = 0; i < keyCount; i++) {
    const Key& k = keys[i];
    if (x >= k.x && x < k.x + k.w && y >= k.y && y < k.y + k.h) return i;
  }
  return -1;
}

void onPress(int32_t x, int32_t y) {
  if (onWelcome) {
    if (x >= BACK_X && x < BACK_X + BACK_W && y >= BACK_Y && y < BACK_Y + BACK_H) {
      text = "";
      shift = true;
      drawInputScreen();
    }
    return;
  }
  pressedKey = findKey(x, y);
  if (pressedKey >= 0) drawKey(keys[pressedKey], true);
}

void onRelease() {
  if (pressedKey < 0) return;
  const Key& k = keys[pressedKey];
  pressedKey = -1;
  drawKey(k, false);
  handleKey(k.code);
}

void showMessage(const char* line1, const char* line2) {
  tft.fillScreen(TFT_BLACK);
  tft.setFont(&fonts::FreeSans12pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(middle_center);
  tft.drawString(line1, tft.width() / 2, tft.height() / 2 - 15);
  tft.drawString(line2, tft.width() / 2, tft.height() / 2 + 15);
}

// Load saved touch calibration, or run calibration on first boot / when asked
void setupTouch() {
  uint16_t cal[8];
  int32_t x, y;
  prefs.begin("touch");
  bool haveCal = prefs.getBytes("cal", cal, sizeof(cal)) == sizeof(cal);

  if (haveCal) {
    tft.setTouchCalibrate(cal);
    showMessage("Touch screen now", "to recalibrate...");
    uint32_t start = millis();
    while (millis() - start < 1500) {
      if (tft.getTouch(&x, &y)) {
        haveCal = false;
        break;
      }
      delay(10);
    }
  }

  if (!haveCal) {
    while (tft.getTouch(&x, &y)) delay(10);  // wait for finger to lift
    showMessage("Touch calibration:", "tap each corner arrow");
    tft.calibrateTouch(cal, TFT_YELLOW, TFT_BLACK, 20);
    prefs.putBytes("cal", cal, sizeof(cal));
  }
  prefs.end();
}

void readSerialName() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;
  text = line.substring(0, MAX_LEN);
  drawWelcomeScreen();
}

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(0);  // portrait, 320 x 480 (use 2 if upside down)
  tft.setBrightness(255);

  setupTouch();
  buildKeyboard();
  drawInputScreen();
}

void loop() {
  readSerialName();

  int32_t x, y;
  if (tft.getTouch(&x, &y)) {
    releaseCount = 0;
    if (!touching) {
      touching = true;
      onPress(x, y);
    }
  } else if (touching && ++releaseCount >= 3) {  // debounce: 3 empty reads = released
    touching = false;
    onRelease();
  }
  delay(10);
}
