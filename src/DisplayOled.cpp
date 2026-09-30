#include "DisplayOled.h"
#include "Config.h"

#include <res/qw_fnt_5x7.h>
#include <res/qw_fnt_8x16.h>

static int servoRel(uint8_t angle)
{
    return (int)angle - 90;
}

const char *DisplayOled::modeName(InputMode m) const
{
    switch (m)
    {
    case InputMode::DMX:
        return "DMX";

    case InputMode::SLIDER:
        return "SLIDER";

    case InputMode::PLAYBACK:
        return "PLAYBACK";

    default:
        return "?";
    }
}

void DisplayOled::clearAndHome()
{
    d.erase();
}

void DisplayOled::begin()
{
    Wire.begin(21, 22);
    Wire.setTimeOut(50);

    if (d.begin(Wire, 0x3D) == false)
    {
        Serial.println("SparkFun OLED init failed");

        while (true)
            delay(100);
    }

    d.erase();
    d.text(0, 0, "OLED OK", 1);
    d.display();

    delay(300);
}

void DisplayOled::drawRun(
    const RuntimeSettings &rt,
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
    uint32_t pbSlotSec)
{
    d.erase();

    if (rt.inputMode == InputMode::DMX)
    {
        // Stor DMX-rubrik
        d.setFont(&QW_FONT_8X16);

        String title = String("DMX ") + String(rt.dmxAddress) + "-" + String(rt.dmxAddress + 2);
        d.text(0, 0, title, 1);
        d.text(1, 0, title, 1);

        // Liten OK / LOST längst till höger
        d.setFont(&QW_FONT_5X7);

        String statusText = dmxOk ? "OK" : "LOST";
        d.text(dmxOk ? 110 : 98, 4, statusText, 1);

        String servoText = String("Servo:") + String(currentValue);
        d.text(0, 24, servoText, 1);

        int a = servoRel(angle);

        String angleText = "Ang:";
        if (a >= 0)
            angleText += "+";

        angleText += String(a);
        d.text(70, 24, angleText, 1);

        String fwdText = String("FWD:") + String(pwm1);
        String revText = String("REV:") + String(pwm2);

        d.text(0, 40, fwdText, 1);
        d.text(70, 40, revText, 1);
    }
    else if (rt.inputMode == InputMode::SLIDER)
    {
        d.erase();

        d.line(78, 0, 78, 63, 1);

        // STOR rubrik
        d.setFont(&QW_FONT_8X16);

        String title = "SLIDER";
        d.text(0, 0, title, 1);
        d.text(1, 0, title, 1);

        String stateText = sliderActive ? "ON" : "OFF";
        d.text(88, 0, stateText, 1);
        d.text(89, 0, stateText, 1);

        // LITEN text från och med här
        d.setFont(&QW_FONT_5X7);

        // SERVO
        // Vänster = slider
        // Höger = faktiskt
        String servoLeft = String("Servo: ") + String(sliderValue);
        String servoRight = String(currentValue);

        d.text(0, 18, servoLeft, 1);
        d.text(86, 18, servoRight, 1);

        // ANGLE
        int sliderRel = servoRel(sliderAngle);

        String angleLeft = "Ang: ";
        if (sliderRel >= 0)
            angleLeft += "+";

        angleLeft += String(sliderRel);
        d.text(0, 30, angleLeft, 1);

        int actualRel = servoRel(angle);

        String angleRight;
        if (actualRel >= 0)
            angleRight = "+";

        angleRight += String(actualRel);
        d.text(86, 30, angleRight, 1);

        // MOTOR FWR
        String fwdLeft = String("FWR: ") + String(sliderPwm1);
        String fwdRight = String(pwm1);

        d.text(0, 42, fwdLeft, 1);
        d.text(86, 42, fwdRight, 1);

        // MOTOR REV
        String revLeft = String("REV: ") + String(sliderPwm2);
        String revRight = String(pwm2);

        d.text(0, 54, revLeft, 1);
        d.text(86, 54, revRight, 1);
    }
    else
    {
        d.setFont(&QW_FONT_8X16);

        String title = "PLAYBACK";

        if (pbPlaying)
        {
            title += " ";
            title += String(pbSlot1to9);
        }

        d.text(0, 0, title, 1);
        d.text(1, 0, title, 1);

        d.setFont(&QW_FONT_5X7);

        char totalBuf[8];
        snprintf(totalBuf, sizeof(totalBuf), "%lu:%02lu", pbSlotSec / 60, pbSlotSec % 60);

        if (pbPlaying)
        {
            char leftBuf[8];
            snprintf(leftBuf, sizeof(leftBuf), "%lu:%02lu", pbRemainSec / 60, pbRemainSec % 60);

            String leftText = String("Left:") + String(leftBuf);
            d.text(0, 24, leftText, 1);

            String pbText = String("PB:") + String(rt.selectedPlayback) + String(" ") + String(totalBuf);
            d.text(0, 40, pbText, 1);
        }
        else
        {
            String pbText = String("PB:") + String(rt.selectedPlayback) + String(" ") + String(totalBuf);
            d.text(0, 24, pbText, 1);

            d.text(0, 42, "CHOOSE PB: +/-", 1);
            d.text(0, 54, "START/STOP: PB", 1);
        }
    }

    if (espNowOk)
    {
        d.setFont(&QW_FONT_5X7);

        String espStatus = "OK";
        d.text(116, 56, espStatus, 1);
    }

    d.display();
}

