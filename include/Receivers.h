#pragma once

#include <Arduino.h>

struct ReceiverInfo
{
    const char *name;
    uint8_t mac[6];
};

static const ReceiverInfo RECEIVERS[] =
    {
        {"Fjaril", {0x3C, 0x0F, 0x02, 0xE4, 0xCD, 0x58}},
        {"Fisk", {0x1C, 0xDB, 0xD4, 0x82, 0xE3, 0x74}}};

static constexpr uint8_t RECEIVER_COUNT =
    sizeof(RECEIVERS) / sizeof(RECEIVERS[0]);