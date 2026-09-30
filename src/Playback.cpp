#include "Playback.h"
#include "Config.h"
#include <Preferences.h>

void Playback::begin()
{
}

void Playback::startRecording(uint8_t slotIndex)
{
    if (slotIndex >= PLAYBACK_SLOTS)
        return;

    recSlot = slotIndex;
    slots[recSlot].len = 0;
    slots[recSlot].recorded = false;

    recLastMs = millis();
    recording = true;
}

void Playback::tickRecord(uint8_t servoValue, uint8_t motorValue)
{
    if (!recording)
        return;

    uint32_t now = millis();

    if (now - recLastMs < SAMPLE_MS)
        return;

    recLastMs = now;

    auto &s = slots[recSlot];

    if (s.len < MAX_SAMPLES)
    {
        s.data[s.len].servo = servoValue;
        s.data[s.len].motor = motorValue;
        s.len++;
    }
}

void Playback::stopRecording()
{
    if (!recording)
        return;

    slots[recSlot].recorded = (slots[recSlot].len > 0);
    lastRecSlot = recSlot;
    recording = false;
}

bool Playback::isRecorded(uint8_t slotIndex) const
{
    if (slotIndex >= PLAYBACK_SLOTS)
        return false;

    return slots[slotIndex].recorded;
}

void Playback::startPlaying(uint8_t slotIndex)
{
    if (slotIndex >= PLAYBACK_SLOTS)
        return;

    if (!slots[slotIndex].recorded || slots[slotIndex].len == 0)
        return;

    playSlot = slotIndex;
    playPos = 0;
    playLastMs = millis();
    playing = true;
}

void Playback::stopPlaying()
{
    playing = false;
}

bool Playback::isPlaying() const
{
    return playing;
}

uint8_t Playback::currentPlayingSlot() const
{
    return playSlot;
}

void Playback::tickPlaybackValues(uint8_t &servoValue, uint8_t &motorValue)
{
    servoValue = 0;
    motorValue = 127;

    if (playSlot >= PLAYBACK_SLOTS)
        return;

    auto &s = slots[playSlot];

    if (s.len == 0)
    {
        playing = false;
        return;
    }

    if (playPos >= s.len)
        playPos = s.len - 1;

    servoValue = s.data[playPos].servo;
    motorValue = s.data[playPos].motor;

    if (!playing)
        return;

    uint32_t now = millis();

    if (now - playLastMs >= SAMPLE_MS)
    {
        playLastMs = now;

        if (playPos + 1 < s.len)
            playPos++;
        else
            playing = false;
    }
}

void Playback::eraseRecording(uint8_t slotIndex)
{
    if (slotIndex >= PLAYBACK_SLOTS)
        return;

    slots[slotIndex].recorded = false;
    slots[slotIndex].len = 0;

    if (playing && playSlot == slotIndex)
        playing = false;

    if (lastRecSlot == slotIndex)
        lastRecSlot = 0;
}

bool Playback::saveAllToFlash()
{
    Preferences p;

    if (!p.begin("playback", false))
        return false;

    p.putUChar("ver", 2);

    for (uint8_t i = 0; i < PLAYBACK_SLOTS; i++)
    {
        char kRec[8];
        char kLen[8];
        char kDat[8];

        snprintf(kRec, sizeof(kRec), "r%u", i);
        snprintf(kLen, sizeof(kLen), "l%u", i);
        snprintf(kDat, sizeof(kDat), "d%u", i);

        p.putBool(kRec, slots[i].recorded);

        uint16_t len = slots[i].recorded ? slots[i].len : 0;

        if (len > MAX_SAMPLES)
            len = MAX_SAMPLES;

        p.putUShort(kLen, len);

        if (len > 0)
        {
            size_t bytes = len * sizeof(PlaybackSample);
            p.putBytes(kDat, slots[i].data, bytes);
        }
        else
        {
            p.remove(kDat);
        }
    }

    p.end();
    return true;
}

bool Playback::loadAllFromFlash()
{
    Preferences p;

    if (!p.begin("playback", true))
        return false;

    uint8_t ver = p.getUChar("ver", 0);

    if (ver != 2)
    {
        p.end();
        return false;
    }

    for (uint8_t i = 0; i < PLAYBACK_SLOTS; i++)
    {
        char kRec[8];
        char kLen[8];
        char kDat[8];

        snprintf(kRec, sizeof(kRec), "r%u", i);
        snprintf(kLen, sizeof(kLen), "l%u", i);
        snprintf(kDat, sizeof(kDat), "d%u", i);

        bool rec = p.getBool(kRec, false);
        uint16_t len = p.getUShort(kLen, 0);

        if (len > MAX_SAMPLES)
            len = MAX_SAMPLES;

        slots[i].recorded = rec && (len > 0);
        slots[i].len = slots[i].recorded ? len : 0;

        if (slots[i].recorded)
        {
            size_t expected = slots[i].len * sizeof(PlaybackSample);
            size_t got = p.getBytes(kDat, slots[i].data, expected);

            if (got != expected)
            {
                slots[i].recorded = false;
                slots[i].len = 0;
            }
        }
    }

    p.end();
    return true;
}

uint32_t Playback::recordingSeconds() const
{
    if (!recording || recSlot >= PLAYBACK_SLOTS)
        return 0;

    return ((uint32_t)slots[recSlot].len * SAMPLE_MS) / 1000;
}

uint32_t Playback::playbackSecondsRemaining() const
{
    if (!playing || playSlot >= PLAYBACK_SLOTS)
        return 0;

    const auto &s = slots[playSlot];

    if (playPos >= s.len)
        return 0;

    return ((uint32_t)(s.len - playPos) * SAMPLE_MS) / 1000;
}

uint32_t Playback::slotSeconds(uint8_t slotIndex) const
{
    if (slotIndex >= PLAYBACK_SLOTS)
        return 0;

    return ((uint32_t)slots[slotIndex].len * SAMPLE_MS) / 1000;
}