void DisplayOled::drawMainMenu(
    const Menu &menu,
    const RuntimeSettings &edit,
    const Playback &playback)
{
    static const char *names[] = {
        "Input Mode",
        "DMX Address",
        "Playback",
        "Servo",
        "Motor PWM",
        "EXIT",
        "SAVE"};

    d.erase();

    const int visibleRows = 5;
    int selected = menu.mainIndex();

    int start = selected - 2;

    if (start < 0)
        start = 0;

    if (start > Menu::ITEM_COUNT - visibleRows)
        start = Menu::ITEM_COUNT - visibleRows;

    if (start < 0)
        start = 0;

    for (int row = 0; row < visibleRows; row++)
    {
        int i = start + row;

        if (i >= Menu::ITEM_COUNT)
            break;

        String line = (i == selected) ? ">" : " ";
        line += names[i];

        if (i == Menu::ITEM_INPUT_MODE)
        {
            line += ": ";
            line += modeName(edit.inputMode);
        }
        else if (i == Menu::ITEM_DMX_ADDRESS)
        {
            line += ": ";
            line += String(edit.dmxAddress);
        }
        else if (i == Menu::ITEM_PLAYBACK)
        {
            uint8_t recCount = 0;

            for (uint8_t s = 0; s < PLAYBACK_SLOTS; ++s)
            {
                if (playback.isRecorded(s))
                    recCount++;
            }

            line += ": ";
            line += String(recCount);
            line += "/";
            line += String(PLAYBACK_SLOTS);
        }
        else if (i == Menu::ITEM_SERVO_SETUP)
        {
            int minRel = servoRel(edit.servoMin);
            int maxRel = servoRel(edit.servoMax);

            line += ": ";

            if (minRel >= 0)
                line += "+";

            line += String(minRel);
            line += "/";

            if (maxRel >= 0)
                line += "+";

            line += String(maxRel);
        }
        else if (i == Menu::ITEM_MOTOR_PWM)
        {
            line += ": ";
            line += String(edit.pwmMin);
            line += "-";
            line += String(edit.pwmMax);
        }

        d.text(0, row * 12, line, 1);
    }

    d.display();
}

void DisplayOled::drawEditInput(const RuntimeSettings &edit)
{
    d.erase();

    d.text(0, 0, "EDIT Input", 1);
    d.text(0, 16, modeName(edit.inputMode), 1);
    d.text(0, 54, "+/-  START=Back", 1);

    d.display();
}

void DisplayOled::drawEditDmx(const RuntimeSettings &edit)
{
    d.erase();

    d.text(0, 0, "EDIT DMX Address", 1);

    String dmxText = String(edit.dmxAddress);
    d.text(0, 16, dmxText, 1);

    d.text(0, 54, "+/-  START=Back", 1);

    d.display();
}

void DisplayOled::drawPlaybackRecList(
    const Menu &menu,
    const Playback &playback)
{
    d.erase();

    d.text(0, 0, "Playback REC", 1);

    int idx = menu.recIndex();
    int maxIdx = PLAYBACK_SLOTS;
    int start = max(0, idx - 1);
    int end = min(maxIdx, start + 3);
    int row = 0;

    for (int i = start; i <= end; i++)
    {
        String line = (i == idx) ? ">" : " ";

        if (i < PLAYBACK_SLOTS)
        {
            line += "Slot ";
            line += String(i + 1);

            if (playback.isRecorded(i))
            {
                uint32_t sec = playback.slotSeconds(i);

                char buf[8];
                snprintf(buf, sizeof(buf), "%lu:%02lu", sec / 60, sec % 60);

                line += "  ";
                line += buf;
            }
        }
        else
        {
            line += "BACK";
        }

        d.text(0, 12 + row * 12, line, 1);
        row++;
    }

    d.display();
}

