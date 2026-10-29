#pragma once
#include "menu.h"
#include "control.h"
uint16_t remote_revision(const Menu *m);
int remote_slot(MenuSchedule *s,const uint8_t *p,unsigned n);
int remote_settings(MenuSettings *s,const uint8_t *p,unsigned n);
unsigned remote_state(uint8_t *p,const Menu *m,const MenuLive *l,unsigned test);
