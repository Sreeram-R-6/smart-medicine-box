#pragma once
#include <stddef.h>
#include "alarm.h"
#define STORAGE_BANK_BYTES 4096u
#define STORAGE_MAGIC 0x4d454442u
#define STORAGE_COMMIT 0x434f4d54u
#define STORAGE_SCHEMA 1u
typedef struct {
 MenuSchedule schedules[MENU_SLOTS];
 MenuSettings settings;
 MenuEvent history[MENU_HISTORY_SIZE];
 uint8_t history_head,history_count;
 uint16_t reserved;
 AlarmCheckpoint alarms;
 uint32_t last_known_epoch;
} DeviceData;
typedef struct { uint32_t magic,schema,sequence,length,crc,commit; } StorageHeader;
uint32_t storage_crc(const void *bytes,size_t count);
int storage_data_valid(const DeviceData *data);
int storage_bank_valid(const void *bank,StorageHeader *header);
int storage_choose(const void *bank0,const void *bank1);
void storage_snapshot(DeviceData *data,const Menu *menu,const Alarm *alarm,uint32_t known_epoch);
void storage_restore(const DeviceData *data,Menu *menu,Alarm *alarm);
int storage_load(DeviceData *data); // 1=loaded,0=blank,-1=both corrupt.
int storage_save(const DeviceData *data);