void DisplayOled::drawRecording(
    uint8_t slot,
    uint32_t recSec)
{
    d.erase();

    String title = String("RECORDING Slot ") + String(slot + 1);
    d.text(0, 0, title, 1);

    char buf[8];
    snprintf(buf, sizeof(buf), "%lu:%02lu", recSec / 60, recSec % 60);

    String timeText = String("Time: ") + String(buf);
    d.text(0, 16, timeText, 1);

    d.text(0, 54, "STOP to stop", 1);

    d.display();
}

void DisplayOled::drawServoSetup(
    const RuntimeSettings &edit,
    uint8_t index)
{
    d.erase();

    d.text(0, 0, "SERVO SETUP", 1);

    String minLine = (index == 0) ? ">" : " ";
    minLine += String(servoRel(edit.servoMin));
    d.text(0, 16, minLine, 1);

    String maxLine = (index == 1) ? ">" : " ";
    maxLine += "+";
    maxLine += String(servoRel(edit.servoMax));
    d.text(0, 28, maxLine, 1);

    String backLine = (index == 2) ? ">BACK" : " BACK";
    d.text(0, 40, backLine, 1);

    d.text(0, 56, "+/- START=Select", 1);

    d.display();
}

void DisplayOled::drawEditServoMin(const RuntimeSettings &edit)
{
    d.erase();

    d.text(0, 0, "EDIT SERVO MIN", 1);

    String servoMinText = String(servoRel(edit.servoMin));
    d.text(0, 20, servoMinText, 2);

    d.text(0, 54, "+/-  START=Back", 1);

    d.display();
}

void DisplayOled::drawEditServoMax(const RuntimeSettings &edit)
{
    d.erase();

    d.text(0, 0, "EDIT SERVO MAX", 1);

    String value = "+";
    value += String(servoRel(edit.servoMax));
    d.text(0, 20, value, 2);

    d.text(0, 54, "+/-  START=Back", 1);

    d.display();
}

void DisplayOled::drawMotorPwmSetup(
    const RuntimeSettings &edit,
    uint8_t index)
{
    d.erase();

    d.text(0, 0, "MOTOR PWM", 1);

    String minLine = (index == 0) ? ">" : " ";
    minLine += "MIN: ";
    minLine += String(edit.pwmMin);
    d.text(0, 16, minLine, 1);

    String maxLine = (index == 1) ? ">" : " ";
    maxLine += "MAX: ";
    maxLine += String(edit.pwmMax);
    d.text(0, 28, maxLine, 1);

    String backLine = (index == 2) ? ">BACK" : " BACK";
    d.text(0, 40, backLine, 1);

    d.text(0, 56, "+/- START=Select", 1);

    d.display();
}

void DisplayOled::drawEditPwmMin(const RuntimeSettings &edit)
{
    d.erase();

    d.text(0, 0, "EDIT PWM MIN", 1);

    String pwmMinText = String(edit.pwmMin);
    d.text(0, 20, pwmMinText, 2);

    d.text(0, 54, "+/- START=Back", 1);

    d.display();
}

void DisplayOled::drawEditPwmMax(const RuntimeSettings &edit)
{
    d.erase();

    d.text(0, 0, "EDIT PWM MAX", 1);

    String pwmMaxText = String(edit.pwmMax);
    d.text(0, 20, pwmMaxText, 2);

    d.text(0, 54, "+/- START=Back", 1);

    d.display();
}

void DisplayOled::drawOtaMode(
    const String &ssid,
    const String &ip)
{
    d.erase();

    // Stor rubrik
    d.setFont(&QW_FONT_8X16);

    String title = "OTA MODE";
    d.text(0, 0, title, 1);
    d.text(1, 0, title, 1);

    // Liten text
    d.setFont(&QW_FONT_5X7);

    String wifiLabel = "WiFi:";
    d.text(0, 20, wifiLabel, 1);

    String wifiName = ssid;
    d.text(0, 31, wifiName, 1);

    String ipLine = String("IP: ") + ip;
    d.text(0, 48, ipLine, 1);

    d.display();
}
