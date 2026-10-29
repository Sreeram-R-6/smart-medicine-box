#include "config.h"
#include "persistence.h"
#include <string.h>
#define STORAGE_BASE 0x0800e000u
static const uint8_t *bank(unsigned n){return (const uint8_t *)(STORAGE_BASE+n*STORAGE_BANK_BYTES);}
int storage_load(DeviceData *d){
 int selected=storage_choose(bank(0),bank(1));
 if(selected<0){
  int blank=1;
  for(unsigned n=0;n<2;n++)for(unsigned i=0;i<STORAGE_BANK_BYTES;i++)if(bank(n)[i]!=0xff)blank=0;
  return blank?0:-1;
 }
 memcpy(d,bank((unsigned)selected)+sizeof(StorageHeader),sizeof *d);return 1;
}
static int program_bytes(uint32_t address,const uint8_t *bytes,unsigned count){
 for(unsigned i=0;i<count;i+=2){
  uint16_t value=bytes[i];value|=(uint16_t)(i+1<count?bytes[i+1]:0xff)<<8;
  if(value!=0xffff&&HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,address+i,value)!=HAL_OK)return 0;
 }
 return 1;
}
int storage_save(const DeviceData *d){
 if(!storage_data_valid(d))return 0;
 int selected=storage_choose(bank(0),bank(1));unsigned target=selected==0?1:0;uint32_t seq=1;
 if(selected>=0){StorageHeader old;memcpy(&old,bank((unsigned)selected),sizeof old);seq=old.sequence+1;}
 uint32_t address=STORAGE_BASE+target*STORAGE_BANK_BYTES;
 StorageHeader h={STORAGE_MAGIC,STORAGE_SCHEMA,seq,sizeof *d,storage_crc(d,sizeof *d),0xffffffffu};
 if(HAL_FLASH_Unlock()!=HAL_OK)return 0;
 FLASH_EraseInitTypeDef erase={0};erase.TypeErase=FLASH_TYPEERASE_PAGES;erase.PageAddress=address;erase.NbPages=4;uint32_t error=0;
 int ok=HAL_FLASHEx_Erase(&erase,&error)==HAL_OK;
 if(ok)ok=program_bytes(address,(const uint8_t *)&h,sizeof h);
 if(ok)ok=program_bytes(address+sizeof h,(const uint8_t *)d,sizeof *d);
 if(ok)ok=memcmp(bank(target)+sizeof h,d,sizeof *d)==0;
 uint32_t commit=STORAGE_COMMIT;
 if(ok)ok=program_bytes(address+offsetof(StorageHeader,commit),(const uint8_t *)&commit,sizeof commit);
 HAL_FLASH_Lock();
 return ok&&storage_bank_valid(bank(target),0);
}
