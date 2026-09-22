#include "App.h"
#include "Config.h"

// --------------------------------------------------
// Hjälpfunktioner
// --------------------------------------------------

static uint8_t valueToServoAngle(
    uint8_t v,
    uint8_t servoMin,
    uint8_t servoMax)
{
    const uint8_t servoCenter = 90;

    int angle;

    if (v <= 126)
    {
        angle = map(
            v,
            0,
            126,
            servoMin,
            servoCenter);
    }
    else
    {
        angle = map(
            v,
            127,
            255,
            servoCenter,
            servoMax);
    }

    return constrain(angle, 0, 180);
}

static uint8_t scaleMotorPwm(
    uint8_t value,
    uint8_t pwmMin,
    uint8_t pwmMax)
{
    // 0 betyder alltid helt av
    if (value == 0)
        return 0;

    // 1..255 skalas till pwmMin..pwmMax
    return static_cast<uint8_t>(
        map(
            value,
            1,
            255,
            pwmMin,
            pwmMax));
}

static void motorFromSlider(
    uint8_t value,
    uint8_t pwmMin,
    uint8_t pwmMax,
    uint8_t &pwmForward,
    uint8_t &pwmReverse,
    bool &enable)
{
    pwmForward = 0;
    pwmReverse = 0;
    enable = false;

    // BACK
    if (value <= 116)
    {
        pwmReverse = static_cast<uint8_t>(
            map(
                value,
                116,
                0,
                pwmMin,
                pwmMax));

        enable = true;
    }

    // STOPP / SLEEP
    else if (value <= 137)
    {
        pwmForward = 0;
        pwmReverse = 0;
        enable = false;
    }

    // FRAM
    else
    {
        pwmForward = static_cast<uint8_t>(
            map(
                value,
                138,
                255,
                pwmMin,
                pwmMax));

        enable = true;
    }
}

// --------------------------------------------------
// BEGIN
// --------------------------------------------------

void App::begin()
{
    Serial.begin(115200);

    buttons.begin();
    settingsStore.begin();

    runtime = settingsStore.load();
    edit = runtime;

    menu.begin();
    transport.begin();
    inputs.begin();
    ui.begin();
    playback.begin();
    playback.loadAllFromFlash();

    uint8_t shownPb =
        playback.isPlaying()
            ? (playback.currentPlayingSlot() + 1)
            : 0;

    uint32_t slotSec =
        playback.slotSeconds(
            runtime.selectedPlayback - 1);

    ui.drawRun(
        runtime,
        lastValue,
        lastAngle,
        lastPwm1,
        lastPwm2,
        playback.isPlaying(),
        shownPb,
        playback.playbackSecondsRemaining(),
        slotSec);
}

// --------------------------------------------------
// TICK
// --------------------------------------------------

void App::tick()
{
    buttons.update();

    switch (state)
    {
    case RUN:
        handleRun();
        break;

    case MENU_MAIN:
        handleMenuMain();
        break;

    case MENU_EDIT_INPUT:
        handleEditInput();
        break;

    case MENU_EDIT_DMX:
        handleEditDmx();
        break;

    case MENU_PLAYBACK_REC_LIST:
        handlePlaybackRecList();
        break;

    case MENU_PLAYBACK_RECORDING:
        handlePlaybackRecording();
        break;

    case MENU_SERVO_SETUP:
        handleServoSetup();
        break;

    case MENU_EDIT_SERVO_MIN:
        handleEditServoMin();
        break;

    case MENU_EDIT_SERVO_MAX:
        handleEditServoMax();
        break;

    case MENU_MOTOR_PWM_SETUP:
        handleMotorPwmSetup();
        break;

    case MENU_EDIT_PWM_MIN:
        handleEditPwmMin();
        break;

    case MENU_EDIT_PWM_MAX:
        handleEditPwmMax();
        break;
    }

    buttons.clearEvents();
}

// --------------------------------------------------
// RUN
// --------------------------------------------------

