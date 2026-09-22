#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>
#include <esp_wifi.h>

// --------------------------------------------------
// PINNAR
// --------------------------------------------------

#define PWM1_PIN 4 // MP6550 IN1 = FRAM
#define PWM2_PIN 5 // MP6550 IN2 = BACK
#define SERVO_PIN 6
#define SLEEP_PIN 7

// --------------------------------------------------
// SERVO
// --------------------------------------------------

#define SERVO_CENTER_DEFAULT 90

// --------------------------------------------------
// PWM MOTOR
// --------------------------------------------------
#define MOTOR_KICK_PWM 220
#define MOTOR_KICK_TIME_MS 70

#define PWM_FREQ 10000
#define PWM_RESOLUTION 8

// Höga LEDC-kanaler för att minska risk för konflikt
// med ESP32Servo
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
//
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

void onReceive(
    const uint8_t *mac,
    const uint8_t *data,
    int len)
{
  (void)mac;

  if (len != sizeof(ControlData))
  {
    Serial.printf(
        "Fel paketlangd: %d, forvantade %d\n",
        len,
        sizeof(ControlData));

    return;
  }

  ControlData rx;

  memcpy(
      &rx,
      data,
      sizeof(rx));

  uint8_t angle =
      constrain(
          rx.angle,
          0,
          180);

  lastRxMs = millis();
  failsafeActive = false;

  // --------------------------
  // SERVO
  // --------------------------

  servo.write(angle);

  // --------------------------
  // MOTOR SAFETY
  // --------------------------

  // TX säger disable
  if (rx.enable == 0)
  {
    motorDisable();

    Serial.printf(
        "Servo=%u  Motor=DISABLED\n",
        angle);

    return;
  }

  // Extra RX-säkerhet:
  // fram och back får aldrig vara aktiva samtidigt.
  if (rx.pwm1 > 0 && rx.pwm2 > 0)
  {
    motorDisable();

    Serial.printf(
        "Servo=%u  FEL: PWM1+PWM2 aktiva -> DISABLED\n",
        angle);

    return;
  }

  // Om enable är satt men båda PWM ändå är 0
  // håller vi också drivern avstängd.
  if (rx.pwm1 == 0 && rx.pwm2 == 0)
  {
    motorDisable();

    Serial.printf(
        "Servo=%u  Motor=DISABLED\n",
        angle);

    return;
  }

  // --------------------------
  // ENABLE MP6550
  // --------------------------

  digitalWrite(
      SLEEP_PIN,
      HIGH);

  // --------------------------
  // MOTOR PWM
  // --------------------------

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

      // Spara kommandot igen eftersom motorDisable() nollställer dem
      requestedPwm1 = rx.pwm1;
      requestedPwm2 = rx.pwm2;
  }

  digitalWrite(
      SLEEP_PIN,
      HIGH);

  // Startkick bara när motorn går från OFF till ON
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
      else if (requestedPwm2 > 0)
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

  Serial.printf(
      "Servo=%u  PWM1=%u  PWM2=%u  ENABLE=1\n",
      angle,
      rx.pwm1,
      rx.pwm2);
}

// --------------------------------------------------
// SETUP
// --------------------------------------------------

void setup()
{
  Serial.begin(115200);

  delay(500);

  Serial.println();
  Serial.println("ESP32-S3 RX startar...");

  // --------------------------------------------------
  // MP6550 SLEEP
  // --------------------------------------------------

  pinMode(
      SLEEP_PIN,
      OUTPUT);

  // Viktigt:
  // motordrivern avstängd redan från start.
  digitalWrite(
      SLEEP_PIN,
      LOW);

  // --------------------------------------------------
  // MOTOR PWM
  // --------------------------------------------------

  ledcSetup(
      PWM1_CHANNEL,
      PWM_FREQ,
      PWM_RESOLUTION);

  ledcSetup(
      PWM2_CHANNEL,
      PWM_FREQ,
      PWM_RESOLUTION);

  ledcAttachPin(
      PWM1_PIN,
      PWM1_CHANNEL);

  ledcAttachPin(
      PWM2_PIN,
      PWM2_CHANNEL);

  ledcWrite(
      PWM1_CHANNEL,
      0);

  ledcWrite(
      PWM2_CHANNEL,
      0);

  // --------------------------------------------------
  // SERVO
  // --------------------------------------------------

  servo.setPeriodHertz(50);

  servo.attach(
      SERVO_PIN,
      500,
      2500);

  servo.write(
      SERVO_CENTER_DEFAULT);

  // --------------------------------------------------
  // WIFI / ESP-NOW
  // --------------------------------------------------

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  esp_wifi_set_channel(
      1,
      WIFI_SECOND_CHAN_NONE);

  Serial.print("RX MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println("ESP-NOW channel = 1");

  if (esp_now_init() != ESP_OK)
  {
      Serial.println("esp_now_init FAIL");
      motorDisable();
      return;
  }

  esp_now_register_recv_cb(onReceive);

  lastRxMs = millis();

  // --------------------------------------------------
  // STATUS
  // --------------------------------------------------

  Serial.println("RX klar");
  Serial.println("GPIO7 = MP6550 SLEEP");
  Serial.println("GPIO4 = MP6550 IN1 / FRAM");
  Serial.println("GPIO5 = MP6550 IN2 / BACK");
  Serial.println("GPIO6 = Servo");
  Serial.println("Motor driver startar DISABLED");
}

// --------------------------------------------------
// LOOP
// --------------------------------------------------

void loop()
{
    static uint32_t lastAliveMs = 0;

    if (millis() - lastAliveMs > 1000)
    {
        lastAliveMs = millis();
        Serial.println("RX alive");
    }

    if (
        kickActive &&
        millis() - kickStartMs >= MOTOR_KICK_TIME_MS)
    {
        kickActive = false;

        ledcWrite(
            PWM1_CHANNEL,
            requestedPwm1);

        ledcWrite(
            PWM2_CHANNEL,
            requestedPwm2);

        Serial.printf(
            "Kick klar -> PWM1=%u PWM2=%u\n",
            requestedPwm1,
            requestedPwm2);
    }

    if (
        !failsafeActive &&
        millis() - lastRxMs > RX_TIMEOUT_MS)
    {
        motorDisable();

        servo.write(
            SERVO_CENTER_DEFAULT);

        failsafeActive = true;

        Serial.println(
            "RX timeout - SLEEP LOW, motor OFF, servo center");
    }
}