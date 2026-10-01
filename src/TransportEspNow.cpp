#include "TransportEspNow.h"
#include <esp_wifi.h>

volatile bool TransportEspNow::lastSendSuccess = false;
volatile uint32_t TransportEspNow::lastSendMs = 0;

void TransportEspNow::onSent(const uint8_t *mac, esp_now_send_status_t status)
{
    (void)mac;

    lastSendSuccess = (status == ESP_NOW_SEND_SUCCESS);
    lastSendMs = millis();
}

void TransportEspNow::begin(uint8_t receiverIndex)
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Lås ESP-NOW till kanal 1
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK)
        return;

    esp_now_register_send_cb(onSent);

    setReceiver(receiverIndex);
}

void TransportEspNow::setReceiver(uint8_t receiverIndex)
{
    if (receiverIndex >= RECEIVER_COUNT)
        receiverIndex = 0;

    esp_now_del_peer(receiverMac);

    memcpy(receiverMac, RECEIVERS[receiverIndex].mac, 6);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMac, 6);

    peerInfo.channel = 1;
    peerInfo.ifidx = WIFI_IF_STA;
    peerInfo.encrypt = false;

    esp_now_add_peer(&peerInfo);

    lastSendSuccess = false;
    lastSendMs = 0;
}

void TransportEspNow::send(
    uint8_t angle,
    uint8_t pwm1,
    uint8_t pwm2,
    bool enable)
{
    p.angle = angle;
    p.pwm1 = pwm1;
    p.pwm2 = pwm2;
    p.enable = enable ? 1 : 0;

    esp_now_send(
        receiverMac,
        reinterpret_cast<uint8_t *>(&p),
        sizeof(p));
}

bool TransportEspNow::linkOk() const
{
    if (!lastSendSuccess)
        return false;

    if (lastSendMs == 0)
        return false;

    return (millis() - lastSendMs) < LINK_TIMEOUT_MS;
}