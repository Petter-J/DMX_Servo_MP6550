#include "InputSources.h"
#include <esp_dmx.h>
#include "Config.h"

static const dmx_port_t dmxPort = DMX_NUM_1;
static uint8_t dmxData[DMX_PACKET_SIZE];

void InputSources::begin()
{
    pinMode(SLIDER_PIN, INPUT);
    pinMode(PWM_SLIDER_PIN, INPUT);

    // analogRead -> 0..255
    analogReadResolution(8);

    dmx_config_t config = DMX_CONFIG_DEFAULT;

    bool ok = dmx_driver_install(
        dmxPort,
        &config,
        nullptr,
        0);

    if (!ok)
    {
        Serial.println("DMX driver fail");
        return;
    }

    dmx_set_pin(
        dmxPort,
        DMX_TX_PIN,
        DMX_RX_PIN,
        DMX_EN_PIN);

    Serial.println("DMX input ready");
}

uint8_t InputSources::readDmx(uint16_t address)
{
    if (address < 1)
        address = 1;

    if (address > 512)
        address = 512;

    static uint8_t lastGood = 0;

    dmx_packet_t packet;

    int packetSize = dmx_receive(
        dmxPort,
        &packet,
        1);

    if (packetSize > 0 && packet.err == DMX_OK)
    {
        lastDmxPacketMs = millis();

        dmx_read(
            dmxPort,
            dmxData,
            packet.size);

        // dmxData[0] = start code
        // DMX kanal 1 = dmxData[1]

        uint16_t index = address;

        if (index < packet.size)
        {
            lastGood = dmxData[index];
        }
    }

    return lastGood;
}

void InputSources::readDmx3(
    uint16_t address,
    uint8_t &servo,
    uint8_t &pwm1,
    uint8_t &pwm2)
{
    // address     = servo
    // address + 1 = PWM1
    // address + 2 = PWM2

    if (address < 1)
        address = 1;

    if (address > 510)
        address = 510;

    static uint8_t lastServo = 0;
    static uint8_t lastPwm1 = 0;
    static uint8_t lastPwm2 = 0;

    dmx_packet_t packet;

    int packetSize = dmx_receive(
        dmxPort,
        &packet,
        1);

    if (packetSize > 0)
    {
        // DMX-signal finns
        lastDmxPacketMs = millis();

        // Uppdatera värden bara om paketet är korrekt
        if (packet.err == DMX_OK)
        {
            dmx_read(
                dmxPort,
                dmxData,
                packet.size);

            uint16_t servoIndex = address;
            uint16_t pwm1Index = address + 1;
            uint16_t pwm2Index = address + 2;

            if (servoIndex < packet.size)
                lastServo = dmxData[servoIndex];

            if (pwm1Index < packet.size)
                lastPwm1 = dmxData[pwm1Index];

            if (pwm2Index < packet.size)
                lastPwm2 = dmxData[pwm2Index];
        }
    }

    servo = lastServo;
    pwm1 = lastPwm1;
    pwm2 = lastPwm2;
}

uint8_t InputSources::readSlider()
{
    return static_cast<uint8_t>(
        analogRead(SLIDER_PIN));
}

uint8_t InputSources::readPwmSlider()
{
    return static_cast<uint8_t>(
        analogRead(PWM_SLIDER_PIN));
}

bool InputSources::dmxOk() const
{
    if (lastDmxPacketMs == 0)
        return false;

    return (millis() - lastDmxPacketMs) <
           DMX_LOST_TIMEOUT_MS;
}