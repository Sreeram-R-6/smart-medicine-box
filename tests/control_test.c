#include "remote.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 uint8_t frame[CT_FRAME],data[CT_MAX];for(unsigned i=0;i<CT_MAX;i++)data[i]=(uint8_t)i;
 for(unsigned n=0;n<=CT_MAX;n++){
  unsigned length=ct_frame(frame,CT_STATE,0xf1234567,data,n);assert(length==n+11);
  CtParser p={0};for(unsigned i=0;i<length;i++)assert(ct_feed(&p,frame[i],i)==(i==length-1?1:0));
  assert(ct_u32(p.bytes+5)==0xf1234567&&p.bytes[4]==n&&!memcmp(p.bytes+9,data,n));
  for(unsigned j=2;j<length;j++){uint8_t b[CT_FRAME];memcpy(b,frame,length);b[j]^=1;memset(&p,0,sizeof p);unsigned valid=0;
   for(unsigned i=0;i<length;i++)valid+=ct_feed(&p,b[i],i)==1;
   assert(!valid);
  }
 }
 CtParser p={0};ct_feed(&p,0xa5,0);ct_feed(&p,0x5a,1);ct_feed(&p,2,2);assert(!ct_feed(&p,0,200));assert(!p.used);
 Menu m;MenuLive l={.date={2026,10,29,12,0,0},.clock_valid=1,.clock_trusted=1,.storage_ok=1};menu_init(&m,&l);
 unsigned rev=remote_revision(&m);m.settings.brightness=3;assert(remote_revision(&m)!=rev);
 uint8_t state[200];assert(remote_state(state,&m,&l,0)==194&&state[6]==11);
 uint8_t slot[21]={0};memcpy(slot,"REMINDER",8);slot[17]=23;slot[18]=59;slot[19]=127;slot[20]=1;MenuSchedule s;
 assert(remote_slot(&s,slot,21));slot[16]='X';assert(!remote_slot(&s,slot,21));slot[16]=0;slot[17]=24;assert(!remote_slot(&s,slot,21));slot[17]=0;slot[19]=0;assert(!remote_slot(&s,slot,21));
 MenuSettings settings;uint8_t values[]={1,10,0xf4,1};assert(remote_settings(&settings,values,4));values[1]=11;assert(!remote_settings(&settings,values,4));
 puts("PASS: all v2 payload lengths, per-byte corruption, timeout, wire state, config revisions and boundary validation");return 0;
}
