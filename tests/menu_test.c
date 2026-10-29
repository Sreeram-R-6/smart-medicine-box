#include "menu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static MenuLive live={.date={2026,10,29,14,35,12},.uptime=123456,.clock_valid=1,.clock_trusted=1,.storage_ok=1,.clock_progress=3};
static void key(Menu *m,enum MenuButton b){menu_button(m,b,&live);}
static void preview(FILE *out,const char *label,const Menu *m){
 MenuFrame f;menu_render(m,&live,&f);
 fprintf(out,"%s\t%d\t%d\n",label,f.selected_row,f.character_column);
 for(unsigned i=0;i<8;i++){assert(strlen(f.rows[i])<=21);fprintf(out,"%s\n",f.rows[i]);}
}
static void open_schedule(Menu *m){menu_init(m,&live);key(m,MENU_OK);key(m,MENU_RIGHT);key(m,MENU_OK);key(m,MENU_OK);assert(m->view==UI_SCHEDULE);}
int main(void){
 Menu m;FILE *out=fopen(".pio/menu-preview.txt","w");assert(out);
 menu_init(&m,&live);assert(m.history_count==1&&m.view==UI_HOME);preview(out,"HOME",&m);
 key(&m,MENU_OK);assert(m.view==UI_MAIN);preview(out,"MAIN MENU",&m);
 key(&m,MENU_LEFT);assert(m.selection==3);key(&m,MENU_RIGHT);assert(m.selection==0);
 key(&m,MENU_OK);assert(m.view==UI_CLOCK);preview(out,"CLOCK EDITOR",&m);
 key(&m,MENU_OK);key(&m,MENU_RIGHT);assert(m.clock_draft.year==2027);key(&m,MENU_BACK);assert(m.clock_draft.year==2026&&!m.editing);
 // Calendar edits clamp leap days and month lengths, with cancel restoring the draft.
 m.clock_draft=(MenuDate){2024,2,29,23,59,59};m.field=0;key(&m,MENU_OK);key(&m,MENU_RIGHT);
 assert(m.clock_draft.year==2025&&m.clock_draft.day==28);key(&m,MENU_BACK);assert(m.clock_draft.day==29);
 m.field=3;key(&m,MENU_OK);key(&m,MENU_RIGHT);assert(m.clock_draft.hour==0);key(&m,MENU_OK);
 assert(!m.clock_command);m.field=6;key(&m,MENU_OK);assert(m.view==UI_CLOCK_WAIT&&m.clock_command);
 uint8_t b[7];menu_clock_bcd(&m,b);assert(b[3]==4&&b[4]==0x29&&b[5]==2&&b[6]==0x24);
 key(&m,MENU_BACK);assert(m.view==UI_CLOCK_WAIT);
 menu_clock_result(&m,4,&live);assert(m.view==UI_NOTICE);key(&m,MENU_OK);assert(m.view==UI_CLOCK&&m.clock_draft.day==29);
 m.clock_command=0;key(&m,MENU_OK);menu_clock_result(&m,2,&live);assert(m.view==UI_NOTICE);key(&m,MENU_OK);assert(m.view==UI_MAIN);
 assert(menu_days_in_month(2000,2)==29&&menu_days_in_month(2100,2)==28&&menu_days_in_month(2026,4)==30);

 open_schedule(&m);preview(out,"SCHEDULE EDITOR",&m);
 // Discarding a modified draft must leave saved slots unchanged.
 m.field=1;key(&m,MENU_OK);key(&m,MENU_RIGHT);key(&m,MENU_OK);assert(m.schedule_draft.hour==9);
 key(&m,MENU_BACK);assert(m.schedules[0].hour==8&&m.view==UI_SCHEDULES);
 key(&m,MENU_OK);m.field=0;key(&m,MENU_OK);assert(m.view==UI_NAME);preview(out,"NAME EDITOR",&m);
 char original[17];memcpy(original,m.name_backup,17);key(&m,MENU_OK);key(&m,MENU_LEFT);assert(m.schedule_draft.name[0]=='L');
 key(&m,MENU_BACK);assert(m.schedule_draft.name[0]=='M'&&m.view==UI_NAME);key(&m,MENU_BACK);assert(!memcmp(original,m.schedule_draft.name,17));
 key(&m,MENU_OK);key(&m,MENU_LEFT);assert(m.name_cursor==16);key(&m,MENU_OK);assert(m.view==UI_SCHEDULE);
 m.field=3;key(&m,MENU_OK);preview(out,"WEEKDAYS",&m);key(&m,MENU_OK);assert(m.schedule_draft.days==0x7e);
 key(&m,MENU_BACK);assert(m.schedule_draft.days==0x7f);
 key(&m,MENU_OK);key(&m,MENU_OK);m.day_cursor=7;key(&m,MENU_OK);assert(m.schedule_draft.days==0x7e);
 m.field=4;key(&m,MENU_OK);key(&m,MENU_RIGHT);key(&m,MENU_OK);m.field=5;key(&m,MENU_OK);
 assert(m.schedules[0].enabled&&m.schedules[0].days==0x7e&&m.view==UI_NOTICE&&m.persist_wait);
 menu_storage_result(&m,1);preview(out,"SAVE RESULT",&m);
 key(&m,MENU_OK);preview(out,"SCHEDULE LIST",&m);
 key(&m,MENU_OK);m.schedule_draft.days=0;m.field=5;key(&m,MENU_OK);assert(m.view==UI_NOTICE&&m.schedules[0].days==0x7e);
 key(&m,MENU_OK);assert(m.field==3);memset(m.schedule_draft.name,' ',16);m.schedule_draft.name[16]=0;m.schedule_draft.days=1;m.field=5;key(&m,MENU_OK);
 assert(m.view==UI_NOTICE&&!strcmp(m.notice,"ENTER A NAME"));

 menu_init(&m,&live);m.view=UI_SETTINGS;preview(out,"SETTINGS",&m);
 key(&m,MENU_OK);assert(!m.settings.sound);key(&m,MENU_RIGHT);key(&m,MENU_OK);key(&m,MENU_RIGHT);
 assert(m.setting_draft==600&&m.settings.duration_ms==500);key(&m,MENU_BACK);assert(m.view==UI_SETTINGS&&!m.editing&&m.settings.duration_ms==500);
 key(&m,MENU_OK);key(&m,MENU_RIGHT);key(&m,MENU_OK);assert(m.settings.duration_ms==600);
 key(&m,MENU_RIGHT);key(&m,MENU_OK);key(&m,MENU_LEFT);assert(!m.contrast_dirty);key(&m,MENU_OK);
 assert(m.settings.brightness==4&&m.contrast_dirty);key(&m,MENU_RIGHT);key(&m,MENU_OK);assert(m.test_sound);
 key(&m,MENU_RIGHT);key(&m,MENU_OK);assert(m.view==UI_DIAGNOSTICS);preview(out,"DIAGNOSTICS",&m);
 key(&m,MENU_BACK);assert(m.view==UI_SETTINGS);

 menu_init(&m,&live);
 for(unsigned i=0;i<70;i++){live.uptime+=1000;menu_record(&m,i&1?EVENT_LID_OPEN:EVENT_LID_CLOSED,&live);}
 assert(m.history_count==32&&m.history_head==7);m.view=UI_HISTORY;preview(out,"HISTORY",&m);
 key(&m,MENU_OK);preview(out,"EVENT DETAILS",&m);assert(m.view==UI_EVENT);
 const MenuEvent *newest=&m.history[(m.history_head+31)%32];assert(newest->type==EVENT_LID_OPEN&&newest->uptime==live.uptime);
 MenuEvent snapshot=m.event_detail;live.uptime+=1000;menu_record(&m,EVENT_LID_CLOSED,&live);assert(memcmp(&snapshot,&m.event_detail,sizeof snapshot)==0);
 // Navigation/render stress explores drafts, cancellation and bounded history indices.
 unsigned rng=12345;
 for(unsigned i=0;i<100000;i++){
  rng=rng*1664525u+1013904223u;key(&m,(enum MenuButton)(rng>>30));
  if(m.clock_command){m.clock_command=0;menu_clock_result(&m,2,&live);}
  if(m.persist_wait)menu_storage_result(&m,1);
  MenuFrame f;menu_render(&m,&live,&f);
  for(unsigned r=0;r<8;r++)assert(memchr(f.rows[r],0,sizeof f.rows[r]));
  assert(m.slot<8&&m.settings.brightness>=1&&m.settings.brightness<=10);
  assert(m.history_selection<m.history_count&&f.selected_row>=-1&&f.selected_row<8);
 }
 fclose(out);puts("PASS: navigation, clock/calendar/save errors, draft cancellation, schedules/names/days, settings, history wrap, 100000 navigation/render steps");
 return 0;
}