void App::handleRun()
{
    if (requireReleaseAfterMenu)
    {
        if (!buttons.startHeld)
            requireReleaseAfterMenu = false;
    }

    if (runtime.inputMode == InputMode::PLAYBACK)
    {
        if (!requireReleaseAfterMenu &&
            buttons.startShort)
        {
            uint8_t idx =
                (runtime.selectedPlayback > 0)
                    ? (runtime.selectedPlayback - 1)
                    : 0;

            playback.startPlaying(idx);
        }

        if (buttons.stopShort)
            playback.stopPlaying();

        if (buttons.plusShort)
        {
            uint8_t cur =
                (runtime.selectedPlayback > 0)
                    ? (runtime.selectedPlayback - 1)
                    : 0;

            for (uint8_t step = 1;
                 step <= PLAYBACK_SLOTS;
                 ++step)
            {
                uint8_t cand =
                    (cur + step) %
                    PLAYBACK_SLOTS;

                if (playback.isRecorded(cand))
                {
                    runtime.selectedPlayback =
                        cand + 1;

                    break;
                }
            }
        }

        if (buttons.minusShort)
        {
            uint8_t cur =
                (runtime.selectedPlayback > 0)
                    ? (runtime.selectedPlayback - 1)
                    : 0;

            for (uint8_t step = 1;
                 step <= PLAYBACK_SLOTS;
                 ++step)
            {
                uint8_t cand =
                    (cur +
                     PLAYBACK_SLOTS -
                     step) %
                    PLAYBACK_SLOTS;

                if (playback.isRecorded(cand))
                {
                    runtime.selectedPlayback =
                        cand + 1;

                    break;
                }
            }
        }
    }

    if (buttons.stopLong1s)
    {
        edit = runtime;
        menuInputArmed = false;
        state = MENU_MAIN;
        return;
    }

    sendCurrentValue();

    uint8_t shownPb =
        playback.isPlaying()
            ? (playback.currentPlayingSlot() + 1)
            : 0;

    uint32_t slotSec =
        playback.slotSeconds(
            runtime.selectedPlayback - 1);

    ui.drawRun(
        runtime,
        lastValue,
        lastAngle,
        lastPwm1,
        lastPwm2,
        playback.isPlaying(),
        shownPb,
        playback.playbackSecondsRemaining(),
        slotSec);
}

// --------------------------------------------------
// HUVUDMENY
// --------------------------------------------------

void App::handleMenuMain()
{
    if (!menuInputArmed)
    {
        if (!buttons.plusShort &&
            !buttons.minusShort &&
            !buttons.startShort &&
            !buttons.startLong1s &&
            !buttons.startLong2s &&
            !buttons.stopShort)
        {
            menuInputArmed = true;
        }

        ui.drawMainMenu(
            menu,
            edit,
            playback);

        return;
    }

    menu.updateMainNavigation(
        buttons.plusShort,
        buttons.minusShort);

    if (buttons.startShort)
    {
        switch (menu.mainIndex())
        {
        case Menu::ITEM_INPUT_MODE:
            state = MENU_EDIT_INPUT;
            break;

        case Menu::ITEM_DMX_ADDRESS:
            state = MENU_EDIT_DMX;
            break;

        case Menu::ITEM_PLAYBACK:
            menu.enterPlaybackRecList();
            state = MENU_PLAYBACK_REC_LIST;
            break;

        case Menu::ITEM_SERVO_SETUP:
            servoSetupIndex = 0;
            state = MENU_SERVO_SETUP;
            break;

        case Menu::ITEM_MOTOR_PWM:
            motorPwmSetupIndex = 0;
            state = MENU_MOTOR_PWM_SETUP;
            break;

        default:
            break;
        }
    }

    if (buttons.startLong1s)
    {
        if (menu.mainIndex() ==
            Menu::ITEM_EXIT)
        {
            runtime = edit;

            playback.stopPlaying();

            requireReleaseAfterMenu = true;
            menuInputArmed = false;
            state = RUN;
        }
        else if (
            menu.mainIndex() ==
            Menu::ITEM_SAVE)
        {
            runtime = edit;

            playback.stopPlaying();

            settingsStore.save(runtime);
            playback.saveAllToFlash();

            requireReleaseAfterMenu = true;
            menuInputArmed = false;
            state = RUN;
        }
    }

    ui.drawMainMenu(
        menu,
        edit,
        playback);
}

