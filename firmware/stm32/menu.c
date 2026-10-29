#include "menu.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static const char *const event_names[]={"STARTUP","LID OPEN","LID CLOSED","CLOCK SET","SCHEDULE EDIT","SETTINGS EDIT",
 "REMINDER DUE","LID LOGGED","REMINDER MISSED","DOWNTIME CHECK","ALARM QUIET","CLOCK GAP","TEST ALARM"};
static const char *const day_names[]={"MON","TUE","WED","THU","FRI","SAT","SUN"};
static const char alphabet[]=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static unsigned wrap(unsigned value,int delta,unsigned count){return (value+count+(unsigned)delta)%count;}
int menu_days_in_month(unsigned y,unsigned m){
 static const uint8_t days[]={31,28,31,30,31,30,31,31,30,31,30,31};
 return m>=1&&m<=12?days[m-1]+(m==2&&y%4==0&&(y%100!=0||y%400==0)):0;
}
static void row(MenuFrame *f,unsigned y,const char *fmt,...){
 if(y>=8)return;
 va_list ap;va_start(ap,fmt);vsnprintf(f->rows[y],sizeof f->rows[y],fmt,ap);va_end(ap);
}
static void notice(Menu *m,const char *text,const char *detail,enum MenuView back){
 snprintf(m->notice,sizeof m->notice,"%s",text);snprintf(m->notice_detail,sizeof m->notice_detail,"%s",detail);
 m->notice_return=back;m->view=UI_NOTICE;m->editing=0;
}
static const MenuEvent *event_at(const Menu *m,unsigned newest){
 return &m->history[(m->history_head+MENU_HISTORY_SIZE-1-newest)%MENU_HISTORY_SIZE];
}
void menu_record(Menu *m,enum MenuEventType type,const MenuLive *l){
 if((m->view==UI_HISTORY||m->view==UI_EVENT)&&m->history_count&&m->history_selection<MENU_HISTORY_SIZE-1)m->history_selection++;
 MenuEvent *e=&m->history[m->history_head];
 e->date=l->date;e->uptime=l->uptime;e->type=(uint8_t)type;e->dated=(uint8_t)(l->clock_valid&&l->clock_trusted);
 e->mask=0;e->reserved=0;e->due=0;m->persist_dirty=1;
 m->history_head=(m->history_head+1)%MENU_HISTORY_SIZE;
 if(m->history_count<MENU_HISTORY_SIZE)m->history_count++;
 m->revision++;
}
void menu_init(Menu *m,const MenuLive *l){
 memset(m,0,sizeof *m);m->view=UI_HOME;m->settings.sound=1;m->settings.brightness=5;m->settings.duration_ms=500;
 for(unsigned i=0;i<MENU_SLOTS;i++){
  snprintf(m->schedules[i].name,sizeof m->schedules[i].name,"MED %u",i+1);
  m->schedules[i].hour=8;m->schedules[i].days=0x7f;
 }
 menu_record(m,EVENT_BOOT,l);
}
static void enter_clock(Menu *m,const MenuLive *l){
 m->clock_draft=l->clock_valid?l->date:(MenuDate){2026,1,1,0,0,0};
 if(m->clock_draft.year<2000||m->clock_draft.year>2099)m->clock_draft=(MenuDate){2026,1,1,0,0,0};
 m->field=0;m->editing=0;m->view=UI_CLOCK;
}
static void adjust_clock(Menu *m,int d){
 MenuDate *c=&m->clock_draft;
 switch(m->field){
 case 0:c->year=(uint16_t)(2000+wrap(c->year-2000,d,100));break;
 case 1:c->month=(uint8_t)(1+wrap(c->month-1,d,12));break;
 case 2:c->day=(uint8_t)(1+wrap(c->day-1,d,(unsigned)menu_days_in_month(c->year,c->month)));break;
 case 3:c->hour=(uint8_t)wrap(c->hour,d,24);break;
 case 4:c->minute=(uint8_t)wrap(c->minute,d,60);break;
 case 5:c->second=(uint8_t)wrap(c->second,d,60);break;
 default:break;
 }
 unsigned last=(unsigned)menu_days_in_month(c->year,c->month);if(c->day>last)c->day=(uint8_t)last;
}
static void save_schedule(Menu *m,const MenuLive *l){
 int end=15;while(end>=0&&m->schedule_draft.name[end]==' ')m->schedule_draft.name[end--]=0;
 int nonblank=0;for(unsigned i=0;i<16&&m->schedule_draft.name[i];i++)if(m->schedule_draft.name[i]!=' ')nonblank=1;
 if(!nonblank){notice(m,"ENTER A NAME","SELECT NAME TO EDIT",UI_SCHEDULE);m->field=0;return;}
 if(!m->schedule_draft.days){notice(m,"SELECT AT LEAST 1 DAY","OPEN DAYS TO CHANGE",UI_SCHEDULE);m->field=3;return;}
 m->schedules[m->slot]=m->schedule_draft;menu_record(m,EVENT_SCHEDULE,l);
 m->persist_wait=1;notice(m,"SAVING SCHEDULE","PLEASE WAIT",UI_SCHEDULES);
}
void menu_storage_result(Menu *m,int success){
 if(!m->persist_wait)return;
 m->persist_wait=0;
 notice(m,success?"SCHEDULE SAVED":"SAVE FAILED",success?"READY TO USE":"CHECK STORAGE RETRY",UI_SCHEDULES);
 m->revision++;
}
void menu_button(Menu *m,enum MenuButton b,const MenuLive *l){
 int d=b==MENU_LEFT?-1:1;m->revision++;
 switch(m->view){
 case UI_HOME:
  if(b==MENU_OK){m->view=UI_MAIN;m->selection=m->main_selection;}
  break;
 case UI_MAIN:
  if(b==MENU_BACK){m->view=UI_HOME;break;}
  if(b==MENU_LEFT||b==MENU_RIGHT){m->selection=(uint8_t)wrap(m->selection,d,4);m->main_selection=m->selection;}
  if(b==MENU_OK){
   switch(m->selection){
   case 0:enter_clock(m,l);break;
   case 1:m->view=UI_SCHEDULES;break;
   case 2:m->view=UI_HISTORY;m->history_selection=0;break;
   case 3:m->view=UI_SETTINGS;break;
   }
  }
  break;
 case UI_CLOCK:
  if(b==MENU_BACK){if(m->editing){m->clock_draft=m->clock_backup;m->editing=0;}else m->view=UI_MAIN;break;}
  if(b==MENU_LEFT||b==MENU_RIGHT){if(m->editing)adjust_clock(m,d);else m->field=(uint8_t)wrap(m->field,d,7);}
  if(b==MENU_OK){
   if(m->field==6){m->clock_command=1;m->view=UI_CLOCK_WAIT;}
   else if(m->editing)m->editing=0;
   else{m->clock_backup=m->clock_draft;m->editing=1;}
  }
  break;
 case UI_CLOCK_WAIT:break; // Let the in-flight I2C write complete before navigation.
 case UI_SCHEDULES:
  if(b==MENU_BACK)m->view=UI_MAIN;
  else if(b==MENU_LEFT||b==MENU_RIGHT)m->slot=(uint8_t)wrap(m->slot,d,MENU_SLOTS);
  else if(b==MENU_OK){m->schedule_draft=m->schedules[m->slot];m->view=UI_SCHEDULE;m->field=0;m->editing=0;}
  break;
 case UI_SCHEDULE:
  if(b==MENU_BACK){if(m->editing)m->editing=0;else m->view=UI_SCHEDULES;break;}
  if(b==MENU_LEFT||b==MENU_RIGHT){
   if(!m->editing)m->field=(uint8_t)wrap(m->field,d,6);
   else if(m->field==1)m->setting_draft=(uint16_t)wrap(m->setting_draft,d,24);
   else if(m->field==2)m->setting_draft=(uint16_t)wrap(m->setting_draft,d,60);
   else if(m->field==4)m->setting_draft=!m->setting_draft;
  }
  if(b==MENU_OK){
   if(m->field==0){
    memcpy(m->name_backup,m->schedule_draft.name,17);size_t n=strlen(m->schedule_draft.name);
    memset(m->schedule_draft.name+n,' ',16-n);m->schedule_draft.name[16]=0;m->name_cursor=0;m->editing=0;m->view=UI_NAME;
   }else if(m->field==3){m->days_backup=m->schedule_draft.days;m->day_cursor=0;m->view=UI_DAYS;}
   else if(m->field==5)save_schedule(m,l);
   else if(m->editing){
    if(m->field==1)m->schedule_draft.hour=(uint8_t)m->setting_draft;
    if(m->field==2)m->schedule_draft.minute=(uint8_t)m->setting_draft;
    if(m->field==4)m->schedule_draft.enabled=(uint8_t)m->setting_draft;
    m->editing=0;
   }else{m->setting_draft=m->field==1?m->schedule_draft.hour:m->field==2?m->schedule_draft.minute:m->schedule_draft.enabled;m->editing=1;}
  }
  break;
 case UI_NAME:
  if(b==MENU_BACK){
   if(m->editing){m->schedule_draft.name[m->name_cursor]=(char)m->setting_draft;m->editing=0;}
   else{memcpy(m->schedule_draft.name,m->name_backup,17);m->view=UI_SCHEDULE;}
  }
  else if(b==MENU_LEFT||b==MENU_RIGHT){
   if(!m->editing)m->name_cursor=(uint8_t)wrap(m->name_cursor,d,17);
   else{
    const char *p=strchr(alphabet,m->schedule_draft.name[m->name_cursor]);unsigned i=p?(unsigned)(p-alphabet):0;
    m->schedule_draft.name[m->name_cursor]=alphabet[wrap(i,d,sizeof alphabet-1)];
   }
  }else if(b==MENU_OK){
   if(m->name_cursor==16)m->view=UI_SCHEDULE;
   else if(m->editing)m->editing=0;
   else{m->setting_draft=(uint8_t)m->schedule_draft.name[m->name_cursor];m->editing=1;}
  }
  break;
 case UI_DAYS:
  if(b==MENU_BACK){m->schedule_draft.days=m->days_backup;m->view=UI_SCHEDULE;}
  else if(b==MENU_LEFT||b==MENU_RIGHT)m->day_cursor=(uint8_t)wrap(m->day_cursor,d,8);
  else if(b==MENU_OK){if(m->day_cursor==7)m->view=UI_SCHEDULE;else m->schedule_draft.days^=(uint8_t)(1u<<m->day_cursor);}
  break;
 case UI_HISTORY:
  if(b==MENU_BACK)m->view=UI_MAIN;
  else if((b==MENU_LEFT||b==MENU_RIGHT)&&m->history_count)m->history_selection=(uint8_t)wrap(m->history_selection,d,m->history_count);
  else if(b==MENU_OK&&m->history_count){m->event_detail=*event_at(m,m->history_selection);m->view=UI_EVENT;}
  break;
 case UI_EVENT:
  if(b==MENU_BACK)m->view=UI_HISTORY;
  else if((b==MENU_LEFT||b==MENU_RIGHT)&&m->history_count){m->history_selection=(uint8_t)wrap(m->history_selection,d,m->history_count);m->event_detail=*event_at(m,m->history_selection);}
  break;
 case UI_SETTINGS:
  if(b==MENU_BACK){if(m->editing)m->editing=0;else m->view=UI_MAIN;break;}
  if(b==MENU_LEFT||b==MENU_RIGHT){
   if(!m->editing)m->settings_selection=(uint8_t)wrap(m->settings_selection,d,5);
   else if(m->settings_selection==1)m->setting_draft=(uint16_t)(100+100*wrap(m->setting_draft/100-1,d,20));
   else if(m->settings_selection==2)m->setting_draft=(uint16_t)(1+wrap(m->setting_draft-1,d,10));
  }
  if(b==MENU_OK){
   switch(m->settings_selection){
   case 0:m->settings.sound=!m->settings.sound;menu_record(m,EVENT_SETTINGS,l);break;
   case 1:case 2:
    if(m->editing){
     if(m->settings_selection==1)m->settings.duration_ms=m->setting_draft;
     else{m->settings.brightness=(uint8_t)m->setting_draft;m->contrast_dirty=1;}
     m->editing=0;menu_record(m,EVENT_SETTINGS,l);
    }else{m->setting_draft=m->settings_selection==1?m->settings.duration_ms:m->settings.brightness;m->editing=1;}
    break;
   case 3:m->test_sound=1;break;
   case 4:m->view=UI_DIAGNOSTICS;break;
   }
  }
  break;
 case UI_DIAGNOSTICS:if(b==MENU_BACK)m->view=UI_SETTINGS;break;
 case UI_NOTICE:if(!m->persist_wait&&(b==MENU_BACK||b==MENU_OK))m->view=m->notice_return;break;
 }
}
void menu_clock_bcd(const Menu *m,uint8_t b[7]){
 const MenuDate *c=&m->clock_draft;
 unsigned values[]={c->second,c->minute,c->hour,0,c->day,c->month,c->year-2000};
 // Gregorian weekday, Sunday=0; the project uses Monday=1..Sunday=7.
 static const unsigned offsets[]={0,3,2,5,0,3,5,1,4,6,2,4};
 unsigned y=c->year-(c->month<3);unsigned dow=(y+y/4-y/100+y/400+offsets[c->month-1]+c->day)%7;
 values[3]=dow?dow:7;
 for(unsigned i=0;i<7;i++)b[i]=(uint8_t)((values[i]/10)*16+values[i]%10);
}
void menu_clock_result(Menu *m,unsigned result,const MenuLive *l){
 if(m->view!=UI_CLOCK_WAIT)return;
 if(result==2){menu_record(m,EVENT_CLOCK_SET,l);notice(m,"CLOCK SAVED","TIME UPDATED",UI_MAIN);}
 else if(result>=3)notice(m,"CLOCK WRITE FAILED","CHECK RTC AND RETRY",UI_CLOCK);
 m->revision++;
}
int menu_can_repeat(const Menu *m){return m->editing;}
static unsigned scroll_start(unsigned selected){return selected>=5?selected-4:0;}
static void select_row(MenuFrame *f,unsigned y){f->selected_row=(int8_t)y;}
static void field_row(MenuFrame *f,unsigned y,int selected,const char *fmt,...){
 if(y>=7)return;
 va_list ap;va_start(ap,fmt);vsnprintf(f->rows[y],sizeof f->rows[y],fmt,ap);va_end(ap);
 if(selected)select_row(f,y);
}
void menu_render(const Menu *m,const MenuLive *l,MenuFrame *f){
 memset(f,0,sizeof *f);f->selected_row=-1;f->character_column=-1;
 switch(m->view){
 case UI_HOME:{
  row(f,0,"MEDICINE BOX");
  if(l->clock_valid){row(f,1,"%02u:%02u:%02u",l->date.hour,l->date.minute,l->date.second);row(f,2,"%04u-%02u-%02u",l->date.year,l->date.month,l->date.day);}
  else{row(f,1,"CLOCK UNAVAILABLE");row(f,2,"SET CLOCK IN MENU");}
  row(f,3,"BOX: %s",l->lid_open?"OPEN":"CLOSED");
  row(f,4,"%s",l->clock_trusted?"CLOCK OK":l->rtc_osf||l->clock_progress>=3?"RTC NEEDS SYNC":l->clock_valid?"CHECKING CLOCK":"CLOCK UNAVAILABLE");
  unsigned enabled=0;for(unsigned i=0;i<MENU_SLOTS;i++)enabled+=m->schedules[i].enabled;
  row(f,5,"ALARMS ON %u/8 DUE %lu",enabled,(unsigned long)l->alarm_count);
  row(f,6,"%s",!l->storage_ok?"STORAGE ERROR":!l->clock_trusted?"NEW ALARMS PAUSED":l->alarm_count?"BACK+OK SHOW ALARMS":"BACK+OK TEST ALARM");
  row(f,7,"OK OPEN MENU");break;
 }
 case UI_MAIN:{
  const char *items[]={"SET CLOCK","SCHEDULES","HISTORY","SETTINGS"};row(f,0,"MAIN MENU");
  for(unsigned i=0;i<4;i++)field_row(f,i+1,i==m->selection,"%s",items[i]);
  row(f,6,"BACK HOME");row(f,7,"L/R MOVE OK SELECT");break;
 }
 case UI_CLOCK:{
  row(f,0,"SET CLOCK");row(f,1,"%s",m->editing?"BACK CANCEL VALUE":"DRAFT UNTIL SAVE");
  unsigned start=scroll_start(m->field);const MenuDate *c=&m->clock_draft;
  const char *labels[]={"YEAR","MONTH","DAY","HOUR","MINUTE","SECOND","SAVE TO RTC"};
  unsigned v[]={c->year,c->month,c->day,c->hour,c->minute,c->second};
  for(unsigned i=start;i<7&&i<start+5;i++){
   if(i==6)field_row(f,2+i-start,i==m->field,"%s",labels[i]);
   else if(i==0)field_row(f,2+i-start,i==m->field,"%s %04u",labels[i],v[i]);
   else field_row(f,2+i-start,i==m->field,"%s %02u",labels[i],v[i]);
  }
  row(f,7,"%s",m->editing?"L/R VALUE OK DONE":"L/R MOVE OK EDIT");break;
 }
 case UI_CLOCK_WAIT:row(f,0,"SET CLOCK");row(f,3,"WRITING AND VERIFYING");row(f,7,"PLEASE WAIT");break;
 case UI_SCHEDULES:{
  row(f,0,"MEDICINE ALARMS");row(f,1,"WEEKDAYS PER SLOT");unsigned start=scroll_start(m->slot);
  for(unsigned i=start;i<MENU_SLOTS&&i<start+5;i++){
   const MenuSchedule *s=&m->schedules[i];field_row(f,2+i-start,i==m->slot,"%u %02u:%02u %-3s %.8s",i+1,s->hour,s->minute,s->enabled?"ON":"OFF",s->name);
  }
  row(f,7,"L/R SLOT OK EDIT");break;
 }
 case UI_SCHEDULE:{
  row(f,0,"EDIT ALARM %u",m->slot+1);row(f,1,"%s",m->editing?"BACK CANCEL VALUE":"BACK CANCEL ALL");
  unsigned start=scroll_start(m->field);const MenuSchedule *s=&m->schedule_draft;
  for(unsigned i=start;i<6&&i<start+5;i++){
   unsigned y=2+i-start;int selected=i==m->field;
   switch(i){
   case 0:field_row(f,y,selected,"NAME %.16s",s->name);break;
   case 1:field_row(f,y,selected,"HOUR %02u",m->editing&&selected?m->setting_draft:s->hour);break;
   case 2:field_row(f,y,selected,"MINUTE %02u",m->editing&&selected?m->setting_draft:s->minute);break;
   case 3:{char days[8];for(unsigned d=0;d<7;d++)days[d]=(s->days&(1u<<d))?"MTWTFSS"[d]:'-';days[7]=0;field_row(f,y,selected,"DAYS %s",days);break;}
   case 4:field_row(f,y,selected,"ALARM %s",(m->editing&&selected?m->setting_draft:s->enabled)?"ON":"OFF");break;
   case 5:field_row(f,y,selected,"SAVE ALARM");break;
   }
  }
  row(f,7,"%s",m->editing?"L/R VALUE OK DONE":"L/R MOVE OK EDIT");break;
 }
 case UI_NAME:
  row(f,0,"EDIT MEDICINE NAME");row(f,1,"%s",m->editing?"EDITING LETTER":"SELECT LETTER OR DONE");row(f,3,"%.16s",m->schedule_draft.name);
  f->selected_row=m->name_cursor==16?5:3;f->character_column=m->name_cursor==16?-1:(int8_t)m->name_cursor;
  row(f,5,"DONE");row(f,6,"BACK CANCEL NAME");row(f,7,"%s",m->editing?"L/R LETTER OK DONE":"L/R CURSOR OK EDIT");break;
 case UI_DAYS:{
  row(f,0,"SELECT WEEKDAYS");row(f,1,"OK TOGGLE BACK CANCEL");unsigned start=scroll_start(m->day_cursor);
  for(unsigned i=start;i<8&&i<start+5;i++){
   if(i==7)field_row(f,2+i-start,i==m->day_cursor,"DONE");
   else field_row(f,2+i-start,i==m->day_cursor,"%s %s",day_names[i],m->schedule_draft.days&(1u<<i)?"ON":"OFF");
  }
  row(f,7,"L/R DAY OK SELECT");break;
 }
 case UI_HISTORY:{
  row(f,0,"DEVICE HISTORY");row(f,1,"ACTUAL DEVICE EVENTS");
  if(!m->history_count)row(f,3,"NO EVENTS YET");
  else{
   unsigned start=scroll_start(m->history_selection);
   for(unsigned i=start;i<m->history_count&&i<start+5;i++){
    const MenuEvent *e=event_at(m,i);
    if(e->dated)field_row(f,2+i-start,i==m->history_selection,"%02u:%02u %.15s",e->date.hour,e->date.minute,event_names[e->type]);
    else field_row(f,2+i-start,i==m->history_selection,"UP%luS %.13s",(unsigned long)(e->uptime/1000),event_names[e->type]);
   }
  }
  row(f,7,"L/R EVENT OK DETAILS");break;
 }
 case UI_EVENT:{
  row(f,0,"EVENT DETAILS");const MenuEvent *e=&m->event_detail;
  row(f,1,"%s",event_names[e->type]);
  if(e->dated){row(f,2,"%04u-%02u-%02u",e->date.year,e->date.month,e->date.day);row(f,3,"%02u:%02u:%02u",e->date.hour,e->date.minute,e->date.second);}
  else{row(f,2,"CLOCK WAS UNTRUSTED");row(f,3,"UPTIME %lu S",(unsigned long)(e->uptime/1000));}
  if(e->mask)row(f,4,"SLOT MASK %u",e->mask);
  row(f,5,"NO DOSE CONFIRMATION");row(f,6,"%s",m->persist_dirty?"SAVE PENDING":l->storage_ok?"SAVED TO FLASH":"STORAGE ERROR");row(f,7,"L/R EVENT BACK LIST");break;
 }
 case UI_SETTINGS:
  row(f,0,"SETTINGS");row(f,1,"%s",m->editing?"BACK CANCEL VALUE":m->persist_dirty?"SAVING CHANGES":l->storage_ok?"CHANGES AUTO SAVE":"STORAGE ERROR");
  field_row(f,2,m->settings_selection==0,"OPEN TONE %s",m->settings.sound?"ON":"OFF");
  field_row(f,3,m->settings_selection==1,"TONE LENGTH %u MS",m->editing&&m->settings_selection==1?m->setting_draft:m->settings.duration_ms);
  field_row(f,4,m->settings_selection==2,"BRIGHTNESS %u/10",m->editing&&m->settings_selection==2?m->setting_draft:m->settings.brightness);
  field_row(f,5,m->settings_selection==3,"TEST ALARM");field_row(f,6,m->settings_selection==4,"DIAGNOSTICS");
  row(f,7,"%s",m->editing?"L/R VALUE OK SAVE":"L/R MOVE OK SELECT");break;
 case UI_DIAGNOSTICS:
  row(f,0,"DIAGNOSTICS");row(f,1,"RTC OSF %lu CHECK %lu/3",(unsigned long)l->rtc_osf,(unsigned long)l->clock_progress);
  row(f,2,"I2C ERR O%lu R%lu",(unsigned long)l->oled_errors,(unsigned long)l->rtc_errors);
  row(f,3,"STORAGE %s",l->storage_ok?"OK":"ERROR");row(f,4,"ESP LINK %s",l->link_up?"UP":"DOWN");
  row(f,5,"UPTIME %lu S",(unsigned long)(l->uptime/1000));row(f,6,"FW 1.1.0");row(f,7,"BACK SETTINGS");break;
 case UI_NOTICE:
  row(f,0,"RESULT");row(f,3,"%s",m->notice);row(f,4,"%s",m->notice_detail);row(f,7,"OK OR BACK RETURN");break;
 }
}
