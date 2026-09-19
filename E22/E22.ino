// Ammo Counter for Arduino Nano Every
// Requires: "Adafruit SSD1306" and "Adafruit GFX Library" (install via Library Manager)
// Wiring: Display VCC->5V, GND->GND, SDA->A4, SCL->A5
// Display is physically mounted vertically with pins at the bottom, so its
// content is rotated 90 degrees via DISPLAY_ROTATION below.

#include "LED.h"
#include "AmmoCounter.h"
#include "Button.h"
#include <FastLED.h>

// Pin configuration
#define AMMO_LED_PIN 6
#define BODY_LED_PIN 7
#define TRIGGER_PIN 9
#define MODE_BUTTON_PIN 10

#define AMMO_LED_COUNT 1     // The Ammo Counter's own mode-indicator LED
#define MAX_BODY_LED_COUNT 8 // Upper bound on LEDs in the E22 body string (across all groups)

const int MAX_AMMO = 99;
const unsigned long TRIGGER_REPEAT_INTERVAL_MS = 200; // 5 shots/sec while trigger is held
const int TRIGGER_PRESSED_STATE = HIGH;               // trigger is normally-closed (closed unless pressed), so pressing it reads HIGH

// Number of LEDs in each body LED group. Any group (or all of them) may be 0 if that
// part of the body has no LEDs installed. Update to match the physical build.
// const int BODY_LED_GROUP_COUNTS[LED_GROUP_COUNT] = {
//     6, // REAR_RECEIVER_WINDOWS
//     2, // FRONT_RECEIVER_HOLES
//     0, // UPPER_BARREL
//     0, // LOWER_BARREL
// };
const int BODY_LED_GROUP_COUNTS[LED_GROUP_COUNT] = {
    0, // REAR_RECEIVER_WINDOWS
    0, // FRONT_RECEIVER_HOLES
    0, // UPPER_BARREL
    0, // LOWER_BARREL
};

// Ammo Counter LED flash rate during the splash screen, speeding up as it progresses.
const unsigned long SPLASH_FLASH_MAX_INTERVAL_MS = 400;
const unsigned long SPLASH_FLASH_MIN_INTERVAL_MS = 60;

// Dictionary mapping modes to their corresponding LED colors
const ModeScreen MODE_SCREENS[] = {
    {"Safe", nullptr, false},
    {"Stun", nullptr, false},
    {"Kill", nullptr, true},
};
const CRGB MODE_COLORS[] =
    {
        CRGB::Green,
        CRGB::Blue,
        CRGB::Red,
};
const int MODE_COUNT = sizeof(MODE_COLORS) / sizeof(MODE_COLORS[0]);

// Dictionary mapping splash screens to their corresponding fonts
const SplashScreen SPLASH_SCREENS[] = {
    {"BlasTech", &FreeSans9pt7b, 5000},
    {"E-22", &FreeSans9pt7b, 5000},
    {"Happy Hunting", &FreeSansOblique9pt7b, 5000},
};
const int SPLASH_SCREEN_COUNT = sizeof(SPLASH_SCREENS) / sizeof(SPLASH_SCREENS[0]);

int modeIndex = 0;

int modeButtonLastReading = HIGH;
int modeButtonState = HIGH;
unsigned long modeButtonLastDebounceTime = 0;

int triggerButtonLastReading = LOW;
int triggerButtonState = LOW;
unsigned long triggerButtonLastDebounceTime = 0;
unsigned long lastTriggerFireTime = 0;

bool mode_updated = false;
bool trigger_updated = false;

// Tracks whether the splash sequence was still active on the previous loop iteration,
// so we can detect the exact moment it finishes and switch the LEDs to their solid mode color.
bool wasSplashing = true;
bool splashFlashOn = false;
unsigned long lastSplashFlashToggleTime = 0;

AmmoCounterLEDController<AMMO_LED_PIN, AMMO_LED_COUNT> *ammoLed;
BodyLEDController<BODY_LED_PIN, MAX_BODY_LED_COUNT> *bodyLeds = nullptr; // stays null when no body LED groups are configured
AmmoCounter *ammoCounter;