// --------------------------------------------------
// INPUT MODE
// --------------------------------------------------

void App::handleEditInput()
{
    if (buttons.plusShort)
        edit.inputMode =
            nextInputMode(edit.inputMode);

    if (buttons.minusShort)
        edit.inputMode =
            prevInputMode(edit.inputMode);

    if (buttons.startShort)
        state = MENU_MAIN;

    ui.drawEditInput(edit);
}

// --------------------------------------------------
// DMX ADDRESS
// --------------------------------------------------

void App::handleEditDmx()
{
    static uint32_t nextRptPlus = 0;
    static uint32_t nextRptMinus = 0;

    static uint16_t plusRepeats = 0;
    static uint16_t minusRepeats = 0;

    const uint32_t FIRST_DELAY_MS = 350;
    const uint32_t REPEAT_MS = 70;
    const uint16_t DMX_MAX_START = 510;

    auto stepFor =
        [](uint16_t repeats) -> uint16_t
    {
        return (repeats < 10) ? 1 : 10;
    };

    auto wrapAdd =
        [DMX_MAX_START](
            uint16_t v,
            uint16_t step) -> uint16_t
    {
        uint16_t x = v - 1;

        x =
            (x + step) %
            DMX_MAX_START;

        return x + 1;
    };

    auto wrapSub =
        [DMX_MAX_START](
            uint16_t v,
            uint16_t step) -> uint16_t
    {
        uint16_t x = v - 1;

        step %= DMX_MAX_START;

        x =
            (x +
             DMX_MAX_START -
             step) %
            DMX_MAX_START;

        return x + 1;
    };

    uint32_t now = millis();

    if (buttons.plusShort)
        edit.dmxAddress =
            wrapAdd(edit.dmxAddress, 1);

    if (buttons.minusShort)
        edit.dmxAddress =
            wrapSub(edit.dmxAddress, 1);

    if (buttons.plusHeld)
    {
        if (nextRptPlus == 0)
        {
            nextRptPlus =
                now + FIRST_DELAY_MS;

            plusRepeats = 0;
        }

        if (now >= nextRptPlus)
        {
            uint16_t step =
                stepFor(plusRepeats++);

            edit.dmxAddress =
                wrapAdd(
                    edit.dmxAddress,
                    step);

            nextRptPlus =
                now + REPEAT_MS;
        }
    }
    else
    {
        nextRptPlus = 0;
        plusRepeats = 0;
    }

    if (buttons.minusHeld)
    {
        if (nextRptMinus == 0)
        {
            nextRptMinus =
                now + FIRST_DELAY_MS;

            minusRepeats = 0;
        }

        if (now >= nextRptMinus)
        {
            uint16_t step =
                stepFor(minusRepeats++);

            edit.dmxAddress =
                wrapSub(
                    edit.dmxAddress,
                    step);

            nextRptMinus =
                now + REPEAT_MS;
        }
    }
    else
    {
        nextRptMinus = 0;
        minusRepeats = 0;
    }

    if (buttons.startShort)
        state = MENU_MAIN;

    ui.drawEditDmx(edit);
}

// --------------------------------------------------
// PLAYBACK LIST
// --------------------------------------------------

void App::handlePlaybackRecList()
{
    menu.updatePlaybackRecNavigation(
        buttons.plusShort,
        buttons.minusShort);

    if (buttons.startShort &&
        menu.playbackRecIsBack())
    {
        state = MENU_MAIN;
        return;
    }

    if (buttons.startLong1s &&
        !menu.playbackRecIsBack())
    {
        playback.startRecording(
            menu.playbackRecSlotIndex());

        state =
            MENU_PLAYBACK_RECORDING;

        return;
    }

    if (buttons.stopLong1s &&
        !menu.playbackRecIsBack())
    {
        uint8_t slot =
            menu.playbackRecSlotIndex();

        playback.eraseRecording(slot);

        if (runtime.selectedPlayback ==
            slot + 1)
        {
            runtime.selectedPlayback = 1;
        }

        if (edit.selectedPlayback ==
            slot + 1)
        {
            edit.selectedPlayback = 1;
        }
    }

    ui.drawPlaybackRecList(
        menu,
        playback);
}

