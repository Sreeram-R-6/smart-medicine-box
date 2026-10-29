#include "persistence.h"
#include <string.h>
_Static_assert(sizeof(StorageHeader)+sizeof(DeviceData)<=STORAGE_BANK_BYTES,"Persistent data exceeds reserved bank");
_Static_assert(offsetof(DeviceData,analytics)==STORAGE_LEGACY_BYTES,"Legacy storage prefix changed");
uint32_t storage_crc(const void *p,size_t count){
 const uint8_t *b=p;uint32_t crc=0xffffffffu;
 while(count--){crc^=*b++;for(unsigned bit=0;bit<8;bit++)crc=(crc>>1)^((0u-(crc&1))&0xedb88320u);}
 return ~crc;
}
static int storage_base_valid(const DeviceData *d){
 if(d->last_known_epoch>3155759999u||d->alarms.cursor>3155759999u)return 0;
 if(d->settings.sound>1||d->settings.brightness<1||d->settings.brightness>10||d->settings.duration_ms<100||d->settings.duration_ms>2000||d->settings.duration_ms%100)return 0;
 if(d->history_head>=MENU_HISTORY_SIZE||d->history_count>MENU_HISTORY_SIZE)return 0;
 for(unsigned i=0;i<MENU_SLOTS;i++){
  const MenuSchedule *s=&d->schedules[i];
  if(!memchr(s->name,0,17)||s->hour>23||s->minute>59||s->days>127||s->enabled>1||(s->enabled&&!s->days))return 0;
  if(d->alarms.last_due[i]>3155759999u)return 0;
  const AlarmGroup *g=&d->alarms.groups[i];
  if(g->active>1||g->armed>1||g->muted>1||g->due>3155759999u||(g->active&&(!g->mask||g->window_ms>ALARM_WINDOW_SECONDS*1000)))return 0;
  for(unsigned n=0;n<MENU_SLOTS;n++)if(!memchr(g->names[n],0,17))return 0;
 }
 for(unsigned i=0;i<MENU_HISTORY_SIZE;i++)if(d->history[i].type>EVENT_TEST_ALARM||d->history[i].dated>1||
  d->history[i].due>3155759999u||(d->history[i].dated&&!calendar_valid(&d->history[i].date)))return 0;
 return 1;
}
int storage_data_valid(const DeviceData *d){return storage_base_valid(d)&&analytics_valid(&d->analytics);}
int storage_bank_valid(const void *bank,StorageHeader *out){
 StorageHeader h;memcpy(&h,bank,sizeof h);
 int legacy=h.schema==1&&h.length==STORAGE_LEGACY_BYTES;
 if(h.magic!=STORAGE_MAGIC||h.commit!=STORAGE_COMMIT||(!legacy&&(h.schema!=STORAGE_SCHEMA||h.length!=sizeof(DeviceData))))return 0;
 const uint8_t *payload=(const uint8_t *)bank+sizeof h;
 if(storage_crc(payload,h.length)!=h.crc||!storage_base_valid((const DeviceData *)payload)||(!legacy&&!analytics_valid(&((const DeviceData *)payload)->analytics)))return 0;
 if(out)*out=h;
 return 1;
}
int storage_decode(const void *bank,DeviceData *d){
 StorageHeader h;if(!storage_bank_valid(bank,&h))return 0;
 memset(d,0,sizeof *d);memcpy(d,(const uint8_t *)bank+sizeof h,h.length);return 1;
}
int storage_choose(const void *b0,const void *b1){
 StorageHeader h0,h1;int v0=storage_bank_valid(b0,&h0),v1=storage_bank_valid(b1,&h1);
 if(!v0&&!v1)return -1;
 if(!v0)return 1;
 if(!v1)return 0;
 return (int32_t)(h1.sequence-h0.sequence)>0?1:0;
}
void storage_snapshot(DeviceData *d,const Menu *m,const Alarm *a,uint32_t known){
 memset(d,0,sizeof *d);memcpy(d->schedules,m->schedules,sizeof d->schedules);d->settings=m->settings;
 memcpy(d->history,m->history,sizeof d->history);d->history_head=m->history_head;d->history_count=m->history_count;
 d->alarms=a->saved;d->last_known_epoch=known;
}
void storage_restore(const DeviceData *d,Menu *m,Alarm *a){
 memcpy(m->schedules,d->schedules,sizeof m->schedules);m->settings=d->settings;
 memcpy(m->history,d->history,sizeof m->history);m->history_head=d->history_head;m->history_count=d->history_count;
 a->saved=d->alarms;a->recovering=1;m->contrast_dirty=1;m->persist_dirty=0;
}
