#include "alarm.h"
#include <stdio.h>
#include <string.h>
static void emit(Alarm *a,unsigned type,unsigned mask,uint32_t due){
 unsigned next=(a->write+1)%16;
 if(next==a->read){a->read=(a->read+1)%16;a->lost_events++;}
 a->events[a->write]=(AlarmEvent){due,(uint8_t)type,(uint8_t)mask};a->write=(uint8_t)next;a->dirty=1;a->revision++;
}
int alarm_event(Alarm *a,AlarmEvent *e){if(a->read==a->write)return 0;*e=a->events[a->read];a->read=(a->read+1)%16;return 1;}
unsigned alarm_active_count(const Alarm *a){unsigned n=0;for(unsigned i=0;i<MENU_SLOTS;i++)n+=a->saved.groups[i].active;return n;}
static void select_active(Alarm *a,int direction){
 a->name_page=0;
 for(unsigned n=0;n<MENU_SLOTS;n++){
  a->selected=(uint8_t)((a->selected+MENU_SLOTS+direction)%MENU_SLOTS);
  if(a->saved.groups[a->selected].active)return;
 }
}
void alarm_init(Alarm *a,int restoring){memset(a,0,sizeof *a);a->recovering=(uint8_t)restoring;}
static uint32_t latest_due(const MenuSchedule *s,uint32_t now){
 uint32_t day=now/86400;unsigned dow=(day+5)%7;
 for(unsigned back=0;back<7&&back<=day;back++){
  uint32_t due=(day-back)*86400u+s->hour*3600u+s->minute*60u;
  if(due<=now&&(s->days&(1u<<((dow+7-back)%7))))return due;
 }
 return 0;
}
static void add_due(Alarm *a,unsigned slot,uint32_t due,const MenuSchedule slots[MENU_SLOTS],const MenuLive *l){
 if(!alarm_active_count(a))a->sound_started=l->uptime;
 unsigned group=MENU_SLOTS;
 for(unsigned i=0;i<MENU_SLOTS;i++)if(a->saved.groups[i].active&&a->saved.groups[i].due==due){group=i;break;}
 if(group==MENU_SLOTS)for(unsigned i=0;i<MENU_SLOTS;i++)if(!a->saved.groups[i].active){group=i;break;}
 if(group==MENU_SLOTS){emit(a,ALARM_DOWNTIME,1u<<slot,due);return;}
 AlarmGroup *g=&a->saved.groups[group];
 if(!g->active){memset(g,0,sizeof *g);g->active=1;g->due=due;g->started=l->uptime;g->window_ms=(ALARM_WINDOW_SECONDS-(calendar_seconds(&l->date)-due))*1000;g->armed=(uint8_t)!l->lid_open;}
 g->mask|=(uint8_t)(1u<<slot);memcpy(g->names[slot],slots[slot].name,17);
 a->selected=(uint8_t)group;a->name_page=0;a->show=1;a->test_active=0;emit(a,ALARM_DUE,1u<<slot,due);
}
void alarm_poll(Alarm *a,const MenuLive *l,const MenuSchedule slots[MENU_SLOTS]){
 if(a->test_active&&l->lid_open&&!a->previous_open){a->test_active=0;a->revision++;}
 uint32_t now=l->clock_valid?calendar_seconds(&l->date):0;
 if(a->recovering){
  for(unsigned i=0;i<MENU_SLOTS;i++){
   AlarmGroup *g=&a->saved.groups[i];if(!g->active)continue;
   g->muted=0;g->armed=(uint8_t)!l->lid_open;g->started=l->uptime;
   a->selected=(uint8_t)i;a->show=1;a->sound_started=l->uptime;
  }
  a->recovering=0;a->dirty=1;
 }
 for(unsigned i=0;i<MENU_SLOTS;i++){
  AlarmGroup *g=&a->saved.groups[i];if(!g->active)continue;
  if(!l->lid_open)g->armed=1;
 }
 if(l->lid_open&&!a->previous_open){
  for(unsigned i=0;i<MENU_SLOTS;i++){AlarmGroup *g=&a->saved.groups[i];
   if(g->active&&g->armed){g->active=0;emit(a,ALARM_LID_ACK,g->mask,g->due);}
  }
 }
 a->previous_open=(uint8_t)l->lid_open;
 if(!alarm_active_count(a))a->show=0;
 if(!l->clock_trusted)return;
 if(!a->saved.cursor){a->saved.cursor=now>59?now-59:0;a->dirty=1;}
 if(now<a->saved.cursor){a->saved.cursor=now;emit(a,ALARM_CLOCK_GAP,0,now);return;}
 if(now-a->saved.cursor>86400)emit(a,ALARM_CLOCK_GAP,0,now);
 for(unsigned slot=0;slot<MENU_SLOTS;slot++){
  const MenuSchedule *s=&slots[slot];if(!s->enabled||!s->days)continue;
  uint32_t due=latest_due(s,now);
  if(!due||due<=a->saved.cursor||due<=a->saved.last_due[slot])continue;
  a->saved.last_due[slot]=due;
  if(now-due<ALARM_WINDOW_SECONDS)add_due(a,slot,due,slots,l);
  else emit(a,ALARM_DOWNTIME,1u<<slot,due);
 }
 a->saved.cursor=now;
 if(a->show&&!a->saved.groups[a->selected].active&&alarm_active_count(a))select_active(a,1);
}
void alarm_button(Alarm *a,enum MenuButton b){
 if(a->test_active)return;
 if(!a->show||!alarm_active_count(a))return;
 if(b==MENU_LEFT||b==MENU_RIGHT){select_active(a,b==MENU_LEFT?-1:1);a->revision++;}
 if(b==MENU_OK){unsigned count=0;for(unsigned i=0;i<MENU_SLOTS;i++)count+=(a->saved.groups[a->selected].mask>>i)&1;
  a->name_page=(a->name_page+1)%((count+2)/3);a->revision++;}

}
void alarm_test(Alarm *a,uint32_t tick){
 if(alarm_active_count(a)){a->show=1;if(!a->saved.groups[a->selected].active)select_active(a,1);}
 else{a->test_active=1;a->test_started=tick;}
 a->revision++;
}
unsigned alarm_frequency(const Alarm *a,uint32_t now){
 int sounding=a->test_active;
 if(!a->recovering)for(unsigned i=0;i<MENU_SLOTS;i++)if(a->saved.groups[i].active&&!a->saved.groups[i].muted)sounding=1;
 if(!sounding)return 0;
 /* Twenty 500-ms beats make a full ten-second melody; repeat without a long gap. */
 static const uint16_t notes[20]={1568,1976,2349,1976,1568,1976,2637,2349,1976,1568,1319,1568,1976,2349,2637,2349,1976,1568,1976,1568};
 uint32_t phase=(now-(a->test_active?a->test_started:a->sound_started))%10000;
 return phase%500<450?notes[phase/500]:0;
}
void alarm_render(const Alarm *a,const MenuLive *l,MenuFrame *f){
 (void)l;memset(f,0,sizeof *f);f->selected_row=-1;f->character_column=-1;
 if(a->test_active){
  snprintf(f->rows[0],22,"TEST ALARM");snprintf(f->rows[2],22,"10 SECOND CHIME");
  snprintf(f->rows[3],22,"REPEATS UNTIL OPEN");
  snprintf(f->rows[5],22,"NO MEDICINE EVENT");snprintf(f->rows[7],22,"CLOSE THEN OPEN LID");return;
 }
 const AlarmGroup *g=&a->saved.groups[a->selected];MenuDate due=calendar_date(g->due);
 snprintf(f->rows[0],22,"MEDICINE REMINDER");snprintf(f->rows[1],22,"DUE %02u:%02u GROUP %u",due.hour,due.minute,alarm_active_count(a));
 unsigned count=0;for(unsigned i=0;i<MENU_SLOTS;i++)if(g->mask&(1u<<i))count++;
 unsigned page=a->name_page%((count+2)/3),index=0,row=2;
 for(unsigned i=0;i<MENU_SLOTS;i++)if(g->mask&(1u<<i)){if(index/3==page&&row<5)snprintf(f->rows[row++],22,"%s",g->names[i]);index++;}
 snprintf(f->rows[5],22,"%s",g->armed?"OPEN LID TO LOG":"CLOSE THEN OPEN LID");
 snprintf(f->rows[6],22,"OK NAMES L/R GROUP");snprintf(f->rows[7],22,"REPEATS UNTIL OPEN");
}
