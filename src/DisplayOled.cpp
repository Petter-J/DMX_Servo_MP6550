#include "DisplayOled.h"
#include "Config.h"

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
    d.clearDisplay();
    d.setCursor(0, 0);
}

void DisplayOled::begin()
{
    Wire.begin(21, 22);
    Wire.setTimeOut(50);

    if (!d.begin(0x3C, true))
    {
        Serial.println("SH1106 init failed");
        while (true)
            delay(100);
    }

    d.setTextColor(SH110X_WHITE);
    d.setTextSize(1);
    clearAndHome();
    d.println("OLED OK (SH1106)");
    d.display();
    delay(300);
}

void DisplayOled::drawRun(const RuntimeSettings &rt,
                          uint8_t currentValue,
                          uint8_t angle,
                          uint8_t pwm1,
                          uint8_t pwm2,
                          bool pbPlaying,
                          uint8_t pbSlot1to9,
                          uint32_t pbRemainSec,
                          uint32_t pbSlotSec)
{
    d.clearDisplay();
    d.setTextColor(SH110X_WHITE);

    d.setTextSize(2);
    d.setCursor(0, 0);

    switch (rt.inputMode)
    {
    case InputMode::DMX:
        break;
    case InputMode::SLIDER:
        d.print("SLIDER");
        break;
    case InputMode::PLAYBACK:
        d.print("PLAYBACK");
        if (pbPlaying)
        {
            d.print(" ");
            d.print(pbSlot1to9);
        }
        break;
    default:
        d.print("?");
        break;
    }

    d.setTextSize(2);
    d.setCursor(0, 22);

    if (rt.inputMode == InputMode::DMX)
    {
        d.setTextSize(2);
        d.setCursor(0, 0);
        d.print("DMX ");
        d.print(rt.dmxAddress);

        d.setTextSize(1);

        d.setCursor(0, 24);
        d.print("Servo:");
        d.print(currentValue);

        d.setCursor(70, 24);
        d.print("Ang:");

        int a = servoRel(angle);

        if (a >= 0)
            d.print("+");

        d.print(a);

        d.setCursor(0, 40);
        d.print("FWD:");
        d.print(pwm1);

        d.setCursor(70, 40);
        d.print("REV:");
        d.print(pwm2);
    }
    else if (rt.inputMode == InputMode::SLIDER)
    {
        d.setTextSize(2);
        d.setCursor(0, 0);
        d.print("SLIDER");

        d.setTextSize(1);

        d.setCursor(0, 24);
        d.print("Servo:");
        d.print(currentValue);

        d.setCursor(70, 24);
        d.print("Ang:");

        int a = servoRel(angle);

        if (a >= 0)
            d.print("+");

        d.print(a);

        d.setCursor(0, 40);
        d.print("FWD:");
        d.print(pwm1);

        d.setCursor(70, 40);
        d.print("REV:");
        d.print(pwm2);
    }
    else
    {

        d.setTextSize(2);
        d.setCursor(0, 22);

        char totalBuf[8];
        snprintf(totalBuf, sizeof(totalBuf), "%lu:%02lu",
                 pbSlotSec / 60,
                 pbSlotSec % 60);

        if (pbPlaying)
        {
            char leftBuf[8];
            snprintf(leftBuf, sizeof(leftBuf), "%lu:%02lu",
                     pbRemainSec / 60,
                     pbRemainSec % 60);

            d.setTextSize(2);

            d.setCursor(0, 22);
            d.print("Left:");
            d.print(leftBuf);

            d.setCursor(0, 45);
            d.print("PB:");
            d.print(rt.selectedPlayback);
            d.print(" ");
            d.print(totalBuf);
        }
        else
        {
            d.setTextSize(2);

            d.setCursor(0, 22);
            d.print("PB:");
            d.print(rt.selectedPlayback);
            d.print(" ");
            d.print(totalBuf);

            d.setTextSize(1);
            d.setCursor(0, 45);
            d.print("CHOOSE PB: +/-");

            d.setCursor(0, 56);
            d.print("START/STOP:PB ON/OFF");
        }
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

    clearAndHome();
    d.setTextSize(1);

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

        d.setCursor(0, row * 12);

        d.print(
            (i == selected)
                ? ">"
                : " ");

        d.print(names[i]);

        if (i == Menu::ITEM_INPUT_MODE)
        {
            d.print(": ");
            d.print(modeName(edit.inputMode));
        }
        else if (i == Menu::ITEM_DMX_ADDRESS)
        {
            d.print(": ");
            d.print(edit.dmxAddress);
        }
        else if (i == Menu::ITEM_PLAYBACK)
        {
            uint8_t recCount = 0;

            for (uint8_t s = 0;
                 s < PLAYBACK_SLOTS;
                 ++s)
            {
                if (playback.isRecorded(s))
                    recCount++;
            }

            d.print(": ");
            d.print(recCount);
            d.print("/");
            d.print(PLAYBACK_SLOTS);
        }
        else if (i == Menu::ITEM_SERVO_SETUP)
        {
            int minRel = servoRel(edit.servoMin);
            int maxRel = servoRel(edit.servoMax);

            d.print(": ");

            if (minRel >= 0)
                d.print("+");

            d.print(minRel);

            d.print("/");

            if (maxRel >= 0)
                d.print("+");

            d.print(maxRel);
        }
        else if (i == Menu::ITEM_MOTOR_PWM)
        {
            d.print(": ");
            d.print(edit.pwmMin);
            d.print("-");
            d.print(edit.pwmMax);
        }
    }

    d.display();
}

