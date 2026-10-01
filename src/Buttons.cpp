#include "Buttons.h"

void Buttons::begin()
{
    pinMode(bStart.pin, INPUT_PULLUP);
    pinMode(bStop.pin, INPUT_PULLUP);
    pinMode(bPlus.pin, INPUT_PULLUP);
    pinMode(bMinus.pin, INPUT_PULLUP);
}

void Buttons::upd(B &b)
{
    b.shortEv = false;
    b.pressEv = false;
    b.l1Ev = false;
    b.l2Ev = false;
    b.l5Ev = false;

    bool raw = (digitalRead(b.pin) == LOW);

    if (raw != b.lastRaw)
    {
        b.dbMs = millis();
        b.lastRaw = raw;
    }

    if (millis() - b.dbMs > DB)
    {
        if (raw != b.stable)
        {
            b.stable = raw;

            if (b.stable)
            {
                b.downMs = millis();

                b.f1 = false;
                b.f2 = false;
                b.f5 = false;

                b.pressEv = true;
            }
            else if ((millis() - b.downMs) < L1)
            {
                b.shortEv = true;
            }
        }
    }

    if (b.stable)
    {
        uint32_t h = millis() - b.downMs;

        if (!b.f1 && h >= L1)
        {
            b.l1Ev = true;
            b.f1 = true;
        }

        if (!b.f2 && h >= L2)
        {
            b.l2Ev = true;
            b.f2 = true;
        }

        if (!b.f5 && h >= L5)
        {
            b.l5Ev = true;
            b.f5 = true;
        }
    }
}

void Buttons::update()
{
    startShort = false;
    startLong1s = false;
    startLong2s = false;

    stopShort = false;
    stopPress = false;
    stopLong2s = false;
    stopLong5s = false;

    plusShort = false;
    minusShort = false;

    startHeld = false;
    plusHeld = false;
    minusHeld = false;

    upd(bStart);
    upd(bStop);
    upd(bPlus);
    upd(bMinus);

    startShort = bStart.shortEv;
    startLong1s = bStart.l1Ev;
    startLong2s = bStart.l2Ev;

    stopShort = bStop.shortEv;
    stopPress = bStop.pressEv;
    stopLong2s = bStop.l2Ev;
    stopLong5s = bStop.l5Ev;

    plusShort = bPlus.shortEv;
    minusShort = bMinus.shortEv;

    startHeld = bStart.stable;
    plusHeld = bPlus.stable;
    minusHeld = bMinus.stable;
}

void Buttons::clearEvents()
{
    bStart.shortEv = bStart.pressEv = bStart.l1Ev = bStart.l2Ev = false;
    bStop.shortEv = bStop.pressEv =  bStop.l2Ev = bStop.l5Ev = false;
    bPlus.shortEv = bPlus.pressEv = bPlus.l1Ev = bPlus.l2Ev = false;
    bMinus.shortEv = bMinus.pressEv = bMinus.l1Ev = bMinus.l2Ev = false;

    startShort = startLong1s = startLong2s = false;
    stopShort = stopPress = stopLong2s = stopLong5s = false;
    plusShort = minusShort = false;

    startHeld = false;
    plusHeld = minusHeld = false;
}