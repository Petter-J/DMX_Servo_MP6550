#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

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
    void begin();

    void send(
        uint8_t angle,
        uint8_t pwm1,
        uint8_t pwm2,
        bool enable);

private:
    uint8_t receiverMac[6] = {0x3C,0x0F,0x02,0xE4,0xCD,0x58};

    ControlData p{};

    static void onSent(
        const uint8_t *mac,
        esp_now_send_status_t status);
};