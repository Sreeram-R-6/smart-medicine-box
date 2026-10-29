#include "calendar.h"
int calendar_valid(const MenuDate *d){
 return d->year>=2000&&d->year<=2099&&d->month>=1&&d->month<=12&&d->day>=1&&
  d->day<=menu_days_in_month(d->year,d->month)&&d->hour<24&&d->minute<60&&d->second<60;
}
uint32_t calendar_seconds(const MenuDate *d){
 uint32_t days=0;
 for(unsigned y=2000;y<d->year;y++)days+=365+(y%4==0);
 for(unsigned m=1;m<d->month;m++)days+=(uint32_t)menu_days_in_month(d->year,m);
 days+=d->day-1;
 return days*86400u+d->hour*3600u+d->minute*60u+d->second;
}
MenuDate calendar_date(uint32_t seconds){
 MenuDate d={2000,1,1,0,0,0};uint32_t days=seconds/86400;
 d.hour=(uint8_t)(seconds%86400/3600);d.minute=(uint8_t)(seconds%3600/60);d.second=(uint8_t)(seconds%60);
 while(days>=365u+(d.year%4==0)){days-=365u+(d.year%4==0);d.year++;}
 while(days>=(unsigned)menu_days_in_month(d.year,d.month)){days-=(unsigned)menu_days_in_month(d.year,d.month);d.month++;}
 d.day=(uint8_t)(days+1);return d;
}
unsigned calendar_weekday(const MenuDate *d){return (calendar_seconds(d)/86400u+5)%7;}
