#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_Qwiic_OLED.h>

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
                 uint8_t sliderValue,
                 uint8_t sliderAngle,
                 uint8_t sliderPwm1,
                 uint8_t sliderPwm2,
                 bool sliderActive,
                 bool dmxOk,
                 bool espNowOk,
                 bool pbPlaying,
                 uint8_t pbSlot1to9,
                 uint32_t pbRemainSec,
                 uint32_t pbSlotSec);

    void drawOtaMode(
        const String &ssid,
        const String &ip);

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
    Qwiic1in3OLED d;

    const char *modeName(InputMode m) const;

    void clearAndHome();
};