// Returns true if the current mode is "Kill".
bool modeIsKill()
{
    return MODE_SCREENS[modeIndex].mode == "Kill";
}

// Sum of all configured body LED group counts, used to decide whether the body LED controller is needed at all.
int sumBodyLedGroupCounts()
{
    int total = 0;
    for (int i = 0; i < LED_GROUP_COUNT; i++)
    {
        total += BODY_LED_GROUP_COUNTS[i];
    }
    return total;
}

// Flashes the Ammo Counter's LED white, speeding up as the splash sequence progresses.
void updateSplashFlash(unsigned long now)
{
    float progress = ammoCounter->splashProgress();
    unsigned long interval = SPLASH_FLASH_MAX_INTERVAL_MS -
                             (unsigned long)((SPLASH_FLASH_MAX_INTERVAL_MS - SPLASH_FLASH_MIN_INTERVAL_MS) * progress);

    if (now - lastSplashFlashToggleTime >= interval)
    {
        splashFlashOn = !splashFlashOn;
        ammoLed->showColor(splashFlashOn ? CRGB::White : CRGB::Black);
        lastSplashFlashToggleTime = now;
    }
}

// Applies the current mode's color to both the Ammo Counter LED and the body LEDs (if any are registered).
void applyModeColorToLeds()
{
    ammoLed->showModeColor();
    if (bodyLeds)
    {
        bodyLeds->showModeColor();
    }
}

void setup()
{
    // Initialize button pins
    pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(TRIGGER_PIN, INPUT_PULLUP);

    // Initialize the ammo counter display (this shows the splash screens and then goes to the initial ammo count view)
    ammoCounter = new AmmoCounter(MAX_AMMO, MODE_SCREENS, MODE_COUNT, SPLASH_SCREENS, SPLASH_SCREEN_COUNT);

    // Initialize the Ammo Counter's own indicator LED
    ammoLed = new AmmoCounterLEDController<AMMO_LED_PIN, AMMO_LED_COUNT>();
    ammoLed->init(MODE_COLORS, MODE_COUNT);

    // Initialize the E22 body LEDs (if any are configured); they stay off until the splash sequence finishes
    if (sumBodyLedGroupCounts() > 0)
    {
        bodyLeds = new BodyLEDController<BODY_LED_PIN, MAX_BODY_LED_COUNT>();
        bodyLeds->init(BODY_LED_GROUP_COUNTS, MODE_COLORS, MODE_COUNT);
        bodyLeds->off();
    }
}

void loop()
{
    unsigned long now = millis();

    ammoCounter->update(now);

    if (ammoCounter->isSplashing())
    {
        updateSplashFlash(now);
    }
    else if (wasSplashing)
    {
        // Splash sequence just finished: switch the LEDs to solid mode color.
        applyModeColorToLeds();
        wasSplashing = false;
    }

    // Do not process button inputs while the splash sequence is active or was just active.
    if (!ammoCounter->isSplashing() && !wasSplashing)
    {
        if (checkButtonPressed(MODE_BUTTON_PIN, modeButtonLastReading, modeButtonState, modeButtonLastDebounceTime))
        {
            modeIndex = (modeIndex + 1) % MODE_COUNT;
            ammoLed->setModeIndex(modeIndex);
            if (bodyLeds)
            {
                bodyLeds->setModeIndex(modeIndex);
            }
            ammoCounter->setModeIndex(modeIndex);
            mode_updated = true;
        }

        updateDebouncedState(TRIGGER_PIN, triggerButtonLastReading, triggerButtonState, triggerButtonLastDebounceTime);
        if (triggerButtonState == TRIGGER_PRESSED_STATE && (millis() - lastTriggerFireTime) >= TRIGGER_REPEAT_INTERVAL_MS && modeIsKill())
        {
            ammoCounter->decrementAmmo();
            trigger_updated = true;
            lastTriggerFireTime = millis();
        }

        if (mode_updated)
        {
            applyModeColorToLeds();
            ammoCounter->drawCounter();
            mode_updated = false;
        }
        if (trigger_updated)
        {
            ammoCounter->drawCounter();
            trigger_updated = false;
        }
    }
}