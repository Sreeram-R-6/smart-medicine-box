#include "screen.h"
#include "upcoming.h"
#include "calendar.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Menu m;
static Alarm a;
static MenuLive live={.date={2026,10,29,19,42,17},.clock_valid=1,.clock_trusted=1,.storage_ok=1,.clock_progress=3,.link_up=1};
static struct {uint8_t before[16],pixels[1024],after[16];} guarded;
static FILE *out;
static void save(const char *name,int boot){
 memset(&guarded,0xa7,sizeof guarded);
 if(boot>=0)screen_boot(guarded.pixels,(uint32_t)boot);else screen_render(guarded.pixels,&m,&live,&a);
 for(unsigned i=0;i<16;i++)assert(guarded.before[i]==0xa7&&guarded.after[i]==0xa7);
 char title[32]={0};snprintf(title,sizeof title,"%s",name);fwrite(title,1,32,out);fwrite(guarded.pixels,1,1024,out);
}
static void next_tests(void){
 MenuSchedule slots[8]={0};NextReminder next;MenuDate date={2026,10,29,23,1,0};
 slots[0]=(MenuSchedule){"FIRST",6,0,127,1};
 assert(upcoming_reminder(slots,&date,&next)&&next.days_away==1&&calendar_date(next.epoch).hour==6);
 slots[0].days=2;assert(upcoming_reminder(slots,&date,&next)&&next.days_away==5);
 date=(MenuDate){2026,10,27,6,0,0};assert(upcoming_reminder(slots,&date,&next)&&next.days_away==0);
 date.second=1;assert(upcoming_reminder(slots,&date,&next)&&next.days_away==7);
 slots[1]=slots[0];assert(upcoming_reminder(slots,&date,&next)&&next.mask==3);
 slots[0].enabled=slots[1].enabled=0;assert(!upcoming_reminder(slots,&date,&next));
 slots[0].enabled=1;date=(MenuDate){2099,12,31,23,59,59};assert(!upcoming_reminder(slots,&date,&next));
 date=(MenuDate){2026,2,30,0,0,0};assert(!upcoming_reminder(slots,&date,&next));
}
int main(void){
 next_tests();menu_init(&m,&live);alarm_init(&a,0);
 m.schedules[0]=(MenuSchedule){"EVENING MEDICINE",20,0,127,1};
 m.schedules[1]=(MenuSchedule){"SECOND MEDICINE",20,0,127,1};
 out=fopen(".pio/oled-ui-previews.bin","wb");assert(out);
 save("BOOT REVEAL",650);save("BOOT BRAND",1400);save("BOOT FINISH",2100);
 save("HOME NEXT MEDICINE",-1);live.uptime=3100;save("HOME GROUP SECOND NAME",-1);
 live.date.hour=23;live.date.minute=30;save("HOME TOMORROW",-1);live.date.hour=19;live.date.minute=42;
 live.clock_trusted=0;live.rtc_osf=1;save("CLOCK WARNING",-1);live.clock_trusted=1;live.rtc_osf=0;
 m.view=UI_MAIN;save("MAIN MENU",-1);m.selection=3;save("MAIN SETTINGS SELECTED",-1);
 m.view=UI_SCHEDULES;m.slot=0;save("SCHEDULE FULL NAME",-1);
 m.schedules[0].days=21;save("SCHEDULE WEEKDAYS",-1);m.schedules[0].days=127;
 m.view=UI_SCHEDULE;m.schedule_draft=m.schedules[0];m.field=0;save("EDIT ALARM NAME",-1);m.field=5;save("EDIT ALARM SAVE",-1);
 m.view=UI_CLOCK;m.clock_draft=live.date;m.field=3;m.editing=1;save("CLOCK EDIT HOUR",-1);m.field=6;m.editing=0;save("CLOCK SAVE",-1);
 m.view=UI_NAME;m.name_cursor=15;memcpy(m.schedule_draft.name,"ABCDEFGHIJKLMNOP",17);save("NAME MAXIMUM LENGTH",-1);
 m.view=UI_DAYS;m.day_cursor=4;save("DAYS EDITOR",-1);
 m.view=UI_SETTINGS;m.settings_selection=1;m.settings.duration_ms=2000;save("SETTINGS MAX LENGTH",-1);m.settings_selection=4;save("SETTINGS DIAGNOSTICS",-1);
 m.view=UI_HISTORY;save("HISTORY EVENT",-1);
 m.view=UI_NOTICE;snprintf(m.notice,sizeof m.notice,"SCHEDULE SAVED");snprintf(m.notice_detail,sizeof m.notice_detail,"READY TO USE");save("SAVE CONFIRMED",-1);
 alarm_test(&a,0);save("REPEATING TEST ALARM",-1);a.test_active=0;
 a.saved.groups[0].active=1;a.saved.groups[0].mask=1;a.saved.groups[0].armed=1;a.saved.groups[0].due=calendar_seconds(&live.date);strcpy(a.saved.groups[0].names[0],"EVENING MEDICINE");a.show=1;save("MEDICINE ALARM",-1);
 // Every view, selection and edit state stays inside the OLED buffer.
 for(unsigned view=0;view<=UI_NOTICE;view++)for(unsigned selection=0;selection<8;selection++){
  a.show=0;a.saved.groups[0].active=0;m.view=(enum MenuView)view;m.selection=selection%4;m.field=selection%6;m.slot=selection;m.settings_selection=selection%5;m.day_cursor=selection;m.name_cursor=selection;m.editing=selection&1;
  memset(&guarded,0xa7,sizeof guarded);screen_render(guarded.pixels,&m,&live,&a);for(unsigned i=0;i<16;i++)assert(guarded.before[i]==0xa7&&guarded.after[i]==0xa7);
 }
 fclose(out);puts("PASS: next reminder weekday/midnight/week/group/calendar bounds; guarded render of all views; 23 pixel previews");return 0;
}
