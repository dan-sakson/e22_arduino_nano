#pragma once

#include <Arduino.h>

const unsigned long DEBOUNCE_DELAY_MS = 50;

// Returns true once on the transition into pressedState, after debouncing.
// pressedState defaults to LOW, matching a normally-open button (open unless pressed) wired
// with INPUT_PULLUP. Pass HIGH for a normally-closed button (closed unless pressed).
inline bool checkButtonPressed(int pin, int &lastReading, int &state, unsigned long &lastDebounceTime, int pressedState = LOW)
{
    int reading = digitalRead(pin);
    bool pressed = false;

    if (reading != lastReading)
    {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS)
    {
        if (reading != state)
        {
            state = reading;
            if (state == pressedState)
            {
                pressed = true;
            }
        }
    }

    lastReading = reading;
    return pressed;
}

// Updates a debounced button state without regard to edges (used for auto-repeat).
inline void updateDebouncedState(int pin, int &lastReading, int &state, unsigned long &lastDebounceTime)
{
    int reading = digitalRead(pin);

    if (reading != lastReading)
    {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS)
    {
        state = reading;
    }

    lastReading = reading;
}
