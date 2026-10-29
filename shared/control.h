#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "protocol.h"
/* v2: A5 5A 02 type length seqLE32 payload CRC16 (version through payload). */
#define CT_MAX 208
#define CT_FRAME (CT_MAX+11)
enum { CT_HELLO=16,CT_STATE,CT_SLOT,CT_SETTINGS,CT_CLOCK,CT_TEST,CT_RESULT,CT_HISTORY,CT_EVENT,CT_ANALYTICS,CT_REPORT };
enum { CT_OK=0,CT_INVALID=1,CT_BUSY=2,CT_CONFLICT=3,CT_STORAGE=4,CT_RTC=5 };
typedef struct {uint8_t bytes[CT_FRAME];unsigned used;uint32_t last;} CtParser;
static inline uint32_t ct_u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static inline void ct_put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static inline unsigned ct_frame(uint8_t *b,uint8_t type,uint32_t seq,const uint8_t *data,unsigned n){
 if(n>CT_MAX)return 0;
 b[0]=0xa5;b[1]=0x5a;b[2]=2;b[3]=type;b[4]=(uint8_t)n;ct_put32(b+5,seq);
 if(n)memcpy(b+9,data,n);
 uint16_t c=med_crc(b+2,n+7);b[n+9]=(uint8_t)c;b[n+10]=(uint8_t)(c>>8);return n+11;
}
static inline int ct_feed(CtParser *p,uint8_t v,uint32_t now){
 if(p->used&&(uint32_t)(now-p->last)>100)p->used=0;
 p->last=now;
 if(!p->used){if(v==0xa5)p->bytes[p->used++]=v;return 0;}
 if(p->used==1&&v!=0x5a){p->used=v==0xa5?1:0;return 0;}
 p->bytes[p->used++]=v;
 if((p->used==3&&v!=2)||(p->used==5&&v>CT_MAX)){p->used=0;return -1;}
 if(p->used<5||p->used<(unsigned)p->bytes[4]+11)return 0;
 unsigned n=p->bytes[4];p->used=0;
 return med_crc(p->bytes+2,n+7)==(uint16_t)(p->bytes[n+9]|p->bytes[n+10]<<8)?1:-1;
}
