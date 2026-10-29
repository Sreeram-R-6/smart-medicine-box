#pragma once
#include "calendar.h"
typedef struct {
 uint32_t last_epoch,last_change,last_sample,progress,errors,jumps;
 uint8_t initialized,valid,trusted,osf,stalled,fault;
 MenuDate date;
} RtcHealth;
void rtc_health_sample(RtcHealth *health,const uint8_t registers[19],int read_ok,uint32_t tick);
