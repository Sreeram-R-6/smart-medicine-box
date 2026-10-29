#include "remote.h"
#include "calendar.h"
uint16_t remote_revision(const Menu *m){
 uint8_t p[172];memcpy(p,m->schedules,168);p[168]=m->settings.sound;p[169]=m->settings.brightness;
 p[170]=(uint8_t)m->settings.duration_ms;p[171]=(uint8_t)(m->settings.duration_ms>>8);return med_crc(p,sizeof p);
}
_Static_assert(sizeof(MenuSchedule)==21,"Wire schedule layout changed");
int remote_slot(MenuSchedule *s,const uint8_t *p,unsigned n){
 if(n!=21||p[16]||p[17]>23||p[18]>59||p[19]>127||p[20]>1||!p[19]||!p[0])return 0;
 unsigned length=0;while(length<16&&p[length]){uint8_t c=p[length++];if(c!=' '&&(c<'A'||c>'Z')&&(c<'0'||c>'9'))return 0;}
 if(!length)return 0;
 memset(s,0,sizeof *s);memcpy(s->name,p,length);s->hour=p[17];s->minute=p[18];s->days=p[19];s->enabled=p[20];return 1;
}
int remote_settings(MenuSettings *s,const uint8_t *p,unsigned n){
 if(n!=4)return 0;
 unsigned d=p[2]|p[3]<<8;
 if(p[0]>1||p[1]<1||p[1]>10||d<100||d>2000||d%100)return 0;
 *s=(MenuSettings){p[0],p[1],(uint16_t)d};return 1;
}
unsigned remote_state(uint8_t *p,const Menu *m,const MenuLive *l,unsigned test){
 memset(p,0,200);ct_put32(p,l->clock_valid?calendar_seconds(&l->date):0);
 uint16_t rev=remote_revision(m);p[4]=(uint8_t)rev;p[5]=(uint8_t)(rev>>8);
 p[6]=(uint8_t)((l->clock_valid?1:0)|(l->clock_trusted?2:0)|(l->lid_open?4:0)|(l->storage_ok?8:0)|(l->rtc_osf?16:0)|(test?32:0));
 p[7]=(uint8_t)l->alarm_count;p[8]=m->settings.sound;p[9]=m->settings.brightness;p[10]=(uint8_t)m->settings.duration_ms;p[11]=(uint8_t)(m->settings.duration_ms>>8);
 memcpy(p+12,m->schedules,168);ct_put32(p+180,l->uptime);ct_put32(p+184,l->rtc_errors);ct_put32(p+188,l->oled_errors);
 p[192]=m->history_count;p[193]=(uint8_t)l->clock_progress;
 return 194;
}