void DisplayOled::drawEditInput(const RuntimeSettings &edit)
{
    clearAndHome();
    d.setTextSize(1);

    d.println("EDIT Input");
    d.setCursor(0, 16);
    d.println(modeName(edit.inputMode));

    d.setCursor(0, 54);
    d.print("+/-  START=Back");
    d.display();
}

void DisplayOled::drawEditDmx(const RuntimeSettings &edit)
{
    clearAndHome();
    d.setTextSize(1);

    d.println("EDIT DMX Address");
    d.setCursor(0, 16);
    d.println(edit.dmxAddress);

    d.setCursor(0, 54);
    d.print("+/-  START=Back");
    d.display();
}

void DisplayOled::drawPlaybackRecList(const Menu &menu, const Playback &playback)
{
    clearAndHome();
    d.setTextSize(1);
    d.println("Playback REC");

    int idx = menu.recIndex();
    int maxIdx = PLAYBACK_SLOTS; // sista = BACK
    int start = max(0, idx - 1);
    int end = min(maxIdx, start + 3);
    int row = 0;

    for (int i = start; i <= end; i++)
    {
        d.setCursor(0, 12 + row * 12);
        d.print((i == idx) ? ">" : " ");

        if (i < PLAYBACK_SLOTS)
        {
            d.print("Slot ");
            d.print(i + 1);

            if (playback.isRecorded(i))
            {
                uint32_t sec = playback.slotSeconds(i);

                char buf[8];
                snprintf(buf, sizeof(buf), "%lu:%02lu", sec / 60, sec % 60);

                d.print("  ");
                d.print(buf);
            }
        }
        else
        {
            d.print("BACK");
        }
        row++;
    }

    d.display();
}

void DisplayOled::drawRecording(uint8_t slot, uint32_t recSec)
{
    clearAndHome();
    d.setTextSize(1);

    d.print("RECORDING Slot ");
    d.println(slot + 1);

    d.setCursor(0, 16);
    d.print("Time: ");

    char buf[8];
    snprintf(buf, sizeof(buf), "%lu:%02lu", recSec / 60, recSec % 60);
    d.println(buf);

    d.setCursor(0, 54);
    d.println("STOP to stop");

    d.display();
}

void DisplayOled::drawServoSetup(const RuntimeSettings &edit, uint8_t index)
{
    clearAndHome();
    d.setTextSize(1);

    d.println("SERVO SETUP");

    d.setCursor(0, 16);
    d.print(index == 0 ? ">" : " ");
    d.print(servoRel(edit.servoMin));

    d.setCursor(0, 28);
    d.print(index == 1 ? ">" : " ");
    d.print("+");
    d.print(servoRel(edit.servoMax));

    d.setCursor(0, 40);
    d.print(index == 2 ? ">" : " ");
    d.print("BACK");

    d.setCursor(0, 56);
    d.print("+/- START=Select");

    d.display();
}

void DisplayOled::drawEditServoMin(const RuntimeSettings &edit)
{
    clearAndHome();
    d.setTextSize(1);

    d.println("EDIT SERVO MIN");
    d.setCursor(0, 20);
    d.setTextSize(2);
    d.println(servoRel(edit.servoMin));

    d.setTextSize(1);
    d.setCursor(0, 54);
    d.print("+/-  START=Back");

    d.display();
}

void DisplayOled::drawEditServoMax(const RuntimeSettings &edit)
{
    clearAndHome();
    d.setTextSize(1);

    d.println("EDIT SERVO MAX");
    d.setCursor(0, 20);
    d.setTextSize(2);
    d.print("+");
    d.println(servoRel(edit.servoMax));

    d.setTextSize(1);
    d.setCursor(0, 54);
    d.print("+/-  START=Back");

    d.display();
}

void DisplayOled::drawMotorPwmSetup(
    const RuntimeSettings &edit,
    uint8_t index)
{
    clearAndHome();
    d.setTextSize(1);

    d.println("MOTOR PWM");

    d.setCursor(0, 16);
    d.print(index == 0 ? ">" : " ");
    d.print("MIN: ");
    d.print(edit.pwmMin);

    d.setCursor(0, 28);
    d.print(index == 1 ? ">" : " ");
    d.print("MAX: ");
    d.print(edit.pwmMax);

    d.setCursor(0, 40);
    d.print(index == 2 ? ">" : " ");
    d.print("BACK");

    d.setCursor(0, 56);
    d.print("+/- START=Select");

    d.display();
}

void DisplayOled::drawEditPwmMin(
    const RuntimeSettings &edit)
{
    clearAndHome();

    d.setTextSize(1);
    d.println("EDIT PWM MIN");

    d.setCursor(0, 20);
    d.setTextSize(2);
    d.println(edit.pwmMin);

    d.setTextSize(1);
    d.setCursor(0, 54);
    d.print("+/- START=Back");

    d.display();
}

void DisplayOled::drawEditPwmMax(
    const RuntimeSettings &edit)
{
    clearAndHome();

    d.setTextSize(1);
    d.println("EDIT PWM MAX");

    d.setCursor(0, 20);
    d.setTextSize(2);
    d.println(edit.pwmMax);

    d.setTextSize(1);
    d.setCursor(0, 54);
    d.print("+/- START=Back");

    d.display();
}