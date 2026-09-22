#pragma once

#define BTN_START_PIN 25
#define BTN_STOP_PIN 26
#define BTN_PLUS_PIN 14
#define BTN_MINUS_PIN 32

#define SLIDER_PIN_CFG 34
#define PWM_SLIDER_PIN_CFG 39

#define DMX_RX_PIN 16
#define DMX_TX_PIN 17
#define DMX_EN_PIN 4


static constexpr uint8_t PLAYBACK_STOP_VALUE_DEFAULT = 127; // 0..255

static constexpr uint8_t PLAYBACK_SLOTS = 5;