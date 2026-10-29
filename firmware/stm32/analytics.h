#pragma once
#include "alarm.h"
#define DOSE_RECORDS 24
#define OPEN_INTERVALS 8
enum { DOSE_OPENED=1,DOSE_RESPONSE_VALID=2,DOSE_PENDING=4,DOSE_INTERRUPTED=8,
 DOSE_DOWNTIME=16,DOSE_INTERVAL_PARTIAL=32,DOSE_IMPORTED=64,DOSE_TIME_UNCERTAIN=128 };
enum { REPORT_UNKNOWN=0,REPORT_TAKEN=1,REPORT_NOT_TAKEN=2 };
typedef struct {
 uint32_t id,due,opened,response_ms,ring_ms,reported_at;
 char name[17];
 uint8_t slot,report,flags;
 uint16_t interval_opens,interval_extra;
} DoseRecord;
typedef struct {
 uint64_t response_ms,ring_ms;
 uint32_t due,opened,taken,not_taken,response_count,ring_count;
} SlotStats;
typedef struct {uint32_t start,end,opens,extra,partial;} OpenInterval;
typedef struct {
 DoseRecord records[DOSE_RECORDS];
 SlotStats slots[MENU_SLOTS];
 OpenInterval intervals[OPEN_INTERVALS];
 uint32_t generation,next_id,total_opens,total_extra,boundary,interval_opens,interval_extra;
 uint32_t tracking_since,untrusted_opens,previous_opens,previous_extra,lost_records;
 uint8_t head,count,interval_head,interval_count,interval_partial,previous_partial,reserved[2];
} AnalyticsData;
typedef struct {AnalyticsData data;uint32_t tick,checkpoint;uint8_t dirty,critical;} Analytics;
void analytics_init(Analytics *a,const AnalyticsData *saved,const Alarm *alarm,uint32_t tick);
void analytics_tick(Analytics *a,const MenuLive *live);
void analytics_lid(Analytics *a,int is_reminder,const MenuLive *live);
void analytics_alarm(Analytics *a,const AlarmEvent *event,const MenuSchedule slots[MENU_SLOTS],const MenuLive *live);
void analytics_gap(Analytics *a);
int analytics_report(Analytics *a,uint32_t id,unsigned report,unsigned previous,const MenuLive *live);
int analytics_valid(const AnalyticsData *data);
const DoseRecord *analytics_record(const AnalyticsData *data,unsigned index);
const OpenInterval *analytics_interval(const AnalyticsData *data,unsigned index);
unsigned analytics_packet(uint8_t *out,const AnalyticsData *data,unsigned mode,unsigned index,uint32_t generation);
