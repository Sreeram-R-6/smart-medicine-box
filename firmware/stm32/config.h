#pragma once
#include "stm32f1xx_hal.h"
#define FW_VERSION "1.2.2-oled-startup"
#define BUTTON_PORT GPIOA
static const uint16_t BUTTON_PINS[4] = {GPIO_PIN_0,GPIO_PIN_1,GPIO_PIN_4,GPIO_PIN_5};
#define SENSOR_PORT GPIOB
#define SENSOR_PIN GPIO_PIN_12
// FC-51 nominal active-low detection; verify against the module indicator.
#define SENSOR_DETECT_LEVEL GPIO_PIN_RESET
// Two-position physical calibration: closed reads LOW, open reads HIGH.
// Opening chirps and alarm acknowledgment use the LOW -> HIGH transition.
#define SENSOR_OPEN_LEVEL GPIO_PIN_SET
#define SENSOR_VERIFIED 1
#define BUZZER_PORT GPIOB
#define BUZZER_PIN GPIO_PIN_0
#define BUZZER_OFF GPIO_PIN_SET
#define BUZZER_ON GPIO_PIN_RESET
#define OPEN_CHIRP_MS 500u
#define ENABLE_OPEN_CHIRP 1
#define BUZZER_TONE_HZ 2000u
#define GREEN_PIN GPIO_PIN_8
#define RED_PIN GPIO_PIN_9
#define BUTTON_DEBOUNCE_MS 30u
#define SENSOR_FILTER_MS 100u
#define ENABLE_I2C_DIAGNOSTICS 1
// Set to 1 only after checking supply and existing pull-ups.
// Trial geometry/controller; an I2C ACK cannot identify the display.
#define OLED_HEIGHT 64
