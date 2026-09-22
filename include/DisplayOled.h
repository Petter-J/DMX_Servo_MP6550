#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include "Settings.h"
#include "Menu.h"
#include "Playback.h"

class DisplayOled
{
public:
    void begin();

    void drawRun(const RuntimeSettings &rt,
                 uint8_t currentValue,
                 uint8_t angle,
                 uint8_t pwm1,
                 uint8_t pwm2,
                 bool pbPlaying,
                 uint8_t pbSlot1to9,
                 uint32_t pbRemainSec,
                 uint32_t pbSlotSec);

    void drawRecording(
        uint8_t slot,
        uint32_t recSec);

    void drawMainMenu(
        const Menu &menu,
        const RuntimeSettings &edit,
        const Playback &playback);

    void drawEditInput(
        const RuntimeSettings &edit);

    void drawEditDmx(
        const RuntimeSettings &edit);

    void drawPlaybackRecList(
        const Menu &menu,
        const Playback &playback);

    void drawServoSetup(
        const RuntimeSettings &edit,
        uint8_t index);

    void drawEditServoMin(
        const RuntimeSettings &edit);

    void drawEditServoMax(
        const RuntimeSettings &edit);

    void drawMotorPwmSetup(
        const RuntimeSettings &edit,
        uint8_t index);

    void drawEditPwmMin(
        const RuntimeSettings &edit);

    void drawEditPwmMax(
        const RuntimeSettings &edit);

private:
    Adafruit_SH1106G d{128, 64, &Wire, -1};

    const char *modeName(InputMode m) const;

    void clearAndHome();
};