#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define OLED_HEIGHT 64
#define HAL_OK 0
static uint32_t now,i2c_error;
static uint32_t soft_i2c_write_failed_byte;
static int display_dirty,present=1,fail_data,visible,fail_on;
static unsigned page,column,recoveries;
static uint8_t device[1024];
static uint32_t HAL_GetTick(void){return now;}
static void HAL_Delay(unsigned n){now+=n;}
static void logmsg(const char *tag,const char *fmt,...){(void)tag;(void)fmt;}
static void i2c_recover(void){recoveries++;}
static int bus_probe(uint8_t address){return present&&address==0x3c?0:1;}
static int bus_write(uint8_t address,const uint8_t *p,unsigned n){
 assert(address==0x3c);if(!present)return 1;
 if(p[0]==0&&n==2&&p[1]==0xae)visible=0;
 if(p[0]==0&&n==2&&p[1]==0xaf){if(fail_on){fail_on--;return 1;}visible=1;}
 if(p[0]==0&&n==4&&(p[1]&0xf8)==0xb0){page=p[1]&7;column=(p[2]&15)|((p[3]&15)<<4);}
 if(p[0]==0x40){
  unsigned bytes=n-1;int failed=fail_data>0;
  if(failed){fail_data--;bytes=5;}
  for(unsigned i=0;i<bytes;i++){device[page*128+column]=p[i+1];column=(column+1)%128;}
  if(failed){i2c_error=1;return 1;}
 }
 return 0;
}
#include "oled_test.h"
static void complete_frame(void){
 unsigned before=oled_frames;
 for(unsigned i=0;i<4000&&oled_frames==before;i++){now+=3;oled_poll();if(oled_ready&&oled_page==8)oled_begin_frame();}
 assert(oled_frames>before);assert(memcmp(device,oled_pixels,1024)==0);assert(memcmp(oled_sent,oled_pixels,1024)==0);
}
int main(void){
 memset(device,0xa5,sizeof device);
 oled_init();assert(oled_ready&&display_dirty&&!visible&&!oled_lit);
 now+=2100;oled_poll();assert(!visible); // Idle health cannot light old RAM.
 for(unsigned i=0;i<1024;i++)oled_pixels[i]=(uint8_t)(i*37+i/128);
 oled_begin_frame();fail_data=1;
 for(unsigned i=0;i<10;i++){now+=3;oled_poll();assert(!visible);}
 complete_frame();assert(visible&&oled_lit&&oled_errors==1&&recoveries==1);
 // A partial accepted write must be retried from its original column.
 oled_pixels[517]^=0x5a;oled_begin_frame();fail_data=1;complete_frame();assert(oled_errors==2);
 // Repeated failures escalate to full init and a full redraw.
 oled_pixels[12]^=0xa5;oled_begin_frame();fail_data=3;complete_frame();assert(oled_attempts==2);
 present=0;now+=2100;oled_poll();assert(!oled_ready);
 present=1;now+=2100;oled_poll();assert(oled_ready);oled_begin_frame();complete_frame();
 // If display-on itself fails, reconnect and upload everything before lighting.
 oled_init();assert(!visible);fail_on=1;oled_begin_frame();complete_frame();assert(visible&&oled_lit);
 puts("PASS: OLED partial-write readdressing, shadow integrity, bounded retries, reconnect and full redraw");
}
