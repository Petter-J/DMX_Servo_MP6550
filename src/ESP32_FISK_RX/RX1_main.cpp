#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>
#include <esp_wifi.h>

// --------------------------------------------------
// PINNAR
// --------------------------------------------------

#define SERVO_PIN 18

// --------------------------------------------------
// SERVO
// --------------------------------------------------

#define SERVO_CENTER_DEFAULT 90

// --------------------------------------------------
// FAILSAFE
// --------------------------------------------------

#define RX_TIMEOUT_MS 500

Servo servo;

uint32_t lastRxMs = 0;
bool failsafeActive = false;

// --------------------------------------------------
// ESP-NOW DATA
// Måste matcha TX exakt.
// Fisk använder bara angle.
// --------------------------------------------------

struct ControlData
{
    uint8_t angle;
    uint8_t pwm1;
    uint8_t pwm2;
    uint8_t enable;
};

// --------------------------------------------------
// ESP-NOW RECEIVE
// --------------------------------------------------

void onReceive(const uint8_t *mac, const uint8_t *data, int len)
{
    (void)mac;

    if (len != sizeof(ControlData))
        return;

    ControlData rx;
    memcpy(&rx, data, sizeof(rx));

    uint8_t angle = constrain(rx.angle, 0, 180);

    lastRxMs = millis();
    failsafeActive = false;

    servo.write(angle);
}

// --------------------------------------------------
// SETUP
// --------------------------------------------------

void setup()
{
    servo.setPeriodHertz(50);
    servo.attach(SERVO_PIN, 500, 2500);
    servo.write(SERVO_CENTER_DEFAULT);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Samma kanal som TX
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK)
        return;

    esp_now_register_recv_cb(onReceive);

    lastRxMs = millis();
}

// --------------------------------------------------
// LOOP
// --------------------------------------------------

void loop()
{
    if (!failsafeActive && millis() - lastRxMs > RX_TIMEOUT_MS)
    {
        servo.write(SERVO_CENTER_DEFAULT);
        failsafeActive = true;
    }
}