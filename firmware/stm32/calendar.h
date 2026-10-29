#pragma once
#include "menu.h"
int calendar_valid(const MenuDate *date);
uint32_t calendar_seconds(const MenuDate *date);
MenuDate calendar_date(uint32_t seconds);
unsigned calendar_weekday(const MenuDate *date); // Monday=0.
