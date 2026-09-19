// Full wiring test for the Nano Every build:
//   D10 - Mode button (INPUT_PULLUP, normally open - pressed reads LOW)
//   D9  - Trigger button (INPUT_PULLUP, normally closed - pressed reads HIGH)
//   D6  - Ammo Counter LED data pin (single mode-indicator LED)
//   D7  - E22 body LED string data pin
//   SDA/SCL - SSD1306 OLED (128x32)
// Requires: Adafruit_SSD1306, Adafruit_GFX, FastLED

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

#define MODE_BUTTON_PIN 10
#define TRIGGER_PIN 9
#define AMMO_LED_PIN 6
#define AMMO_LED_COUNT 1
#define BODY_LED_PIN 7
#define BODY_LED_COUNT 8

const unsigned long DEBOUNCE_DELAY_MS = 50;
const unsigned long FLASH_INTERVAL_MS = 300;
const int TRIGGER_PRESSED_STATE = HIGH; // trigger is normally-closed (closed unless pressed)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
CRGB ammoLeds[AMMO_LED_COUNT];
CRGB bodyLeds[BODY_LED_COUNT];

// Colors cycled through on each button press, applied to whichever LED string that button controls.
const CRGB CYCLE_COLORS[] = { CRGB::White, CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow, CRGB::Cyan, CRGB::Magenta };
const char *CYCLE_COLOR_NAMES[] = { "White", "Red", "Green", "Blue", "Yellow", "Cyan", "Magenta" };
const int CYCLE_COLOR_COUNT = sizeof(CYCLE_COLORS) / sizeof(CYCLE_COLORS[0]);

int ammoColorIndex = 0;
int bodyColorIndex = 0;
bool flashOn = false;
unsigned long lastFlashToggleTime = 0;

int triggerLastReading = LOW;
int triggerState = LOW;
unsigned long triggerLastDebounceTime = 0;

int modeLastReading = HIGH;
int modeState = HIGH;
unsigned long modeLastDebounceTime = 0;

// Returns the first responding I2C address found on the bus, or 0 if none
uint8_t scanForI2cAddress() {
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      return address;
    }
  }
  return 0;
}

// Debounces a pin and returns true on the transition into pressedState.
bool checkButtonPressed(uint8_t pin, int &lastReading, int &state, unsigned long &lastDebounceTime, int pressedState) {
  int reading = digitalRead(pin);
  bool pressed = false;

  if (reading != lastReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (reading != state) {
      state = reading;
      if (state == pressedState) {
        pressed = true;
      }
    }
  }

  lastReading = reading;
  return pressed;
}

void showStatus(const char *line1, const char *line2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(line1);
  display.setCursor(0, 10);
  display.println(line2);
  display.display();
}

void initDisplay() {
  Serial.println(F("Scanning I2C bus for OLED display..."));
  uint8_t oledAddress = scanForI2cAddress();

  if (oledAddress == 0) {
    Serial.println(F("No I2C device found. Check OLED wiring."));
    return;
  }

  Serial.print(F("Found I2C device at address 0x"));
  Serial.println(oledAddress, HEX);

  if (!display.begin(SSD1306_SWITCHCAPVCC, oledAddress)) {
    Serial.println(F("SSD1306 allocation failed."));
    return;
  }

  showStatus("Wiring Test", "OLED: OK");
  Serial.println(F("Display initialized successfully."));
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for native USB serial to connect
  }

  Wire.begin();
  initDisplay();

  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);

  FastLED.addLeds<WS2812B, AMMO_LED_PIN, GRB>(ammoLeds, AMMO_LED_COUNT);
  FastLED.addLeds<WS2812B, BODY_LED_PIN, GRB>(bodyLeds, BODY_LED_COUNT);
  FastLED.clear();
  FastLED.show();

  Serial.println(F("Wiring test ready. Press buttons to test."));
}

void loop() {
  unsigned long now = millis();

  // Continuously flash both LED strings on/off using whichever color is currently selected for each.
  if (now - lastFlashToggleTime >= FLASH_INTERVAL_MS) {
    flashOn = !flashOn;
    fill_solid(ammoLeds, AMMO_LED_COUNT, flashOn ? CYCLE_COLORS[ammoColorIndex] : CRGB::Black);
    fill_solid(bodyLeds, BODY_LED_COUNT, flashOn ? CYCLE_COLORS[bodyColorIndex] : CRGB::Black);
    FastLED.show();
    lastFlashToggleTime = now;
  }

  if (checkButtonPressed(MODE_BUTTON_PIN, modeLastReading, modeState, modeLastDebounceTime, LOW)) {
    ammoColorIndex = (ammoColorIndex + 1) % CYCLE_COLOR_COUNT;
    Serial.print(F("Mode button pressed - Ammo LED color: "));
    Serial.println(CYCLE_COLOR_NAMES[ammoColorIndex]);
    showStatus("Mode button pressed", CYCLE_COLOR_NAMES[ammoColorIndex]);
  }

  if (checkButtonPressed(TRIGGER_PIN, triggerLastReading, triggerState, triggerLastDebounceTime, TRIGGER_PRESSED_STATE)) {
    bodyColorIndex = (bodyColorIndex + 1) % CYCLE_COLOR_COUNT;
    Serial.print(F("Trigger button pressed - Body LED color: "));
    Serial.println(CYCLE_COLOR_NAMES[bodyColorIndex]);
    showStatus("Trigger pressed", CYCLE_COLOR_NAMES[bodyColorIndex]);
  }
}

