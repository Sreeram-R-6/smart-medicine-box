#include "rtc_health.h"
static unsigned bcd(uint8_t b){return (b>>4)*10+(b&15);}
static int valid_bcd(uint8_t b){return (b>>4)<=9&&(b&15)<=9;}
void rtc_health_sample(RtcHealth *h,const uint8_t b[19],int ok,uint32_t now){
 h->trusted=0;
 if(!ok){h->errors++;h->valid=0;h->progress=0;return;}
 MenuDate d={(uint16_t)(2000+bcd(b[6])+((b[5]&0x80)?100:0)),(uint8_t)bcd(b[5]&31),(uint8_t)bcd(b[4]),
  (uint8_t)bcd(b[2]&63),(uint8_t)bcd(b[1]),(uint8_t)bcd(b[0])};
 int hour_ok=!(b[2]&0x80);
 if(b[2]&0x40){unsigned hour=bcd(b[2]&31);hour_ok=hour_ok&&valid_bcd(b[2]&31)&&hour>=1&&hour<=12;d.hour=(uint8_t)(hour%12+((b[2]&0x20)?12:0));}
 else hour_ok=hour_ok&&valid_bcd(b[2]&63);
 h->osf=!!(b[15]&0x80);
 h->valid=(uint8_t)(valid_bcd(b[0])&&valid_bcd(b[1])&&hour_ok&&!(b[4]&0xc0)&&valid_bcd(b[4])&&
  !(b[5]&0x60)&&valid_bcd(b[5]&31)&&valid_bcd(b[6])&&b[3]>=1&&b[3]<=7&&calendar_valid(&d));
 if(!h->valid){h->progress=0;return;}
 h->date=d;uint32_t epoch=calendar_seconds(&d);
 if(!h->initialized){h->initialized=1;h->last_epoch=epoch;h->last_change=now;h->progress=0;}
 else if(epoch!=h->last_epoch){
  uint32_t expected=(uint32_t)(now-h->last_change)/1000;
  uint32_t advance=epoch-h->last_epoch;
  if(epoch<h->last_epoch||advance>expected+3){h->jumps++;h->progress=0;h->fault=1;}
  else if(h->progress<3)h->progress++;
  h->last_change=now;h->last_epoch=epoch;
 }
 h->last_sample=now;h->stalled=(uint8_t)((uint32_t)(now-h->last_change)>3500);
 if(h->stalled)h->fault=1;
 if(h->osf||h->stalled||(b[14]&0x80))h->progress=0;
 h->trusted=(uint8_t)(h->valid&&!h->osf&&!h->stalled&&!h->fault&&!(b[14]&0x80)&&h->progress>=3);
}
