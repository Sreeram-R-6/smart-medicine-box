#pragma once
#include "alarm.h"
#define SCREEN_BOOT_MS 2200u
/* Pixel-exact, hardware-independent renderer for the 128x64 OLED. */
void screen_render(uint8_t pixels[1024],const Menu *menu,const MenuLive *live,const Alarm *alarm);
void screen_boot(uint8_t pixels[1024],uint32_t elapsed);
