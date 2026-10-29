#pragma once
#include "menu.h"
typedef struct {uint32_t epoch;uint8_t mask,slot,days_away;} NextReminder;
int upcoming_reminder(const MenuSchedule slots[MENU_SLOTS],const MenuDate *now,NextReminder *next);
