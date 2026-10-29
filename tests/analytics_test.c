#include "analytics.h"
#include "persistence.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static MenuLive live={.date={2026,10,29,10,0,0},.clock_valid=1,.clock_trusted=1,.storage_ok=1};
static MenuSchedule slots[8]={{"MEDICINE A",10,0,127,1},{"MEDICINE B",11,0,127,1}};
static uint32_t epoch(void){return calendar_seconds(&live.date);}
static void event(Analytics *a,unsigned type,unsigned mask,uint32_t due){AlarmEvent e={due,(uint8_t)type,(uint8_t)mask};analytics_alarm(a,&e,slots,&live);}
static void migration(void){
 Menu menu;Alarm alarm;DeviceData source,decoded;menu_init(&menu,&live);alarm_init(&alarm,0);storage_snapshot(&source,&menu,&alarm,epoch());
 uint8_t bank[4096];memset(bank,0xff,sizeof bank);
 StorageHeader h={STORAGE_MAGIC,1,5,STORAGE_LEGACY_BYTES,storage_crc(&source,STORAGE_LEGACY_BYTES),STORAGE_COMMIT};
 memcpy(bank,&h,sizeof h);memcpy(bank+sizeof h,&source,STORAGE_LEGACY_BYTES);
 assert(storage_bank_valid(bank,0)&&storage_decode(bank,&decoded));
 assert(!memcmp(&decoded,&source,STORAGE_LEGACY_BYTES));assert(decoded.analytics.count==0&&decoded.analytics.total_opens==0&&storage_data_valid(&decoded));
 bank[sizeof h+1]^=1;assert(!storage_decode(bank,&decoded));
}
int main(void){
 Alarm alarm;alarm_init(&alarm,0);Analytics a;analytics_init(&a,0,&alarm,0);analytics_tick(&a,&live);uint32_t due=epoch();
 event(&a,ALARM_DUE,1,due);event(&a,ALARM_DUE,1,due);assert(a.data.count==1&&a.data.slots[0].due==1);
 live.uptime=9000;live.date.second=9;analytics_tick(&a,&live);analytics_lid(&a,1,&live);event(&a,ALARM_LID_ACK,1,due);event(&a,ALARM_LID_ACK,1,due);
 const DoseRecord *r=analytics_record(&a.data,0);assert(r->response_ms==9000&&r->ring_ms==9000&&(r->flags&DOSE_RESPONSE_VALID)&&!(r->flags&DOSE_PENDING));
 assert(a.data.slots[0].opened==1&&a.data.slots[0].response_count==1&&a.data.slots[0].response_ms==9000&&a.data.slots[0].ring_count==1);
 uint32_t id=r->id;assert(analytics_report(&a,id,REPORT_TAKEN,0,&live)==1&&a.data.slots[0].taken==1);
 assert(analytics_report(&a,id,REPORT_TAKEN,0,&live)==1&&a.data.slots[0].taken==1);
 assert(analytics_report(&a,id,REPORT_NOT_TAKEN,0,&live)==-1&&a.data.slots[0].taken==1);
 assert(analytics_report(&a,id,REPORT_NOT_TAKEN,1,&live)==1&&a.data.slots[0].taken==0&&a.data.slots[0].not_taken==1);
 assert(analytics_report(&a,id,REPORT_UNKNOWN,2,&live)==1&&a.data.slots[0].not_taken==0&&!analytics_record(&a.data,0)->reported_at);
 analytics_lid(&a,0,&live);live.date.hour=11;live.date.second=0;live.uptime=3600000;analytics_tick(&a,&live);event(&a,ALARM_DUE,3,epoch());
 assert(a.data.total_opens==2&&a.data.total_extra==1&&a.data.interval_count==1);
 assert(analytics_interval(&a.data,0)->opens==2&&analytics_interval(&a.data,0)->extra==1);
 assert(analytics_record(&a.data,0)->interval_opens==2&&analytics_record(&a.data,1)->interval_opens==2);
 // Restored active records retain their measured powered time, but are excluded
 // from complete-session ringing averages after a power interruption.
 Analytics restored;analytics_init(&restored,&a.data,&alarm,50);live.uptime=1050;analytics_tick(&restored,&live);
 assert(analytics_record(&restored.data,0)->flags&DOSE_INTERRUPTED);event(&restored,ALARM_LID_ACK,3,epoch());
 assert(restored.data.slots[1].opened==1&&restored.data.slots[1].ring_count==0);
 // A clock discontinuity never produces a falsely precise response delay.
 live.uptime=4000000;live.date.hour=12;event(&a,ALARM_DUE,1,epoch());analytics_gap(&a);live.date.second=5;event(&a,ALARM_LID_ACK,1,epoch()-5);
 assert(!(analytics_record(&a.data,0)->flags&DOSE_RESPONSE_VALID));
 // Uptime wrap is monotonic for intervals shorter than one full wrap.
 Analytics wrap;analytics_init(&wrap,0,&alarm,0xfffffff0);event(&wrap,ALARM_DUE,1,epoch());live.uptime=32;analytics_tick(&wrap,&live);assert(analytics_record(&wrap.data,0)->ring_ms==48);
 // Bounded record retention does not reset per-medicine lifetime counters.
 for(unsigned i=0;i<40;i++)event(&a,ALARM_DOWNTIME,1,epoch()+i*86400u);
 assert(a.data.count==24&&a.data.lost_records>0&&a.data.slots[0].due>24&&analytics_valid(&a.data));
 uint8_t packet[64];assert(analytics_packet(packet,&a.data,0,0,0)==41);
 assert(analytics_packet(packet,&a.data,2,0,a.data.generation)==54);
 assert(analytics_packet(packet,&a.data,2,0,a.data.generation-1)==0);
 assert(!analytics_report(&a,0,1,0,&live));assert(!analytics_report(&a,0xfffffffe,1,0,&live));
 migration();puts("PASS: real-event analytics, timing, openings/intervals, grouped/duplicate events, self-report conflicts, reboot/clock uncertainty, retention, packet generations and schema-1 migration");return 0;
}