// --------------------------------------------------
// PLAYBACK RECORDING
// --------------------------------------------------

void App::handlePlaybackRecording()
{
    uint8_t v =
        inputs.readSlider();

    playback.tickRecord(v);

    if (buttons.stopShort)
    {
        playback.stopRecording();

        edit.selectedPlayback =
            playback.lastRecordedSlot() + 1;

        state =
            MENU_PLAYBACK_REC_LIST;

        return;
    }

    lastValue = v;

    lastAngle =
        valueToServoAngle(
            v,
            edit.servoMin,
            edit.servoMax);

    // Motor alltid disabled under inspelning
    lastPwm1 = 0;
    lastPwm2 = 0;

    transport.send(
        lastAngle,
        0,
        0,
        false);

    ui.drawRecording(
        playback.currentRecordingSlot(),
        playback.recordingSeconds());
}

// --------------------------------------------------
// SKICKA AKTUELLT VÄRDE
// --------------------------------------------------

void App::sendCurrentValue()
{
    uint8_t servoValue = 0;
    uint8_t pwm1 = 0;
    uint8_t pwm2 = 0;

    bool motorEnable = false;

    // ==================================================
    // DMX
    //
    // N   = Servo
    // N+1 = Fram
    // N+2 = Back
    // ==================================================

    if (runtime.inputMode == InputMode::DMX)
    {
        uint8_t dmxForward = 0;
        uint8_t dmxReverse = 0;

        inputs.readDmx3(
            runtime.dmxAddress,
            servoValue,
            dmxForward,
            dmxReverse);

        // Båda 0 = driver SLEEP
        if (dmxForward == 0 &&
            dmxReverse == 0)
        {
            pwm1 = 0;
            pwm2 = 0;
            motorEnable = false;
        }

        // Båda aktiva samtidigt = SLEEP
        else if (
            dmxForward > 0 &&
            dmxReverse > 0)
        {
            pwm1 = 0;
            pwm2 = 0;
            motorEnable = false;
        }

        // FRAM
        else if (dmxForward > 0)
        {
            pwm1 =
                scaleMotorPwm(
                    dmxForward,
                    runtime.pwmMin,
                    runtime.pwmMax);

            pwm2 = 0;
            motorEnable = true;
        }

        // BACK
        else
        {
            pwm1 = 0;

            pwm2 =
                scaleMotorPwm(
                    dmxReverse,
                    runtime.pwmMin,
                    runtime.pwmMax);

            motorEnable = true;
        }
    }

    // ==================================================
    // SLIDER
    //
    // Slider 1 = Servo
    //
    // Motor-slider:
    // 0..125   = BACK
    // 126..128 = SLEEP
    // 129..255 = FRAM
    // ==================================================

    else if (
        runtime.inputMode ==
        InputMode::SLIDER)
    {
        servoValue =
            inputs.readSlider();

        uint8_t motorValue =
            inputs.readPwmSlider();

        motorFromSlider(
            motorValue,
            runtime.pwmMin,
            runtime.pwmMax,
            pwm1,
            pwm2,
            motorEnable);
    }

    // ==================================================
    // PLAYBACK
    //
    // Playback styr bara servo.
    // Motor alltid disabled.
    // ==================================================

    else
    {
        servoValue =
            playback.isPlaying()
                ? playback.tickPlaybackValue()
                : runtime.playbackStopValue;

        pwm1 = 0;
        pwm2 = 0;
        motorEnable = false;
    }

    lastValue = servoValue;

    lastAngle =
        valueToServoAngle(
            servoValue,
            runtime.servoMin,
            runtime.servoMax);

    lastPwm1 = pwm1;
    lastPwm2 = pwm2;

    transport.send(
        lastAngle,
        pwm1,
        pwm2,
        motorEnable);
}

// --------------------------------------------------
// SERVO SETUP
// --------------------------------------------------

