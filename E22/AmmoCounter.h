#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansOblique9pt7b.h>
#include <Wire.h>

const int DISPLAY_ROTATION = 1;
// Splash screens are shown rotated 90 degrees counter-clockwise from the ammo counter view,
// since the ammo counter's landscape orientation is too narrow for the splash text.
const int SPLASH_DISPLAY_ROTATION = (DISPLAY_ROTATION + 3) % 4;
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 32;
const int OLED_RESET = -1;
const int SCREEN_ADDRESS = 0x3C;
const int DEFAULT_TEXT_SIZE = 1;
const int AMMO_TEXT_SIZE = 2;
const uint8_t MAX_CONTRAST = 255;
const uint8_t FADE_CONTRAST_STEP = 15;
const uint8_t FADE_CONTRAST_STEPS = MAX_CONTRAST / FADE_CONTRAST_STEP;
const uint8_t MIN_CONTRAST = 0;

// Enum for text justification options
enum TextJustification
{
    LEFT,
    CENTER,
    RIGHT
};

// Struct to map splash screen text to their corresponding fonts
struct SplashScreen
{
    const char *text;
    const GFXfont *font;
    const unsigned long holdDurationMs;
};

// Struct to serve as dictionary mapping modes to their corresponding LED colors
struct ModeScreen
{
    const char *mode;
    const GFXfont *font;
    const bool showsAmmo;
};

class AmmoCounter
{
    private:
        int _mode_index;
        int _ammo_count;
        int _max_ammo;
        const ModeScreen *_modes;
        int _mode_count;
        const SplashScreen *_splashes;
        int _splash_count;
        Adafruit_SSD1306 _display;

        // Non-blocking splash screen playback state.
        bool _splashing;
        int _splash_index;
        bool _splash_fading;
        unsigned long _phase_start_time;
        unsigned long _splash_start_time;
        unsigned long _splash_total_duration;
        uint8_t _fade_contrast;

        void beginSplashEntry(int index, unsigned long now);
    public:
        AmmoCounter(int maxAmmo, const ModeScreen *modes, int modeCount, const SplashScreen *splashes = nullptr, int splashCount = 0);

        void setContrast(uint8_t contrast);
        void setModeIndex(int modeIndex);
        void decrementAmmo();
        void prepareDisplay(const GFXfont *font = nullptr, int textSize = 1, int rotation = DISPLAY_ROTATION);
        void printText(const char *text, int textSize, TextJustification justification = CENTER, const GFXfont *font = nullptr, int y = -1);
        void drawCounter();

        // Advances the splash screen animation; must be called every loop iteration.
        // No-op once the splash sequence has finished.
        void update(unsigned long now);
        bool isSplashing() const;
        // Overall splash sequence progress, from 0.0 (just started) to 1.0 (finished).
        float splashProgress() const;
};
