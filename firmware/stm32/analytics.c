#include "analytics.h"
#include "control.h"
#include <limits.h>
#include <string.h>
_Static_assert(sizeof(DoseRecord)==48,"Dose wire layout changed");
_Static_assert(sizeof(SlotStats)==40,"Slot statistics layout changed");
_Static_assert(sizeof(OpenInterval)==20,"Interval wire layout changed");
static void increment(uint32_t *n){if(*n<UINT32_MAX)(*n)++;}
static uint32_t add_ms(uint32_t a,uint32_t b){return UINT32_MAX-a<b?UINT32_MAX:a+b;}
static void changed(Analytics *a,int critical){if(++a->data.generation==0)a->data.generation=1;a->dirty=1;if(critical)a->critical=1;}
static DoseRecord *find(AnalyticsData *d,unsigned slot,uint32_t due){
 for(unsigned i=0;i<d->count;i++){DoseRecord *r=&d->records[(d->head+DOSE_RECORDS-1-i)%DOSE_RECORDS];if(r->slot==slot&&r->due==due)return r;}return 0;
}
const DoseRecord *analytics_record(const AnalyticsData *d,unsigned index){return index<d->count?&d->records[(d->head+DOSE_RECORDS-1-index)%DOSE_RECORDS]:0;}
const OpenInterval *analytics_interval(const AnalyticsData *d,unsigned index){return index<d->interval_count?&d->intervals[(d->interval_head+OPEN_INTERVALS-1-index)%OPEN_INTERVALS]:0;}
static void boundary(Analytics *a,uint32_t due){
 AnalyticsData *d=&a->data;if(due==d->boundary)return;
 if(d->boundary&&due<d->boundary){d->interval_partial=1;return;}
 if(d->boundary){
  d->intervals[d->interval_head]=(OpenInterval){d->boundary,due,d->interval_opens,d->interval_extra,d->interval_partial};
  d->interval_head=(d->interval_head+1)%OPEN_INTERVALS;if(d->interval_count<OPEN_INTERVALS)d->interval_count++;
 }
 d->previous_opens=d->interval_opens;d->previous_extra=d->interval_extra;d->previous_partial=d->interval_partial;
 d->boundary=due;d->interval_opens=0;d->interval_extra=0;d->interval_partial=0;
}
static DoseRecord *create(Analytics *a,unsigned slot,uint32_t due,const char *name,unsigned flags){
 AnalyticsData *d=&a->data;DoseRecord *existing=find(d,slot,due);if(existing)return existing;
 DoseRecord *r=&d->records[d->head];if(d->count==DOSE_RECORDS)increment(&d->lost_records);
 memset(r,0,sizeof *r);if(!d->next_id)d->next_id=1;r->id=d->next_id++;r->slot=(uint8_t)slot;r->due=due;r->flags=(uint8_t)flags;
 memcpy(r->name,name,16);r->name[16]=0;
 r->interval_opens=(uint16_t)(d->previous_opens>65535?65535:d->previous_opens);r->interval_extra=(uint16_t)(d->previous_extra>65535?65535:d->previous_extra);
 d->head=(d->head+1)%DOSE_RECORDS;if(d->count<DOSE_RECORDS)d->count++;
 increment(&d->slots[slot].due);changed(a,1);return r;
}
void analytics_init(Analytics *a,const AnalyticsData *saved,const Alarm *alarm,uint32_t tick){
 memset(a,0,sizeof *a);if(saved)a->data=*saved;
 a->tick=a->checkpoint=tick;if(!a->data.next_id){a->data.next_id=1;a->data.interval_partial=1;changed(a,0);}
 a->data.interval_partial=1;
 for(unsigned i=0;i<a->data.count;i++){DoseRecord *r=&a->data.records[i];if(r->flags&DOSE_PENDING){r->flags|=DOSE_INTERRUPTED;changed(a,1);}}
 for(unsigned g=0;g<MENU_SLOTS;g++)if(alarm->saved.groups[g].active){const AlarmGroup *group=&alarm->saved.groups[g];
  for(unsigned slot=0;slot<MENU_SLOTS;slot++)if((group->mask&(1u<<slot))&&!find(&a->data,slot,group->due))create(a,slot,group->due,group->names[slot],DOSE_PENDING|DOSE_INTERRUPTED|DOSE_IMPORTED|DOSE_INTERVAL_PARTIAL);
 }
}
void analytics_tick(Analytics *a,const MenuLive *l){
 uint32_t delta=l->uptime-a->tick;a->tick=l->uptime;int active=0;
 if(!a->data.tracking_since&&l->clock_trusted){a->data.tracking_since=calendar_seconds(&l->date);changed(a,0);}
 for(unsigned i=0;i<a->data.count;i++){DoseRecord *r=&a->data.records[i];if(r->flags&DOSE_PENDING){r->ring_ms=add_ms(r->ring_ms,delta);active=1;}}
 if(active&&(uint32_t)(l->uptime-a->checkpoint)>=60000){a->checkpoint=l->uptime;changed(a,0);}
}
void analytics_lid(Analytics *a,int reminder,const MenuLive *l){
 increment(&a->data.total_opens);increment(&a->data.interval_opens);
 if(!reminder){increment(&a->data.total_extra);increment(&a->data.interval_extra);}
 if(!l->clock_trusted){increment(&a->data.untrusted_opens);a->data.interval_partial=1;}
 changed(a,0);
}
void analytics_gap(Analytics *a){
 int modified=!a->data.interval_partial;a->data.interval_partial=1;
 for(unsigned i=0;i<a->data.count;i++)if((a->data.records[i].flags&DOSE_PENDING)&&!(a->data.records[i].flags&DOSE_TIME_UNCERTAIN)){a->data.records[i].flags|=DOSE_TIME_UNCERTAIN;modified=1;}
 if(modified)changed(a,0);
}
void analytics_alarm(Analytics *a,const AlarmEvent *e,const MenuSchedule slots[MENU_SLOTS],const MenuLive *l){
 if(e->type==ALARM_CLOCK_GAP){analytics_gap(a);return;}
 if(e->type==ALARM_DUE||e->type==ALARM_DOWNTIME){
  boundary(a,e->due);
  for(unsigned slot=0;slot<MENU_SLOTS;slot++)if(e->mask&(1u<<slot))create(a,slot,e->due,slots[slot].name,(e->type==ALARM_DUE?DOSE_PENDING:DOSE_DOWNTIME|DOSE_TIME_UNCERTAIN)|(a->data.previous_partial?DOSE_INTERVAL_PARTIAL:0));
 }else if(e->type==ALARM_LID_ACK){
  for(unsigned slot=0;slot<MENU_SLOTS;slot++)if(e->mask&(1u<<slot)){
   SlotStats *s=&a->data.slots[slot];DoseRecord *r=find(&a->data,slot,e->due);
   if(r&&(r->flags&DOSE_OPENED))continue;
   increment(&s->opened);if(!r)continue;
   r->flags=(uint8_t)((r->flags&~DOSE_PENDING)|DOSE_OPENED);
   if(l->clock_trusted){uint32_t now=calendar_seconds(&l->date);r->opened=now;
    if(now>=r->due&&now-r->due<=UINT32_MAX/1000&&!(r->flags&(DOSE_TIME_UNCERTAIN|DOSE_IMPORTED))){r->response_ms=(now-r->due)*1000;r->flags|=DOSE_RESPONSE_VALID;s->response_ms+=r->response_ms;increment(&s->response_count);}}
   if(!(r->flags&(DOSE_INTERRUPTED|DOSE_IMPORTED))){s->ring_ms+=r->ring_ms;increment(&s->ring_count);}
   changed(a,1);
  }
 }
}
int analytics_report(Analytics *a,uint32_t id,unsigned report,unsigned previous,const MenuLive *l){
 if(!id||report>REPORT_NOT_TAKEN)return 0;
 for(unsigned i=0;i<a->data.count;i++){DoseRecord *r=&a->data.records[i];if(r->id!=id)continue;
  if(r->report==report)return 1;
  if(r->report!=previous)return -1;
  SlotStats *s=&a->data.slots[r->slot];if(r->report==REPORT_TAKEN&&s->taken)s->taken--;if(r->report==REPORT_NOT_TAKEN&&s->not_taken)s->not_taken--;
  r->report=(uint8_t)report;r->reported_at=report&&l->clock_trusted?calendar_seconds(&l->date):0;
  if(report==REPORT_TAKEN)increment(&s->taken);
  if(report==REPORT_NOT_TAKEN)increment(&s->not_taken);
  changed(a,1);return 1;
 }
 return 0;
}
int analytics_valid(const AnalyticsData *d){
 if(d->head>=DOSE_RECORDS||d->count>DOSE_RECORDS||d->interval_head>=OPEN_INTERVALS||d->interval_count>OPEN_INTERVALS||d->interval_partial>1||d->boundary>3155759999u||d->tracking_since>3155759999u||d->total_extra>d->total_opens||d->interval_extra>d->interval_opens)return 0;
 for(unsigned i=0;i<d->count;i++){const DoseRecord *r=analytics_record(d,i);if(!r->id||r->slot>=8||r->report>2||!memchr(r->name,0,17)||r->due>3155759999u||r->opened>3155759999u||r->reported_at>3155759999u||r->interval_extra>r->interval_opens)return 0;}
 for(unsigned i=0;i<d->interval_count;i++){const OpenInterval *r=analytics_interval(d,i);if(r->start>r->end||r->end>3155759999u||r->extra>r->opens||r->partial>1)return 0;}
 for(unsigned i=0;i<8;i++){const SlotStats *s=&d->slots[i];if(s->opened>s->due||s->taken>s->due||s->not_taken>s->due||s->taken>s->due-s->not_taken||s->response_count>s->opened||s->ring_count>s->opened)return 0;}
 return 1;
}
unsigned analytics_packet(uint8_t *p,const AnalyticsData *d,unsigned mode,unsigned index,uint32_t generation){
 p[0]=(uint8_t)mode;p[1]=(uint8_t)index;ct_put32(p+2,d->generation);
 if(mode==0){p[6]=d->count;p[7]=d->interval_count;ct_put32(p+8,d->tracking_since);ct_put32(p+12,d->total_opens);ct_put32(p+16,d->total_extra);ct_put32(p+20,d->boundary);ct_put32(p+24,d->interval_opens);ct_put32(p+28,d->interval_extra);p[32]=d->interval_partial;ct_put32(p+33,d->untrusted_opens);ct_put32(p+37,d->lost_records);return 41;}
 if(generation!=d->generation)return 0;
 if(mode==1&&index<8){memcpy(p+6,&d->slots[index],40);return 46;}
 if(mode==2){const DoseRecord *r=analytics_record(d,index);if(r){memcpy(p+6,r,48);return 54;}}
 if(mode==3){const OpenInterval *r=analytics_interval(d,index);if(r){memcpy(p+6,r,20);return 26;}}
 return 0;
}
