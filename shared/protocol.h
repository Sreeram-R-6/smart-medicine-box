#ifndef MED_PROTOCOL_H
#define MED_PROTOCOL_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
// Fixed 10-byte diagnostic frame. No strings or dynamic allocation.
// A5 5A version type seqLE16 valueLE16 crcLE16. CRC covers bytes 2..7.
enum { MED_HELLO=1, MED_HEARTBEAT=2, MED_STATUS=3, MED_ACK=4 };
typedef struct { uint8_t bytes[10]; uint8_t used; uint32_t last; } MedParser;
static inline uint16_t med_crc(const uint8_t *p, size_t n) {
 uint16_t c=0xffff;
 while(n--) { c^=(uint16_t)*p++<<8; for(unsigned i=0;i<8;i++) c=(c&0x8000)?(uint16_t)((c<<1)^0x1021):(uint16_t)(c<<1); }
 return c;
}
static inline void med_frame(uint8_t *b,uint8_t type,uint16_t seq,uint16_t value) {
 b[0]=0xa5;b[1]=0x5a;b[2]=1;b[3]=type;b[4]=(uint8_t)seq;b[5]=(uint8_t)(seq>>8);
 b[6]=(uint8_t)value;b[7]=(uint8_t)(value>>8);uint16_t c=med_crc(b+2,6);b[8]=(uint8_t)c;b[9]=(uint8_t)(c>>8);
}
// Return 1 for a valid complete frame, -1 for corrupt/unknown-version frame.
static inline int med_feed(MedParser *p,uint8_t v,uint32_t now) {
 if(p->used && (uint32_t)(now-p->last)>100) p->used=0;
 p->last=now;
 if(!p->used) {if(v==0xa5)p->bytes[p->used++]=v;return 0;}
 if(p->used==1 && v!=0x5a) {p->used=(v==0xa5)?1:0;return 0;}
 p->bytes[p->used++]=v;
 if(p->used<10)return 0;
 p->used=0;
 return p->bytes[2]==1 && med_crc(p->bytes+2,6)==(uint16_t)(p->bytes[8]|p->bytes[9]<<8)?1:-1;
}
static inline uint16_t med_seq(const MedParser *p){return (uint16_t)(p->bytes[4]|p->bytes[5]<<8);}
#endif
