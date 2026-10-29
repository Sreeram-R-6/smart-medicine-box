#include "protocol.h"
#include <assert.h>
#include <stdio.h>

static int feed_frame(MedParser *p,const uint8_t *b,uint32_t now){
 int result=0;for(unsigned i=0;i<10;i++){int r=med_feed(p,b[i],now+i);if(r)result=r;}return result;
}
int main(void){
 assert(med_crc((const uint8_t *)"123456789",9)==0x29b1);
 MedParser p={0};uint8_t b[10];
 med_frame(b,MED_HELLO,0x1234,0xabcd);
 assert(feed_frame(&p,b,0)==1);assert(med_seq(&p)==0x1234);
 assert(p.bytes[6]==0xcd&&p.bytes[7]==0xab);
 b[6]^=1;assert(feed_frame(&p,b,20)==-1);b[6]^=1;
 assert(feed_frame(&p,b,40)==1);
 // An incomplete frame expires, then the next frame is accepted.
 med_feed(&p,0xa5,100);med_feed(&p,0x5a,101);
 assert(feed_frame(&p,b,300)==1);
 // Noise and repeated first-sync bytes must not prevent resynchronization.
 med_feed(&p,0x11,400);med_feed(&p,0xa5,401);
 assert(feed_frame(&p,b,402)==1);
 // Reject unsupported versions even with a matching CRC.
 b[2]=2;uint16_t crc=med_crc(b+2,6);b[8]=(uint8_t)crc;b[9]=(uint8_t)(crc>>8);
 assert(feed_frame(&p,b,500)==-1);
 // Timeout arithmetic must survive uptime wrap.
 p.used=0;med_feed(&p,0xa5,0xfffffff0u);med_feed(&p,0x5a,0xfffffff1u);
 med_frame(b,MED_HEARTBEAT,65535,0);assert(feed_frame(&p,b,200)==1);
 puts("PASS: CRC vector, framing, corruption, timeout, resync, version, uptime wrap");
 return 0;
}
