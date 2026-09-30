#pragma once

#include <Arduino.h>
#include "Config.h"

class Playback
{
public:
    void begin();

    void startRecording(uint8_t slotIndex); // 0..PLAYBACK_SLOTS-1
    void stopRecording();

    void tickRecord(
        uint8_t servoValue,
        uint8_t motorValue);

    void eraseRecording(uint8_t slotIndex);

    bool isRecorded(uint8_t slotIndex) const;

    uint8_t lastRecordedSlot() const
    {
        return lastRecSlot;
    }

    uint8_t currentRecordingSlot() const
    {
        return recSlot;
    }

    void tickPlaybackValues(
        uint8_t &servoValue,
        uint8_t &motorValue);

    void startPlaying(uint8_t slotIndex);
    void stopPlaying();

    bool isPlaying() const;

    uint8_t currentPlayingSlot() const;

    bool saveAllToFlash();
    bool loadAllFromFlash();

    uint32_t recordingSeconds() const;
    uint32_t playbackSecondsRemaining() const;
    uint32_t slotSeconds(uint8_t slotIndex) const;

private:
    static constexpr uint16_t MAX_SAMPLES = 6000;
    static constexpr uint16_t SAMPLE_MS = 50; // 20 Hz = var 50:e ms

    struct PlaybackSample
    {
        uint8_t servo = 0;
        uint8_t motor = 127;
    };

    struct SlotMeta
    {
        bool recorded = false;
        uint16_t len = 0;
        PlaybackSample data[MAX_SAMPLES]{};
    };

    SlotMeta slots[PLAYBACK_SLOTS];

    bool recording = false;
    uint8_t recSlot = 0;
    uint8_t lastRecSlot = 0;

    bool playing = false;
    uint8_t playSlot = 0;

    uint32_t recLastMs = 0;
    uint32_t playLastMs = 0;

    uint16_t playPos = 0;
};