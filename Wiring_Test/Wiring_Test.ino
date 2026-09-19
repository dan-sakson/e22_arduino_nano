// Full wiring test for the Nano Every build:
//   D10 - Mode button (INPUT_PULLUP)
//   D9  - Trigger button (INPUT_PULLUP)
//   D6  - WS2812B RGB LED data pin
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
#define LED_PIN 6
#define LED_COUNT 10

const unsigned long DEBOUNCE_DELAY_MS = 50;
const unsigned long FLASH_DURATION_MS = 150;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
CRGB leds[LED_COUNT];

bool triggerLastReading = LOW;
bool triggerState = LOW;
unsigned long triggerLastDebounceTime = 0;

bool modeLastReading = HIGH;
bool modeState = HIGH;
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

// Debounces a pin and returns true on the falling edge (button pressed)
bool checkButtonPressed(uint8_t pin, bool &lastReading, bool &state, unsigned long &lastDebounceTime) {
  bool reading = digitalRead(pin);
  bool pressed = false;

  if (reading != lastReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (reading != state) {
      state = reading;
      if (state == LOW) {
        pressed = true;
      }
    }
  }

  lastReading = reading;
  return pressed;
}

void flashLedsWhite() {
  fill_solid(leds, LED_COUNT, CRGB::White);
  FastLED.show();
  delay(FLASH_DURATION_MS);
  fill_solid(leds, LED_COUNT, CRGB::Black);
  FastLED.show();
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

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Wiring Test"));
  display.setCursor(0, 10);
  display.println(F("OLED: OK"));
  display.display();
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

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_COUNT);
  FastLED.clear();
  FastLED.show();

  Serial.println(F("Wiring test ready. Press buttons to test."));
}

void loop() {
  if (checkButtonPressed(TRIGGER_PIN, triggerLastReading, triggerState, triggerLastDebounceTime)) {
    Serial.println(F("Trigger button pressed"));
    flashLedsWhite();
  }

  if (checkButtonPressed(MODE_BUTTON_PIN, modeLastReading, modeState, modeLastDebounceTime)) {
    Serial.println(F("Mode button pressed"));
    flashLedsWhite();
  }
}