void App::handleServoSetup()
{
    if (buttons.plusShort)
    {
        servoSetupIndex =
            (servoSetupIndex + 1) % 3;
    }

    if (buttons.minusShort)
    {
        servoSetupIndex =
            (servoSetupIndex + 2) % 3;
    }

    if (buttons.startShort)
    {
        if (servoSetupIndex == 0)
        {
            state =
                MENU_EDIT_SERVO_MIN;
        }
        else if (servoSetupIndex == 1)
        {
            state =
                MENU_EDIT_SERVO_MAX;
        }
        else
        {
            state = MENU_MAIN;
        }

        return;
    }

    if (buttons.stopShort)
    {
        state = MENU_MAIN;
        return;
    }

    ui.drawServoSetup(
        edit,
        servoSetupIndex);
}

// --------------------------------------------------
// SERVO MIN
// --------------------------------------------------

void App::handleEditServoMin()
{
    static uint32_t nextRptPlus = 0;
    static uint32_t nextRptMinus = 0;

    const uint32_t FIRST_DELAY_MS = 350;
    const uint32_t REPEAT_MS = 70;

    uint32_t now = millis();

    if (buttons.plusShort &&
        edit.servoMin < 89)
    {
        edit.servoMin++;
    }

    if (buttons.minusShort &&
        edit.servoMin > 0)
    {
        edit.servoMin--;
    }

    if (buttons.plusHeld)
    {
        if (nextRptPlus == 0)
            nextRptPlus =
                now + FIRST_DELAY_MS;

        if (now >= nextRptPlus)
        {
            if (edit.servoMin < 89)
                edit.servoMin++;

            nextRptPlus =
                now + REPEAT_MS;
        }
    }
    else
    {
        nextRptPlus = 0;
    }

    if (buttons.minusHeld)
    {
        if (nextRptMinus == 0)
            nextRptMinus =
                now + FIRST_DELAY_MS;

        if (now >= nextRptMinus)
        {
            if (edit.servoMin > 0)
                edit.servoMin--;

            nextRptMinus =
                now + REPEAT_MS;
        }
    }
    else
    {
        nextRptMinus = 0;
    }

    if (edit.servoMin >= 90)
        edit.servoMin = 89;

    lastAngle = edit.servoMin;

    // Motor disabled under servo-test
    transport.send(
        lastAngle,
        0,
        0,
        false);

    if (buttons.startShort ||
        buttons.stopShort)
    {
        nextRptPlus = 0;
        nextRptMinus = 0;

        state =
            MENU_SERVO_SETUP;

        return;
    }

    ui.drawEditServoMin(edit);
}

// --------------------------------------------------
// SERVO MAX
// --------------------------------------------------

void App::handleEditServoMax()
{
    static uint32_t nextRptPlus = 0;
    static uint32_t nextRptMinus = 0;

    const uint32_t FIRST_DELAY_MS = 350;
    const uint32_t REPEAT_MS = 70;

    uint32_t now = millis();

    if (buttons.plusShort &&
        edit.servoMax < 180)
    {
        edit.servoMax++;
    }

    if (buttons.minusShort &&
        edit.servoMax > 91)
    {
        edit.servoMax--;
    }

    if (buttons.plusHeld)
    {
        if (nextRptPlus == 0)
            nextRptPlus =
                now + FIRST_DELAY_MS;

        if (now >= nextRptPlus)
        {
            if (edit.servoMax < 180)
                edit.servoMax++;

            nextRptPlus =
                now + REPEAT_MS;
        }
    }
    else
    {
        nextRptPlus = 0;
    }

    if (buttons.minusHeld)
    {
        if (nextRptMinus == 0)
            nextRptMinus =
                now + FIRST_DELAY_MS;

        if (now >= nextRptMinus)
        {
            if (edit.servoMax > 91)
                edit.servoMax--;

            nextRptMinus =
                now + REPEAT_MS;
        }
    }
    else
    {
        nextRptMinus = 0;
    }

    if (edit.servoMax <= 90)
        edit.servoMax = 91;

    lastAngle = edit.servoMax;

    transport.send(
        lastAngle,
        0,
        0,
        false);

    if (buttons.startShort ||
        buttons.stopShort)
    {
        nextRptPlus = 0;
        nextRptMinus = 0;

        state =
            MENU_SERVO_SETUP;

        return;
    }

    ui.drawEditServoMax(edit);
}

// --------------------------------------------------
// MOTOR PWM SETUP
// --------------------------------------------------

