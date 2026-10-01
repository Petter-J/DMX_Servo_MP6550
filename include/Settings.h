#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

enum class InputMode : uint8_t
{
    DMX = 0,
    SLIDER = 1,
    PLAYBACK = 2
};

inline InputMode nextInputMode(InputMode m)
{
    return static_cast<InputMode>(
        (static_cast<uint8_t>(m) + 1) % 3);
}

inline InputMode prevInputMode(InputMode m)
{
    int v = static_cast<int>(m) - 1;

    if (v < 0)
        v = 2;

    return static_cast<InputMode>(v);
}

struct RuntimeSettings
{
    InputMode inputMode = InputMode::DMX;

    uint8_t receiverIndex = 0;

    uint16_t dmxAddress = 1;
    uint8_t selectedPlayback = 1;

    uint8_t playbackStopValue =
        PLAYBACK_STOP_VALUE_DEFAULT;

    uint8_t servoMin = 10;
    uint8_t servoMax = 170;

    // Motor PWM-gränser
    uint8_t pwmMin = 0;
    uint8_t pwmMax = 255;
};

class SettingsStore
{
public:
    void begin();

    RuntimeSettings load();

    void save(
        const RuntimeSettings &s);

private:
    Preferences prefs;

    void clamp(
        RuntimeSettings &s);
};