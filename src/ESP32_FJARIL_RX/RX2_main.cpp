#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>
#include <esp_wifi.h>

// --------------------------------------------------
// PINNAR
// --------------------------------------------------

#define PWM1_PIN 4
#define PWM2_PIN 5
#define SERVO_PIN 6
#define SLEEP_PIN 7

// --------------------------------------------------
// SERVO
// --------------------------------------------------

#define SERVO_CENTER_DEFAULT 90

// --------------------------------------------------
// MOTOR PWM
// --------------------------------------------------

#define MOTOR_KICK_PWM 220
#define MOTOR_KICK_TIME_MS 70

#define PWM_FREQ 10000
#define PWM_RESOLUTION 8

#define PWM1_CHANNEL 6
#define PWM2_CHANNEL 7

// --------------------------------------------------
// FAILSAFE
// --------------------------------------------------

#define RX_TIMEOUT_MS 500

Servo servo;

uint32_t lastRxMs = 0;
bool failsafeActive = false;

bool motorWasEnabled = false;
bool kickActive = false;

bool lastDirectionForward = false;
bool lastDirectionReverse = false;

uint32_t kickStartMs = 0;

uint8_t requestedPwm1 = 0;
uint8_t requestedPwm2 = 0;

// --------------------------------------------------
// ESP-NOW DATA
// Måste vara exakt samma struct som på TX.
// --------------------------------------------------

struct ControlData
{
    uint8_t angle;
    uint8_t pwm1;
    uint8_t pwm2;
    uint8_t enable;
};

// --------------------------------------------------
// MOTOR OFF
// --------------------------------------------------

void motorDisable()
{
    ledcWrite(PWM1_CHANNEL, 0);
    ledcWrite(PWM2_CHANNEL, 0);

    digitalWrite(SLEEP_PIN, LOW);

    motorWasEnabled = false;
    kickActive = false;

    lastDirectionForward = false;
    lastDirectionReverse = false;

    requestedPwm1 = 0;
    requestedPwm2 = 0;
}

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

    // Servo
    servo.write(angle);

    // TX säger disable
    if (rx.enable == 0)
    {
        motorDisable();
        return;
    }

    // Fram och back får aldrig vara aktiva samtidigt
    if (rx.pwm1 > 0 && rx.pwm2 > 0)
    {
        motorDisable();
        return;
    }

    // Enable satt men båda PWM är 0
    if (rx.pwm1 == 0 && rx.pwm2 == 0)
    {
        motorDisable();
        return;
    }

    requestedPwm1 = rx.pwm1;
    requestedPwm2 = rx.pwm2;

    bool newForward = (requestedPwm1 > 0);
    bool newReverse = (requestedPwm2 > 0);

    bool directionChanged =
        motorWasEnabled &&
        ((lastDirectionForward && newReverse) ||
         (lastDirectionReverse && newForward));

    if (directionChanged)
    {
        motorDisable();

        // motorDisable() nollställer dessa
        requestedPwm1 = rx.pwm1;
        requestedPwm2 = rx.pwm2;
    }

    digitalWrite(SLEEP_PIN, HIGH);

    // Startkick när motorn går från OFF till ON
    if (!motorWasEnabled)
    {
        motorWasEnabled = true;
        kickActive = true;
        kickStartMs = millis();

        lastDirectionForward = (requestedPwm1 > 0);
        lastDirectionReverse = (requestedPwm2 > 0);

        if (requestedPwm1 > 0)
        {
            ledcWrite(PWM1_CHANNEL, MOTOR_KICK_PWM);
            ledcWrite(PWM2_CHANNEL, 0);
        }
        else
        {
            ledcWrite(PWM1_CHANNEL, 0);
            ledcWrite(PWM2_CHANNEL, MOTOR_KICK_PWM);
        }
    }
    else if (!kickActive)
    {
        ledcWrite(PWM1_CHANNEL, requestedPwm1);
        ledcWrite(PWM2_CHANNEL, requestedPwm2);
    }
}

// --------------------------------------------------
// SETUP
// --------------------------------------------------

void setup()
{
    // MP6550
    pinMode(SLEEP_PIN, OUTPUT);
    digitalWrite(SLEEP_PIN, LOW);

    // Motor PWM
    ledcSetup(PWM1_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(PWM2_CHANNEL, PWM_FREQ, PWM_RESOLUTION);

    ledcAttachPin(PWM1_PIN, PWM1_CHANNEL);
    ledcAttachPin(PWM2_PIN, PWM2_CHANNEL);

    ledcWrite(PWM1_CHANNEL, 0);
    ledcWrite(PWM2_CHANNEL, 0);

    // Servo
    servo.setPeriodHertz(50);
    servo.attach(SERVO_PIN, 500, 2500);
    servo.write(SERVO_CENTER_DEFAULT);

    // WiFi / ESP-NOW
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK)
    {
        motorDisable();
        return;
    }

    esp_now_register_recv_cb(onReceive);

    lastRxMs = millis();
}

// --------------------------------------------------
// LOOP
// --------------------------------------------------

void loop()
{
    if (kickActive && millis() - kickStartMs >= MOTOR_KICK_TIME_MS)
    {
        kickActive = false;

        ledcWrite(PWM1_CHANNEL, requestedPwm1);
        ledcWrite(PWM2_CHANNEL, requestedPwm2);
    }

    if (!failsafeActive && millis() - lastRxMs > RX_TIMEOUT_MS)
    {
        motorDisable();
        servo.write(SERVO_CENTER_DEFAULT);

        failsafeActive = true;
    }
}