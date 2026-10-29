#pragma once
#include <stdint.h>

#define MENU_SLOTS 8
#define MENU_HISTORY_SIZE 32
#define MENU_COLUMNS 21
enum MenuButton { MENU_LEFT, MENU_RIGHT, MENU_BACK, MENU_OK };
enum MenuView {
 UI_HOME, UI_MAIN, UI_CLOCK, UI_CLOCK_WAIT, UI_SCHEDULES, UI_SCHEDULE,
 UI_NAME, UI_DAYS, UI_HISTORY, UI_EVENT, UI_SETTINGS, UI_DIAGNOSTICS, UI_NOTICE
};
enum MenuEventType { EVENT_BOOT, EVENT_LID_OPEN, EVENT_LID_CLOSED, EVENT_CLOCK_SET, EVENT_SCHEDULE, EVENT_SETTINGS,
 EVENT_ALARM_DUE,EVENT_ALARM_ACK,EVENT_ALARM_MISSED,EVENT_DOWNTIME,EVENT_ALARM_QUIET,EVENT_CLOCK_GAP,EVENT_TEST_ALARM };
typedef struct { uint16_t year; uint8_t month,day,hour,minute,second; } MenuDate;
typedef struct {
 MenuDate date;
 uint32_t uptime, lid_open, clock_valid, clock_trusted, button_mask, oled_errors, rtc_errors;
 uint32_t alarm_count,storage_ok,clock_progress,rtc_osf,link_up;
} MenuLive;
typedef struct { char name[17]; uint8_t hour,minute,days,enabled; } MenuSchedule;
typedef struct { uint8_t sound,brightness; uint16_t duration_ms; } MenuSettings;
typedef struct { MenuDate date; uint32_t uptime; uint8_t type,dated,mask,reserved;uint32_t due; } MenuEvent;
typedef struct {
 char rows[8][MENU_COLUMNS+1];
 int8_t selected_row, character_column;
} MenuFrame;
typedef struct {
 enum MenuView view, notice_return;
 uint8_t selection, slot, field, editing, name_cursor, day_cursor;
 uint8_t main_selection, settings_selection, history_selection;
 uint8_t history_head,history_count,clock_command,test_sound,contrast_dirty;
 uint8_t persist_dirty,persist_wait;
 uint32_t revision;
 MenuDate clock_draft,clock_backup;
 MenuSchedule schedules[MENU_SLOTS],schedule_draft;
 MenuSettings settings;
 uint16_t setting_draft;
 char name_backup[17], notice[22],notice_detail[22];
 uint8_t days_backup;
 MenuEvent history[MENU_HISTORY_SIZE];
 MenuEvent event_detail;
} Menu;

void menu_init(Menu *m,const MenuLive *live);
void menu_button(Menu *m,enum MenuButton button,const MenuLive *live);
void menu_clock_result(Menu *m,unsigned result,const MenuLive *live);
void menu_clock_bcd(const Menu *m,uint8_t registers[7]);
void menu_record(Menu *m,enum MenuEventType type,const MenuLive *live);
void menu_storage_result(Menu *m,int success);
void menu_render(const Menu *m,const MenuLive *live,MenuFrame *frame);
int menu_can_repeat(const Menu *m);
int menu_days_in_month(unsigned year,unsigned month);
