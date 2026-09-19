#pragma once

#include <Arduino.h>
#include <FastLED.h>

const int DEFAULT_BRIGHTNESS = 128;

// Independently controllable LED groups that make up the E22 body's LED string.
enum LEDGroup
{
    REAR_RECEIVER_WINDOWS = 0,
    FRONT_RECEIVER_HOLES,
    UPPER_BARREL,
    LOWER_BARREL,
    LED_GROUP_COUNT
};

// Shared FastLED plumbing: owns the LED buffer, mode-color table, and the primitives
// (registering with FastLED, filling solid color, showing the current mode color) common
// to both LED controllers. Derived classes add only what's specific to their LED layout.
// Implementation lives in this header (not a .cpp) so the Arduino build links correctly for
// whatever DATA_PIN/MAX_LEDS combination each translation unit instantiates.
template <uint8_t DATA_PIN, int MAX_LEDS>
class LEDControllerBase
{
    protected:
        CRGB _leds[MAX_LEDS > 0 ? MAX_LEDS : 1];
        int _active_leds;
        const CRGB *_mode_colors;
        int _mode_color_count;
        int _mode_index;

        LEDControllerBase() :
            _active_leds(0),
            _mode_colors(nullptr),
            _mode_color_count(0),
            _mode_index(0) {}

        // Registers the first ledCount LEDs of the buffer with FastLED. Safe to call with ledCount == 0.
        void begin(int ledCount, const CRGB *modeColors, int modeColorCount)
        {
            _active_leds = constrain(ledCount, 0, MAX_LEDS);
            _mode_colors = modeColors;
            _mode_color_count = modeColorCount;

            if (_active_leds > 0)
            {
                FastLED.addLeds<WS2812B, DATA_PIN, GRB>(_leds, _active_leds);
                FastLED.setBrightness(DEFAULT_BRIGHTNESS);
            }
        }

    public:
        void setModeIndex(int modeIndex)
        {
            if (modeIndex >= 0 && modeIndex < _mode_color_count)
            {
                _mode_index = modeIndex;
            }
        }

        void showColor(CRGB color)
        {
            if (_active_leds <= 0)
            {
                return;
            }
            fill_solid(_leds, _active_leds, color);
            FastLED.show();
        }

        void off() { showColor(CRGB::Black); }

        void showModeColor()
        {
            if (_mode_color_count > 0)
            {
                showColor(_mode_colors[_mode_index]);
            }
        }
};

// Controller for the Ammo Counter's own mode-indicator LED.
template <uint8_t DATA_PIN, int NUM_LEDS>
class AmmoCounterLEDController : public LEDControllerBase<DATA_PIN, NUM_LEDS>
{
    public:
        void init(const CRGB *modeColors, int modeColorCount)
        {
            this->begin(NUM_LEDS, modeColors, modeColorCount);
        }
};

// Controller for the E22 body's LED string. The string is split into LED_GROUP_COUNT
// independently addressable groups; the number of LEDs in each group (including zero)
// is configured at runtime via init(), so the overall body can have any number of LEDs,
// including none at all.
template <uint8_t DATA_PIN, int MAX_LEDS>
class BodyLEDController : public LEDControllerBase<DATA_PIN, MAX_LEDS>
{
    private:
        int _group_start[LED_GROUP_COUNT];
        int _group_count[LED_GROUP_COUNT];
    public:
        BodyLEDController()
        {
            for (int i = 0; i < LED_GROUP_COUNT; i++)
            {
                _group_start[i] = 0;
                _group_count[i] = 0;
            }
        }

        // groupCounts is indexed by LEDGroup and may contain zeros for groups (or the whole body) that don't exist.
        void init(const int groupCounts[LED_GROUP_COUNT], const CRGB *modeColors, int modeColorCount)
        {
            int start = 0;
            for (int i = 0; i < LED_GROUP_COUNT; i++)
            {
                int count = constrain(groupCounts[i], 0, MAX_LEDS - start);
                _group_start[i] = start;
                _group_count[i] = count;
                start += count;
            }
            this->begin(start, modeColors, modeColorCount);
        }

        int totalLeds() const { return this->_active_leds; }
        int groupLedCount(LEDGroup group) const { return _group_count[group]; }

        void showGroupColor(LEDGroup group, CRGB color)
        {
            int count = _group_count[group];
            if (count <= 0)
            {
                return;
            }
            fill_solid(&this->_leds[_group_start[group]], count, color);
            FastLED.show();
        }

        void offGroup(LEDGroup group) { showGroupColor(group, CRGB::Black); }

        void showGroupModeColor(LEDGroup group)
        {
            if (this->_mode_color_count > 0)
            {
                showGroupColor(group, this->_mode_colors[this->_mode_index]);
            }
        }
};