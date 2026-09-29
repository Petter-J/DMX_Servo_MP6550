#pragma once

#include <Arduino.h>
#include "Config.h"

class InputSources
{
public:
    void begin();

    uint8_t readDmx(uint16_t address);

    void readDmx3(
        uint16_t address,
        uint8_t &servo,
        uint8_t &pwm1,
        uint8_t &pwm2);

    uint8_t readSlider();
    uint8_t readPwmSlider();

    bool dmxOk() const;

private:
    static constexpr uint8_t SLIDER_PIN =
        SLIDER_PIN_CFG;

    static constexpr uint8_t PWM_SLIDER_PIN =
        PWM_SLIDER_PIN_CFG;

    uint32_t lastDmxPacketMs = 0;

    static constexpr uint32_t
        DMX_LOST_TIMEOUT_MS = 1500;
};