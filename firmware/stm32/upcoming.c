#include "upcoming.h"
#include "calendar.h"
#include <string.h>
int upcoming_reminder(const MenuSchedule slots[MENU_SLOTS],const MenuDate *date,NextReminder *next){
 memset(next,0,sizeof *next);if(!calendar_valid(date))return 0;
 uint32_t now=calendar_seconds(date),day=now/86400u;unsigned weekday=calendar_weekday(date);
 uint32_t best=0xffffffffu;
 for(unsigned d=0;d<=7;d++)for(unsigned i=0;i<MENU_SLOTS;i++){
  const MenuSchedule *s=&slots[i];if(!s->enabled||!s->days||s->hour>23||s->minute>59||!(s->days&(1u<<((weekday+d)%7))))continue;
  uint32_t due=(day+d)*86400u+s->hour*3600u+s->minute*60u;
  if(due<now||due>3155759999u||due>best)continue;
  if(due<best){best=due;next->epoch=due;next->mask=0;next->slot=(uint8_t)i;next->days_away=(uint8_t)d;}
  next->mask|=(uint8_t)(1u<<i);
 }
 return next->mask!=0;
}
