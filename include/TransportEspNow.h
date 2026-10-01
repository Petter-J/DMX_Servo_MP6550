#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "Receivers.h"

struct ControlData
{
    uint8_t angle;
    uint8_t pwm1;
    uint8_t pwm2;
    uint8_t enable;
};

class TransportEspNow
{
public:
    void begin(uint8_t receiverIndex);
    void setReceiver(uint8_t receiverIndex);

    void send(
        uint8_t angle,
        uint8_t pwm1,
        uint8_t pwm2,
        bool enable);

    bool linkOk() const;

private:
    uint8_t receiverMac[6]{};

    ControlData p{};

    static volatile bool lastSendSuccess;
    static volatile uint32_t lastSendMs;

    static constexpr uint32_t LINK_TIMEOUT_MS = 1500;

    static void onSent(
        const uint8_t *mac,
        esp_now_send_status_t status);
};