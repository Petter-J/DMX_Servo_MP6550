#include "Settings.h"
#include "Playback.h"

void SettingsStore::begin()
{
}

void SettingsStore::clamp(RuntimeSettings &s)
{
    // DMX
    if (s.dmxAddress < 1)
        s.dmxAddress = 1;

    // 3 DMX-kanaler används:
    // N   = servo
    // N+1 = fram
    // N+2 = back
    if (s.dmxAddress > 510)
        s.dmxAddress = 510;

    // Input mode
    if (static_cast<uint8_t>(s.inputMode) > 2)
        s.inputMode = InputMode::DMX;

    // Playback
    if (s.selectedPlayback < 1)
        s.selectedPlayback = 1;

    if (s.selectedPlayback > PLAYBACK_SLOTS)
        s.selectedPlayback = PLAYBACK_SLOTS;

    // Servo
    if (s.servoMin > 89)
        s.servoMin = 89;

    if (s.servoMax < 91)
        s.servoMax = 91;

    if (s.servoMax > 180)
        s.servoMax = 180;

    // Motor PWM
    if (s.pwmMin > s.pwmMax)
        s.pwmMin = s.pwmMax;
}

RuntimeSettings SettingsStore::load()
{
    RuntimeSettings s;

    prefs.begin("cfg", true);

    s.dmxAddress = prefs.getUShort("dmxAddr", 1);
    s.selectedPlayback = prefs.getUChar("pbSel", 1);
    s.inputMode = static_cast<InputMode>(prefs.getUChar("inMode", 0));
    s.playbackStopValue = prefs.getUChar("pbStop", PLAYBACK_STOP_VALUE_DEFAULT);
    s.servoMin = prefs.getUChar("servoMin", 10);
    s.servoMax = prefs.getUChar("servoMax", 170);
    s.pwmMin = prefs.getUChar("pwmMin", 0);
    s.pwmMax = prefs.getUChar("pwmMax", 255);

    prefs.end();

    clamp(s);

    return s;
}

void SettingsStore::save(const RuntimeSettings &sIn)
{
    RuntimeSettings s = sIn;

    clamp(s);

    prefs.begin("cfg", false);

    prefs.putUShort("dmxAddr", s.dmxAddress);
    prefs.putUChar("pbSel", s.selectedPlayback);
    prefs.putUChar("inMode", static_cast<uint8_t>(s.inputMode));
    prefs.putUChar("pbStop", s.playbackStopValue);
    prefs.putUChar("servoMin", s.servoMin);
    prefs.putUChar("servoMax", s.servoMax);
    prefs.putUChar("pwmMin", s.pwmMin);
    prefs.putUChar("pwmMax", s.pwmMax);

    prefs.end();
}