void App::handleMotorPwmSetup()
{
    if (buttons.plusShort)
    {
        motorPwmSetupIndex =
            (motorPwmSetupIndex + 1) % 3;
    }

    if (buttons.minusShort)
    {
        motorPwmSetupIndex =
            (motorPwmSetupIndex + 2) % 3;
    }

    if (buttons.startShort)
    {
        if (motorPwmSetupIndex == 0)
        {
            state =
                MENU_EDIT_PWM_MIN;
        }
        else if (motorPwmSetupIndex == 1)
        {
            state =
                MENU_EDIT_PWM_MAX;
        }
        else
        {
            state = MENU_MAIN;
        }

        return;
    }

    if (buttons.stopShort)
    {
        state = MENU_MAIN;
        return;
    }

    ui.drawMotorPwmSetup(
        edit,
        motorPwmSetupIndex);
}

// --------------------------------------------------
// PWM MIN
// --------------------------------------------------

void App::handleEditPwmMin()
{
    static uint32_t nextRptPlus = 0;
    static uint32_t nextRptMinus = 0;

    const uint32_t FIRST_DELAY_MS = 350;
    const uint32_t REPEAT_MS = 70;

    uint32_t now = millis();

    if (buttons.plusShort &&
        edit.pwmMin < edit.pwmMax)
    {
        edit.pwmMin++;
    }

    if (buttons.minusShort &&
        edit.pwmMin > 0)
    {
        edit.pwmMin--;
    }

    if (buttons.plusHeld)
    {
        if (nextRptPlus == 0)
            nextRptPlus =
                now + FIRST_DELAY_MS;

        if (now >= nextRptPlus)
        {
            if (edit.pwmMin < edit.pwmMax)
                edit.pwmMin++;

            nextRptPlus =
                now + REPEAT_MS;
        }
    }
    else
    {
        nextRptPlus = 0;
    }

    if (buttons.minusHeld)
    {
        if (nextRptMinus == 0)
            nextRptMinus =
                now + FIRST_DELAY_MS;

        if (now >= nextRptMinus)
        {
            if (edit.pwmMin > 0)
                edit.pwmMin--;

            nextRptMinus =
                now + REPEAT_MS;
        }
    }
    else
    {
        nextRptMinus = 0;
    }

    if (buttons.startShort ||
        buttons.stopShort)
    {
        nextRptPlus = 0;
        nextRptMinus = 0;

        state =
            MENU_MOTOR_PWM_SETUP;

        return;
    }

    ui.drawEditPwmMin(edit);
}

// --------------------------------------------------
// PWM MAX
// --------------------------------------------------

void App::handleEditPwmMax()
{
    static uint32_t nextRptPlus = 0;
    static uint32_t nextRptMinus = 0;

    const uint32_t FIRST_DELAY_MS = 350;
    const uint32_t REPEAT_MS = 70;

    uint32_t now = millis();

    if (buttons.plusShort &&
        edit.pwmMax < 255)
    {
        edit.pwmMax++;
    }

    if (buttons.minusShort &&
        edit.pwmMax > edit.pwmMin)
    {
        edit.pwmMax--;
    }

    if (buttons.plusHeld)
    {
        if (nextRptPlus == 0)
            nextRptPlus =
                now + FIRST_DELAY_MS;

        if (now >= nextRptPlus)
        {
            if (edit.pwmMax < 255)
                edit.pwmMax++;

            nextRptPlus =
                now + REPEAT_MS;
        }
    }
    else
    {
        nextRptPlus = 0;
    }

    if (buttons.minusHeld)
    {
        if (nextRptMinus == 0)
            nextRptMinus =
                now + FIRST_DELAY_MS;

        if (now >= nextRptMinus)
        {
            if (edit.pwmMax > edit.pwmMin)
                edit.pwmMax--;

            nextRptMinus =
                now + REPEAT_MS;
        }
    }
    else
    {
        nextRptMinus = 0;
    }

    if (buttons.startShort ||
        buttons.stopShort)
    {
        nextRptPlus = 0;
        nextRptMinus = 0;

        state =
            MENU_MOTOR_PWM_SETUP;

        return;
    }

    ui.drawEditPwmMax(edit);
}