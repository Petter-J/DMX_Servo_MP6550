#pragma once
#include <Arduino.h>
#include "Config.h"

class Buttons
{
public:
    void begin();
    void update();
    void clearEvents();

    bool startShort = false;
    bool startLong1s = false;
    bool startLong2s = false;

    bool stopShort = false;
    bool stopLong1s = false;
    bool stopLong5s = false;

    bool plusShort = false;
    bool minusShort = false;

    bool startHeld = false;

    bool plusHeld = false;
    bool minusHeld = false;

private:
    struct B
    {
        uint8_t pin;
        bool stable = false;
        bool lastRaw = false;
        uint32_t dbMs = 0;
        uint32_t downMs = 0;
        bool f1 = false;
        bool f2 = false;
        bool f5 = false;

        bool shortEv = false;
        bool l1Ev = false;
        bool l2Ev = false;
        bool l5Ev = false;
    };

    B bStart{BTN_START_PIN}, bStop{BTN_STOP_PIN}, bPlus{BTN_PLUS_PIN}, bMinus{BTN_MINUS_PIN};
    static constexpr uint16_t DB = 30;
    static constexpr uint16_t L1 = 1000;
    static constexpr uint16_t L2 = 2200;
    static constexpr uint16_t L5 = 5000;

    void upd(B &b);
};