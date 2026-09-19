#include "AmmoCounter.h"

AmmoCounter::AmmoCounter(int maxAmmo, const ModeScreen *modes, int modeCount, const SplashScreen *splashes, int splashCount) :
    _mode_index(0),
    _max_ammo(maxAmmo),
    _ammo_count(maxAmmo),
    _modes(modes),
    _mode_count(modeCount),
    _splashes(splashes),
    _splash_count(splashCount),
    _display(Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET)),
    _splashing(false),
    _splash_index(0),
    _splash_fading(false),
    _phase_start_time(0),
    _splash_start_time(0),
    _splash_total_duration(0),
    _fade_contrast(MAX_CONTRAST)
{
    if (!_display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed - check wiring/address"));
        while (true)
        {
            ; // halt
        }
    }

    for (int i = 0; i < _splash_count; i++)
    {
        _splash_total_duration += _splashes[i].holdDurationMs;
    }
    if (_splash_total_duration == 0)
    {
        _splash_total_duration = 1; // avoid divide-by-zero in splashProgress()
    }

    if (_splash_count > 0)
    {
        _splashing = true;
        unsigned long now = millis();
        _splash_start_time = now;
        beginSplashEntry(0, now);
    }
    else
    {
        drawCounter();
    }
}

void AmmoCounter::setContrast(uint8_t contrast)
{
    _display.ssd1306_command(SSD1306_SETCONTRAST);
    _display.ssd1306_command(constrain(contrast, MIN_CONTRAST, MAX_CONTRAST));
}

void AmmoCounter::setModeIndex(int modeIndex)
{
    if (modeIndex >= 0 && modeIndex < _mode_count)
    {
        _mode_index = modeIndex;
    }
}

void AmmoCounter::decrementAmmo()
{
    if (_ammo_count > 0)
    {
        _ammo_count--;
    }
    else
    {
        _ammo_count = _max_ammo;
    }
}

void AmmoCounter::prepareDisplay(const GFXfont *font, int textSize, int rotation)
{
    setContrast(255);
    _display.setRotation(rotation);
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);
    _display.setTextSize(textSize);
    _display.setFont(font);
}

void AmmoCounter::printText(const char *text, int textSize, TextJustification justification, const GFXfont *font, int y)
{
    // measure the actual rendered ink so it can be centered exactly, regardless of font metrics
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    _display.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);

    int startX = 0 - x1;
    switch (justification)
    {
        case CENTER:
            startX = ((_display.width() - textWidth) / 2) - x1;
            break;
        case RIGHT:
            startX = _display.width() - textWidth;
            break;
    }
    int startY = (y < 0) ? (((_display.height() - textHeight) / 2) - y1) : (y - y1);

    _display.setCursor(startX, startY);
    _display.println(text);
}

void AmmoCounter::beginSplashEntry(int index, unsigned long now)
{
    prepareDisplay(_splashes[index].font, DEFAULT_TEXT_SIZE, SPLASH_DISPLAY_ROTATION);
    printText(_splashes[index].text, DEFAULT_TEXT_SIZE, CENTER, _splashes[index].font);
    _display.display();

    _splash_index = index;
    _splash_fading = false;
    _phase_start_time = now;
    _fade_contrast = MAX_CONTRAST;
}

void AmmoCounter::update(unsigned long now)
{
    if (!_splashing)
    {
        return;
    }

    const SplashScreen &splash = _splashes[_splash_index];
    unsigned long halfDuration = splash.holdDurationMs / 2;

    if (!_splash_fading)
    {
        if (now - _phase_start_time >= halfDuration)
        {
            _splash_fading = true;
            _phase_start_time = now;
        }
        return;
    }

    unsigned long stepDuration = halfDuration / FADE_CONTRAST_STEPS;
    if (stepDuration == 0)
    {
        stepDuration = 1;
    }
    if (now - _phase_start_time < stepDuration)
    {
        return;
    }
    _phase_start_time = now;

    if (_fade_contrast <= FADE_CONTRAST_STEP)
    {
        _fade_contrast = MIN_CONTRAST;
    }
    else
    {
        _fade_contrast -= FADE_CONTRAST_STEP;
    }
    setContrast(_fade_contrast);

    if (_fade_contrast == MIN_CONTRAST)
    {
        int nextIndex = _splash_index + 1;
        if (nextIndex >= _splash_count)
        {
            _splashing = false;
            _display.clearDisplay();
            _display.display();
            drawCounter();
        }
        else
        {
            beginSplashEntry(nextIndex, now);
        }
    }
}

bool AmmoCounter::isSplashing() const
{
    return _splashing;
}

float AmmoCounter::splashProgress() const
{
    if (!_splashing)
    {
        return 1.0f;
    }
    unsigned long elapsed = millis() - _splash_start_time;
    float progress = (float)elapsed / (float)_splash_total_duration;
    return constrain(progress, 0.0f, 1.0f);
}

void AmmoCounter::drawCounter()
{
    prepareDisplay(_modes[_mode_index].font, DEFAULT_TEXT_SIZE);

    _display.setTextSize(AMMO_TEXT_SIZE);
    char ammoText[4];
    char maxText[4];
    if (_modes[_mode_index].showsAmmo)
    {
        snprintf(ammoText, sizeof(ammoText), "%d", _ammo_count);
        snprintf(maxText, sizeof(maxText), "%d", _max_ammo);
    }
    else
    {
        snprintf(ammoText, sizeof(ammoText), "-");
        snprintf(maxText, sizeof(maxText), "-");
    }

    // Draw the ammo count and max ammo right-aligned on the display
    printText(ammoText, 0, RIGHT, nullptr, 0);
    printText("/", 0, RIGHT, nullptr, SCREEN_HEIGHT / 2);
    printText(maxText, 0, RIGHT, nullptr, SCREEN_HEIGHT);

    // Draw the current mode at the bottom-left corner of the display, using the default built-in
    // font/size regardless of the mode's custom font so the label can't overflow its narrow row.
    _display.setFont(nullptr);
    _display.setTextSize(DEFAULT_TEXT_SIZE);
    printText(_modes[_mode_index].mode, DEFAULT_TEXT_SIZE, LEFT, nullptr, _display.height() - 8);

    _display.display();
}