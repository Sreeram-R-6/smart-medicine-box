#pragma once
#include "calendar.h"
#define ALARM_WINDOW_SECONDS 900u
enum AlarmEventType { ALARM_DUE,ALARM_LID_ACK,ALARM_MISSED,ALARM_DOWNTIME,ALARM_QUIET,ALARM_CLOCK_GAP };
typedef struct {
 uint32_t due,started,window_ms;
 uint8_t active,mask,armed,muted;
 char names[MENU_SLOTS][17];
} AlarmGroup;
typedef struct {
 uint32_t cursor,last_due[MENU_SLOTS];
 AlarmGroup groups[MENU_SLOTS];
} AlarmCheckpoint;
typedef struct { uint32_t due;uint8_t type,mask; } AlarmEvent;
typedef struct {
 AlarmCheckpoint saved;
 AlarmEvent events[16];
 uint8_t read,write,selected,show,previous_open,recovering,dirty,test_active,name_page;
 uint32_t test_started,sound_started,revision,lost_events;
} Alarm;
void alarm_init(Alarm *alarm,int restoring);
void alarm_poll(Alarm *alarm,const MenuLive *live,const MenuSchedule slots[MENU_SLOTS]);
void alarm_button(Alarm *alarm,enum MenuButton button);
void alarm_test(Alarm *alarm,uint32_t tick);
unsigned alarm_active_count(const Alarm *alarm);
int alarm_event(Alarm *alarm,AlarmEvent *event);
unsigned alarm_frequency(const Alarm *alarm,uint32_t tick);
void alarm_render(const Alarm *alarm,const MenuLive *live,MenuFrame *frame);
