#include "alarm.h"
#include "rtc_health.h"
#include "persistence.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static MenuLive live={.date={2026,10,29,9,59,59},.clock_valid=1,.clock_trusted=1,.storage_ok=1};
static MenuSchedule slots[MENU_SLOTS];
static void setup(Alarm *a){
 memset(slots,0,sizeof slots);slots[0]=(MenuSchedule){"MED A",10,0,127,1};
 live=(MenuLive){.date={2026,10,29,9,59,59},.clock_valid=1,.clock_trusted=1,.storage_ok=1};
 alarm_init(a,0);alarm_poll(a,&live,slots);
}
static void at(Alarm *a,unsigned hour,unsigned minute,unsigned second,uint32_t tick,int open){
 live.date.hour=(uint8_t)hour;live.date.minute=(uint8_t)minute;live.date.second=(uint8_t)second;live.uptime=tick;live.lid_open=(uint32_t)open;
 alarm_poll(a,&live,slots);
}
static unsigned events(Alarm *a,unsigned type){AlarmEvent e;unsigned count=0;while(alarm_event(a,&e))count+=e.type==type;return count;}
static uint8_t bcd(unsigned n){return (uint8_t)((n/10)*16+n%10);}
static void regs(uint8_t b[19],MenuDate d){memset(b,0,19);b[0]=bcd(d.second);b[1]=bcd(d.minute);b[2]=bcd(d.hour);b[3]=(uint8_t)(calendar_weekday(&d)+1);b[4]=bcd(d.day);b[5]=bcd(d.month);b[6]=bcd(d.year-2000);b[14]=0x1c;b[15]=8;}
static void health_tests(void){
 RtcHealth h={0};uint8_t b[19];MenuDate d={2026,10,29,12,0,0};
 for(unsigned i=0;i<4;i++){d.second=(uint8_t)i;regs(b,d);rtc_health_sample(&h,b,1,i*1000);assert(h.trusted==(i==3));}
 b[15]|=0x80;rtc_health_sample(&h,b,1,4000);assert(!h.trusted&&h.osf);
 for(unsigned i=5;i<9;i++){d.second=(uint8_t)i;regs(b,d);rtc_health_sample(&h,b,1,i*1000);}assert(h.trusted);
 rtc_health_sample(&h,b,1,13000);assert(h.stalled&&!h.trusted);
 d.day=30;regs(b,d);rtc_health_sample(&h,b,1,14000);assert(!h.trusted&&h.jumps);
 regs(b,(MenuDate){2026,2,30,0,0,0});rtc_health_sample(&h,b,1,15000);assert(!h.valid);
 rtc_health_sample(&h,b,0,16000);assert(!h.trusted&&h.errors==1);
 // 12-hour noon is valid, while invalid BCD and a disabled oscillator are not trusted.
 memset(&h,0,sizeof h);regs(b,(MenuDate){2026,10,29,12,0,0});b[2]=0x72;
 rtc_health_sample(&h,b,1,0);assert(h.valid&&h.date.hour==12);b[0]=0x6a;rtc_health_sample(&h,b,1,1000);assert(!h.valid);
}
static void bank(uint8_t *out,const DeviceData *d,uint32_t sequence){
 memset(out,0xff,STORAGE_BANK_BYTES);StorageHeader h={STORAGE_MAGIC,STORAGE_SCHEMA,sequence,sizeof *d,storage_crc(d,sizeof *d),STORAGE_COMMIT};
 memcpy(out,&h,sizeof h);memcpy(out+sizeof h,d,sizeof *d);
}
static void persistence_tests(void){
 Menu m;Alarm a;setup(&a);menu_init(&m,&live);DeviceData d;storage_snapshot(&d,&m,&a,calendar_seconds(&live.date));
 assert(storage_data_valid(&d)&&sizeof(StorageHeader)+sizeof d<=4096);
 assert(storage_crc("123456789",9)==0xcbf43926);
 uint8_t b0[STORAGE_BANK_BYTES],b1[STORAGE_BANK_BYTES],new_bank[STORAGE_BANK_BYTES];
 bank(b0,&d,1);d.settings.sound=0;bank(new_bank,&d,2);assert(storage_choose(b0,new_bank)==1);
 // Model a power cut at every halfword of a new bank, preserving the old committed bank.
 for(unsigned cut=0;cut<sizeof(StorageHeader)+sizeof d;cut+=2){
  memset(b1,0xff,sizeof b1);
  unsigned head=cut<20?cut:20;memcpy(b1,new_bank,head);
  if(cut>sizeof(StorageHeader))memcpy(b1+sizeof(StorageHeader),new_bank+sizeof(StorageHeader),cut-sizeof(StorageHeader));
  assert(storage_choose(b0,b1)==0);
 }
 memcpy(b1,new_bank,sizeof b1);memset(b1+20,0xff,4);memcpy(b1+20,new_bank+20,2);assert(storage_choose(b0,b1)==0);
 memcpy(b1+22,new_bank+22,2);assert(storage_choose(b0,b1)==1);
 b1[sizeof(StorageHeader)+1]^=1;assert(storage_choose(b0,b1)==0);
 bank(b0,&d,0xffffffffu);bank(b1,&d,0);assert(storage_choose(b0,b1)==1);
 d.settings.brightness=0;bank(b1,&d,1);assert(storage_choose(b0,b1)==0);
 memset(b0,0xff,sizeof b0);assert(storage_choose(b0,b1)==-1);
}
int main(void){
 for(unsigned y=2000;y<2100;y++)for(unsigned month=1;month<=12;month++){
  MenuDate d={(uint16_t)y,(uint8_t)month,(uint8_t)menu_days_in_month(y,month),23,59,59};MenuDate decoded=calendar_date(calendar_seconds(&d));
  assert(calendar_valid(&d)&&d.year==decoded.year&&d.month==decoded.month&&d.day==decoded.day&&decoded.hour==23&&decoded.minute==59&&decoded.second==59);
 }
 assert(calendar_weekday(&(MenuDate){2026,10,29,0,0,0})==3);
 Alarm a;setup(&a);at(&a,10,0,0,1000,0);assert(alarm_active_count(&a)==1);assert(events(&a,ALARM_DUE)==1);
 for(unsigned i=0;i<10;i++)at(&a,10,0,i,1000+i*1000,0);
 assert(events(&a,ALARM_DUE)==0);
 at(&a,10,0,10,12000,1);assert(alarm_active_count(&a)==0&&events(&a,ALARM_LID_ACK)==1);
 at(&a,10,0,11,13000,1);assert(events(&a,ALARM_LID_ACK)==0);
 at(&a,10,0,0,14000,0);at(&a,10,0,1,15000,1);assert(alarm_active_count(&a)==0&&events(&a,ALARM_DUE)==0);

 setup(&a);live.lid_open=1;alarm_poll(&a,&live,slots);at(&a,10,0,0,1000,1);assert(alarm_active_count(&a)==1);
 at(&a,10,0,1,2000,1);assert(events(&a,ALARM_LID_ACK)==0);
 at(&a,10,0,2,3000,0);at(&a,10,0,3,4000,1);assert(events(&a,ALARM_LID_ACK)==1);

 setup(&a);slots[1]=(MenuSchedule){"MED B",10,0,127,1};at(&a,10,0,0,1000,0);
 assert(alarm_active_count(&a)==1&&a.saved.groups[a.selected].mask==3);
 strcpy(slots[0].name,"RENAMED");assert(!strcmp(a.saved.groups[a.selected].names[0],"MED A"));
 at(&a,10,0,1,2000,1);AlarmEvent e;unsigned ack=0;while(alarm_event(&a,&e))if(e.type==ALARM_LID_ACK){ack++;assert(e.mask==3);}assert(ack==1);

 setup(&a);slots[1]=(MenuSchedule){"MED B",10,1,127,1};at(&a,10,0,0,1000,0);at(&a,10,1,0,61000,0);
 assert(alarm_active_count(&a)==2);at(&a,10,1,1,62000,1);assert(alarm_active_count(&a)==0);
 at(&a,10,1,2,63000,1);assert(alarm_active_count(&a)==0);at(&a,10,1,3,64000,0);at(&a,10,1,4,65000,1);assert(!alarm_active_count(&a));

 setup(&a);at(&a,10,0,0,1000,0);at(&a,10,15,0,901000,0);
 assert(alarm_active_count(&a)==1&&events(&a,ALARM_MISSED)==0);
 at(&a,10,15,1,902000,0);at(&a,10,15,2,903000,1);assert(!alarm_active_count(&a));
 setup(&a);at(&a,10,30,0,1801000,0);assert(!alarm_active_count(&a)&&events(&a,ALARM_DOWNTIME)==1);
 at(&a,10,0,0,1802000,0);at(&a,10,0,1,1803000,0);assert(!alarm_active_count(&a));

 setup(&a);slots[0].days=1;at(&a,10,0,0,1000,0);assert(!alarm_active_count(&a));
 live.date=(MenuDate){2026,11,2,10,0,0};live.uptime=5000;alarm_poll(&a,&live,slots);assert(alarm_active_count(&a)==1);

 setup(&a);at(&a,10,0,0,1000,0);Alarm restored;alarm_init(&restored,1);restored.saved=a.saved;
 live.date.minute=5;live.uptime=100;live.lid_open=1;alarm_poll(&restored,&live,slots);
 assert(alarm_active_count(&restored)==1);
 alarm_init(&restored,1);restored.saved=a.saved;live.date.minute=30;alarm_poll(&restored,&live,slots);
 assert(alarm_active_count(&restored)==1&&events(&restored,ALARM_DOWNTIME)==0);

 setup(&a);live.clock_trusted=0;at(&a,10,0,0,1000,0);assert(!alarm_active_count(&a));
 alarm_test(&a,1000);assert(alarm_frequency(&a,1000)==1568&&alarm_frequency(&a,1500)==1976&&alarm_frequency(&a,2000)==2349&&alarm_frequency(&a,1450)==0);
 for(unsigned i=0;i<20;i++)assert(alarm_frequency(&a,1000+i*500)>0);
 assert(alarm_frequency(&a,11000)==1568);alarm_button(&a,MENU_BACK);alarm_button(&a,MENU_OK);assert(a.test_active);
 at(&a,10,0,1,11001,0);assert(a.test_active);at(&a,10,0,2,12001,1);assert(!a.test_active);
 alarm_test(&a,0xffffff00u);live.uptime=(uint32_t)(0xffffff00u+30001u);alarm_poll(&a,&live,slots);assert(a.test_active);
 live.lid_open=0;alarm_poll(&a,&live,slots);live.lid_open=1;alarm_poll(&a,&live,slots);assert(!a.test_active);
 // A pending reminder keeps sounding across a long outage, even with invalid RTC.
 setup(&a);at(&a,10,0,0,1000,0);alarm_init(&restored,1);restored.saved=a.saved;live.clock_trusted=0;live.uptime=50;live.lid_open=0;
 alarm_poll(&restored,&live,slots);assert(alarm_active_count(&restored)==1&&alarm_frequency(&restored,50)>0);
 live.lid_open=1;alarm_poll(&restored,&live,slots);assert(!alarm_active_count(&restored));

 health_tests();persistence_tests();
 puts("PASS: 1200 calendar roundtrips; RTC trust/OSF/stall; due/duplicate/group/fresh-lid/overlap/late/weekday/reboot/clock-gap/test-tone; CRC/commit/power-cut/fallback/sequence-wrap");
 return 0;
}
