#include "screen.h"
#include "upcoming.h"
#include "calendar.h"
#include <string.h>
#include <stdio.h>
static const uint8_t font_digits[10][5]={
 {0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},
 {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},
 {0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
 {0x3c,0x4a,0x49,0x49,0x30},{1,0x71,9,5,3},
 {0x36,0x49,0x49,0x49,0x36},{6,0x49,0x49,0x29,0x1e}};
static const uint8_t font_letters[26][5]={
 {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},
 {0x3e,0x41,0x41,0x41,0x22},{0x7f,0x41,0x41,0x22,0x1c},
 {0x7f,0x49,0x49,0x49,0x41},{0x7f,9,9,9,1},
 {0x3e,0x41,0x49,0x49,0x7a},{0x7f,8,8,8,0x7f},
 {0,0x41,0x7f,0x41,0},{0x20,0x40,0x41,0x3f,1},
 {0x7f,8,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
 {0x7f,2,0x0c,2,0x7f},{0x7f,4,8,0x10,0x7f},
 {0x3e,0x41,0x41,0x41,0x3e},{0x7f,9,9,9,6},
 {0x3e,0x41,0x51,0x21,0x5e},{0x7f,9,0x19,0x29,0x46},
 {0x46,0x49,0x49,0x49,0x31},{1,1,0x7f,1,1},
 {0x3f,0x40,0x40,0x40,0x3f},{0x1f,0x20,0x40,0x20,0x1f},
 {0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,8,0x14,0x63},
 {7,8,0x70,8,7},{0x61,0x51,0x49,0x45,0x43}};
static uint8_t *canvas;
static void pixel(int x,int y,int on){if(x<0||x>=128||y<0||y>=64)return;uint8_t *p=&canvas[(y/8)*128+x];uint8_t bit=(uint8_t)(1u<<(y&7));if(on)*p|=bit;else *p&=(uint8_t)~bit;}
static void box(int x,int y,int w,int h,int on){for(int yy=y;yy<y+h;yy++)for(int xx=x;xx<x+w;xx++)pixel(xx,yy,on);}
static void line(int x,int y,int x1,int y1,int on){int dx=x1>x?x1-x:x-x1,sx=x<x1?1:-1,dy=y1>y?y-y1:y1-y,sy=y<y1?1:-1,err=dx+dy;for(;;){pixel(x,y,on);if(x==x1&&y==y1)break;int e=2*err;if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}}}
static void glyph(char c,uint8_t g[5]){
 memset(g,0,5);if(c>='a'&&c<='z')c=(char)(c-'a'+'A');
 if(c>='0'&&c<='9'){memcpy(g,font_digits[c-'0'],5);return;}
 if(c>='A'&&c<='Z'){memcpy(g,font_letters[c-'A'],5);return;}
 switch(c){case ':':g[2]=0x36;break;case '-':memset(g,8,5);break;case '+':memset(g,8,5);g[2]=0x3e;break;
 case '.':g[2]=0x60;break;case '/':g[0]=32;g[1]=16;g[2]=8;g[3]=4;g[4]=2;break;
 case '>':g[1]=0x41;g[2]=0x22;g[3]=0x14;g[4]=8;break;case '<':g[0]=8;g[1]=0x14;g[2]=0x22;g[3]=0x41;break;
 case '!':g[2]=0x5f;break;case '?':g[0]=2;g[1]=1;g[2]=0x51;g[3]=9;g[4]=6;break;case '=':memset(g,0x14,5);break;default:break;}
}
static int width(const char *s,unsigned scale,int bold){return (int)strlen(s)*(6*(int)scale+bold)-(int)scale;}
static void text(int x,int y,const char *s,unsigned scale,int bold,int on){
 for(;*s;s++,x+=6*(int)scale+bold){if(x+(int)(5*scale+bold)>128)break;uint8_t g[5];glyph(*s,g);
  for(unsigned col=0;col<5;col++)for(unsigned row=0;row<7;row++)if(g[col]&(1u<<row))box(x+(int)(col*scale),y+(int)(row*scale),(int)scale+bold,(int)scale,on);
 }
}
static void label(int x,int y,const char *s){text(x,y,s,1,0,1);}
static void strong(int x,int y,const char *s){text(x,y,s,1,1,1);}
static void right(int end,int y,const char *s){label(end-width(s,1,0),y,s);}
static void center(int y,const char *s,unsigned scale){text((128-width(s,scale,0))/2,y,s,scale,0,1);}
static void header(const char *s,unsigned selected,unsigned count){
 box(0,0,128,10,1);text(4,1,s,1,1,0);
 if(count){char b[8];snprintf(b,sizeof b,"%u/%u",selected%99+1,count%100);text(125-width(b,1,0),1,b,1,0,0);}
}
static void footer(const char *left,const char *rightText){line(2,54,125,54,1);label(3,57,left);right(125,57,rightText);}
static void scroll(unsigned selected,unsigned count){if(count<=3)return;box(126,12,1,39,1);unsigned h=39*3/count;if(h<4)h=4;unsigned pos=(39-h)*selected/(count-1);box(125,12+(int)pos,3,(int)h,1);}
static unsigned first(unsigned selected,unsigned count){if(count<=3||selected<2)return 0;return selected+1<count?selected-1:count-3;}
static void entry(unsigned y,const char *name,const char *value,int selected,int editing){
 if(selected)box(2,(int)y-2,121,12,1);
 text(6,(int)y,name,1,selected, !selected);
 if(value&&value[0]){int x=119-width(value,1,0);text(x,(int)y,value,1,0,!selected);if(editing)box(x,(int)y+8,width(value,1,0),1,!selected);}
}
static const char *const weekdays[]={"MON","TUE","WED","THU","FRI","SAT","SUN"};
static void home(const Menu *m,const MenuLive *l){
 label(3,0,"MEDBOX");right(125,0,l->lid_open?"LID OPEN":"LID CLOSED");char b[32];
 if(l->clock_valid&&calendar_valid(&l->date)){
  snprintf(b,sizeof b,"%02u:%02u",l->date.hour,l->date.minute);text(3,12,b,2,0,1);
  snprintf(b,sizeof b,"%02u",l->date.second);label(65,19,b);
  right(125,12,weekdays[calendar_weekday(&l->date)]);snprintf(b,sizeof b,"%02u/%02u",l->date.day,l->date.month);right(125,23,b);
 }else{text(3,12,"--:--",2,0,1);right(125,19,"SET TIME");}
 line(3,32,124,32,1);
 NextReminder next;
 if(!l->storage_ok){strong(4,36,"STORAGE ERROR");label(4,46,"CHECK SETTINGS");}
 else if(!l->clock_trusted){strong(4,36,l->rtc_osf||l->clock_progress>=3||!l->clock_valid?"SET THE CLOCK":"CHECKING CLOCK");label(4,46,"REMINDERS PAUSED");}
 else if(upcoming_reminder(m->schedules,&l->date,&next)){
  MenuDate due=calendar_date(next.epoch);snprintf(b,sizeof b,"NEXT %02u:%02u",due.hour,due.minute);strong(4,36,b);
  right(125,36,next.days_away==0?"TODAY":next.days_away==1?"TMRW":weekdays[calendar_weekday(&due)]);
  unsigned count=0;for(unsigned i=0;i<MENU_SLOTS;i++)count+=(next.mask>>i)&1;
  unsigned show=(l->uptime/3000)%count,slot=next.slot;
  for(unsigned i=0;i<MENU_SLOTS;i++)if(next.mask&(1u<<i)){if(!show){slot=i;break;}show--;}
  strong(4,46,m->schedules[slot].name);
 }else{strong(4,36,"NO ALARMS ENABLED");label(4,46,"ADD ONE IN SCHEDULES");}
 label(3,57,"OK MENU");right(125,57,!l->storage_ok?"SAVE ERROR":l->clock_trusted?"CLOCK OK":l->rtc_osf||l->clock_progress>=3?"SYNC CLOCK":"CHECK RTC");
}
static void main_menu(const Menu *m){
 const char *names[]={"SET CLOCK","SCHEDULES","HISTORY","SETTINGS"};header("MENU",m->selection,4);
 for(unsigned i=0;i<4;i++){unsigned y=13+i*10;if(i==m->selection)box(2,(int)y-1,124,9,1);text(8,(int)y,names[i],1,i==m->selection,i!=m->selection);}
 footer("BACK HOME","OK OPEN");
}
static void schedule_list(const Menu *m){
 const MenuSchedule *s=&m->schedules[m->slot];char b[24];header("SCHEDULES",m->slot,MENU_SLOTS);
 strong(4,14,s->name);snprintf(b,sizeof b,"%02u:%02u",s->hour,s->minute);text(4,27,b,2,0,1);right(124,29,s->enabled?"ON":"OFF");
 if(s->days==127)right(124,40,"DAILY");else{char d[8];for(unsigned i=0;i<7;i++)d[i]=s->days&(1u<<i)?"MTWTFSS"[i]:'-';d[7]=0;right(124,44,d);}
 // Position marks communicate that LEFT/RIGHT changes the selected slot.
 for(unsigned i=0;i<MENU_SLOTS;i++){int x=5+(int)i*8;box(x,48,5,i==m->slot?3:1,1);}
 footer("L/R SLOT","OK EDIT");
}
static void clock_editor(const Menu *m){
 const char *names[]={"YEAR","MONTH","DAY","HOUR","MINUTE","SECOND","SAVE CLOCK"};
 const MenuDate *d=&m->clock_draft;unsigned values[]={d->year,d->month,d->day,d->hour,d->minute,d->second};
 header(m->editing?"EDIT VALUE":"SET CLOCK",m->field,7);unsigned start=first(m->field,7);
 for(unsigned i=start;i<start+3&&i<7;i++){char b[8]="";if(i<6)snprintf(b,sizeof b,i==0?"%04u":"%02u",values[i]);entry(15+(i-start)*14,names[i],b,i==m->field,m->editing&&i==m->field);}
 scroll(m->field,7);footer(m->editing?"BACK CANCEL":"BACK MENU",m->editing?"OK DONE":"OK EDIT");
}
static void schedule_editor(const Menu *m){
 const MenuSchedule *s=&m->schedule_draft;const char *names[]={"NAME","HOUR","MINUTE","DAYS","ENABLED","SAVE ALARM"};char title[20];snprintf(title,sizeof title,"EDIT SLOT %u",m->slot+1);header(title,m->field,6);
 unsigned start=first(m->field,6);
 for(unsigned i=start;i<start+3&&i<6;i++){char b[18]="";const char *name=names[i];int editing=m->editing&&i==m->field;
  switch(i){case 0:name=s->name;break;case 1:snprintf(b,sizeof b,"%02u",editing?m->setting_draft:s->hour);break;case 2:snprintf(b,sizeof b,"%02u",editing?m->setting_draft:s->minute);break;
  case 3:for(unsigned d=0;d<7;d++)b[d]=s->days&(1u<<d)?"MTWTFSS"[d]:'-';b[7]=0;break;case 4:snprintf(b,sizeof b,"%s",(editing?m->setting_draft:s->enabled)?"ON":"OFF");break;default:break;}
  entry(15+(i-start)*14,name,b,i==m->field,editing);
 }
 scroll(m->field,6);footer(m->editing?"BACK CANCEL":"BACK DISCARD",m->editing?"OK DONE":"OK EDIT");
}
static void name_editor(const Menu *m){
 header("MEDICINE NAME",0,0);label(4,14,m->editing?"CHOOSE A LETTER":"CHOOSE A POSITION");
 text(8,28,m->schedule_draft.name,1,1,1);
 if(m->name_cursor<16){box(8+m->name_cursor*7,37,6,2,1);}
 if(m->name_cursor==16){box(43,41,42,11,1);text(52,43,"DONE",1,0,0);}else center(44,"OK EDIT / DONE",1);
 footer("BACK",m->editing?"L/R LETTER":"L/R CURSOR");
}
static void days_editor(const Menu *m){
 header("REPEAT DAYS",m->day_cursor,8);unsigned start=first(m->day_cursor,8);
 for(unsigned i=start;i<start+3&&i<8;i++)entry(15+(i-start)*14,i==7?"DONE":weekdays[i],i==7?"":m->schedule_draft.days&(1u<<i)?"ON":"OFF",i==m->day_cursor,0);
 scroll(m->day_cursor,8);footer("BACK","OK TOGGLE");
}
static const MenuEvent *history_at(const Menu *m,unsigned i){return &m->history[(m->history_head+MENU_HISTORY_SIZE-1-i)%MENU_HISTORY_SIZE];}
static const char *event_title(unsigned type){
 const char *names[]={"BOX STARTED","LID OPENED","LID CLOSED","CLOCK SET","ALARM EDITED","SETTINGS SAVED","REMINDER DUE","LID ACKNOWLEDGED","REMINDER MISSED","DOWNTIME CHECK","ALARM QUIET","CLOCK GAP","TEST ALARM"};return type<=EVENT_TEST_ALARM?names[type]:"DEVICE EVENT";
}
static void history_list(const Menu *m){
 header("HISTORY",m->history_selection,m->history_count);
 if(!m->history_count){center(26,"NO EVENTS YET",1);footer("BACK MENU","");return;}
 const MenuEvent *e=history_at(m,m->history_selection);strong(4,15,event_title(e->type));char b[24];
 if(e->dated){snprintf(b,sizeof b,"%02u:%02u:%02u",e->date.hour,e->date.minute,e->date.second);text(4,28,b,2,0,1);snprintf(b,sizeof b,"%02u/%02u/%04u",e->date.day,e->date.month,e->date.year);label(4,46,b);}
 else{label(4,30,"TIME NOT VERIFIED");snprintf(b,sizeof b,"UPTIME %lu S",(unsigned long)(e->uptime/1000));label(4,44,b);}
 footer("L/R EVENT","OK DETAIL");
}
static void settings_list(const Menu *m,const MenuLive *l){
 const char *names[]={"OPEN TONE","TONE TIME","BRIGHTNESS","TEST ALARM","DIAGNOSTICS"};header(m->persist_dirty?"SAVING...":"SETTINGS",m->settings_selection,5);
 unsigned start=first(m->settings_selection,5);
 for(unsigned i=start;i<start+3&&i<5;i++){char b[12]="";int edit=m->editing&&i==m->settings_selection;
  if(i==0)snprintf(b,sizeof b,"%s",m->settings.sound?"ON":"OFF");
  if(i==1)snprintf(b,sizeof b,"%uMS",edit?m->setting_draft:m->settings.duration_ms);
  if(i==2)snprintf(b,sizeof b,"%u/10",edit?m->setting_draft:m->settings.brightness);
  entry(15+(i-start)*14,names[i],b,i==m->settings_selection,edit);
 }
 scroll(m->settings_selection,5);footer(!l->storage_ok?"SAVE ERROR":m->editing?"BACK CANCEL":"BACK MENU",m->editing?"OK SAVE":"OK SELECT");
}
static void alarm_screen(const Alarm *a,const MenuLive *l){
 header(a->test_active?"TEST ALARM":"MEDICINE DUE",0,0);char b[24];
 if(a->test_active){center(16,"10 SECOND",1);center(28,"CHIME",2);center(46,"REPEATS UNTIL OPEN",1);}
 else{
  const AlarmGroup *g=&a->saved.groups[a->selected];MenuDate d=calendar_date(g->due);snprintf(b,sizeof b,"%02u:%02u",d.hour,d.minute);text(4,14,b,2,0,1);
  unsigned n=0;for(unsigned i=0;i<MENU_SLOTS;i++)n+=(g->mask>>i)&1;unsigned wanted=n?(l->uptime/3000)%n:0;
  for(unsigned i=0;i<MENU_SLOTS;i++)if(g->mask&(1u<<i)){if(!wanted){strong(4,34,g->names[i]);break;}wanted--;}
  label(4,46,g->armed?"OPEN LID TO ACK":"CLOSE THEN OPEN LID");
 }
 if(a->test_active)footer(l->lid_open?"CLOSE LID":"OPEN LID",l->lid_open?"THEN OPEN":"TO STOP");
 else footer(l->lid_open?"LID OPEN":"LID CLOSED","L/R GROUP");
}
void screen_boot(uint8_t p[1024],uint32_t elapsed){
 canvas=p;memset(p,0,1024);center(3,"SMART MEDICINE BOX",1);
 // Capsule mark with a drawn seam, followed by the brand reveal.
 int t=(int)(elapsed>800?800:elapsed);int edge=8+24*t/800;
 static const uint8_t arc[15]={0,3,4,5,6,6,6,7,6,6,6,5,4,3,0};
 for(int y=19;y<=33;y++){int left=15-arc[y-19],rightEdge=25+arc[y-19];if(left<=edge)pixel(left,y,1);if(rightEdge<=edge)pixel(rightEdge,y,1);}
 for(int x=15;x<=25&&x<=edge;x++){pixel(x,19,1);pixel(x,33,1);}
 if(t>350)line(16,32,24,20,1);
 if(t>600){box(12,23,2,2,1);box(15,28,2,2,1);}
 char brand[7]="MEDBOX";unsigned count=elapsed<300?0:(elapsed-300)/110;if(count>6)count=6;brand[count]=0;text(41,19,brand,2,0,1);
 if(elapsed>=1000)center(42,"YOUR DAILY ROUTINE",1);
 unsigned progress=elapsed>SCREEN_BOOT_MS?112:112*elapsed/SCREEN_BOOT_MS;box(8,55,(int)progress,2,1);pixel(8,59,1);pixel(119,59,1);
}
void screen_render(uint8_t p[1024],const Menu *m,const MenuLive *l,const Alarm *a){
 canvas=p;memset(p,0,1024);
 if(a->test_active||(a->show&&alarm_active_count(a))){alarm_screen(a,l);return;}
 switch(m->view){
 case UI_HOME:home(m,l);break;case UI_MAIN:main_menu(m);break;case UI_SCHEDULES:schedule_list(m);break;case UI_CLOCK:clock_editor(m);break;
 case UI_SCHEDULE:schedule_editor(m);break;case UI_NAME:name_editor(m);break;case UI_DAYS:days_editor(m);break;case UI_HISTORY:history_list(m);break;case UI_SETTINGS:settings_list(m,l);break;
 case UI_NOTICE:header(m->persist_wait?"SAVING":"RESULT",0,0);center(22,m->notice,1);center(37,m->notice_detail,1);footer(m->persist_wait?"PLEASE WAIT":"BACK",m->persist_wait?"":"OK RETURN");break;
 case UI_CLOCK_WAIT:header("SET CLOCK",0,0);center(20,"SETTING TIME",1);center(35,"VERIFYING RTC",1);footer("PLEASE WAIT","");break;
 default:{MenuFrame f;menu_render(m,l,&f);for(unsigned i=0;i<8;i++)label(2,(int)i*8,f.rows[i]);box(0,0,128,8,1);text(2,0,f.rows[0],1,0,0);break;}
 }
}
