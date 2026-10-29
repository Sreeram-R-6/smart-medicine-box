#include "config.h"
#include "protocol.h"
#include "menu.h"
#include "alarm.h"
#include "persistence.h"
#include "rtc_health.h"
#include "remote.h"
#include "screen.h"
#include <stdio.h>
#include <stdarg.h>

UART_HandleTypeDef huart1,huart2;
I2C_HandleTypeDef hi2c1;
static uint8_t rxbyte,rxring[512];
static volatile unsigned rxhead,rxtail,rxoverflow,rxerror;
static char logbuf[1024];
static unsigned loghead,logtail,logdrop;
static uint8_t logbyte;
static volatile int logbusy;
static uint8_t txframe[CT_FRAME];
static uint8_t txqueue[4][CT_FRAME];
static unsigned txlength[4];
static volatile unsigned txhead,txtail;
static volatile int txbusy;
static uint32_t txdrop,badframes;
static uint32_t lastpeer;
static int linked;
static CtParser parser;
static struct {
 uint32_t raw, stable, changes, low_count, high_count, last_change;
} sensor_diag;
static int display_dirty=1;
static uint32_t boot_screen_started,screen_tick;
static uint8_t boot_screen_seen,boot_screen_done;
static Menu menu;
static Alarm alarms;
static Analytics analytics;
static RtcHealth rtc_health;
static DeviceData stored_data;
static uint32_t last_known_epoch,storage_changed,storage_writes,storage_errors;
static uint8_t storage_ok=1;
static uint16_t buzzer_frequency;
static uint8_t menu_clock_pending;
static const char *const button_names[4]={"LEFT","RIGHT","BACK","OK"};
static struct {
 uint32_t pressed_mask, presses[4], releases[4], last_button, last_pressed;
} button_diag={.last_button=4};
static struct {
 uint32_t active, started, count, completed, duration;
} buzzer_diag;
static void buzzer_set_frequency(unsigned frequency){
 if(frequency==buzzer_frequency)return;
 if(frequency){TIM3->ARR=1000000u/frequency-1;TIM3->CCR3=(TIM3->ARR+1)/2;}
 else TIM3->CCR3=TIM3->ARR+1;
 TIM3->EGR=TIM_EGR_UG;buzzer_frequency=(uint16_t)frequency;
}
static void buzzer_init(void){
 // PB0 is TIM3 channel 3. 8 MHz HSI / 8 / 500 = 2 kHz.
 __HAL_RCC_TIM3_CLK_ENABLE();
 TIM3->CR1=0;TIM3->PSC=7;TIM3->ARR=1000000u/BUZZER_TONE_HZ-1;
 TIM3->CCMR2=TIM_CCMR2_OC3PE|TIM_CCMR2_OC3M_1|TIM_CCMR2_OC3M_2;
 TIM3->CCR3=TIM3->ARR+1; // Constant HIGH = silent between tones.
 TIM3->CCER=TIM_CCER_CC3E;TIM3->EGR=TIM_EGR_UG;
 TIM3->CR1=TIM_CR1_ARPE|TIM_CR1_CEN;
 GPIO_InitTypeDef g={0};g.Pin=BUZZER_PIN;g.Mode=GPIO_MODE_AF_PP;g.Speed=GPIO_SPEED_FREQ_LOW;
 HAL_GPIO_Init(BUZZER_PORT,&g);
}
static void buzzer_poll(uint32_t now){
 if(buzzer_diag.active && (uint32_t)(now-buzzer_diag.started)>=buzzer_diag.duration){
  buzzer_diag.active=0;buzzer_diag.completed++;
 }
 int alarm_sound=alarms.test_active;
 if(!alarms.recovering)for(unsigned i=0;i<MENU_SLOTS;i++)if(alarms.saved.groups[i].active&&!alarms.saved.groups[i].muted)alarm_sound=1;
 buzzer_set_frequency(alarm_sound?alarm_frequency(&alarms,now):buzzer_diag.active?BUZZER_TONE_HZ:0);
}
static void buzzer_opened(void){
 buzzer_diag.duration=menu.settings.duration_ms;
 buzzer_diag.started=HAL_GetTick();buzzer_diag.active=1;buzzer_diag.count++;
}
static void fatal(void){ __disable_irq(); for(;;){} }
static void logmsg(const char *tag,const char *fmt,...) {
 char b[180]; int n=snprintf(b,sizeof b,"[%lu][%s] ",(unsigned long)HAL_GetTick(),tag);
 va_list ap;va_start(ap,fmt);vsnprintf(b+n,sizeof b-(size_t)n,fmt,ap);va_end(ap);
 size_t len=strlen(b);if(len>sizeof b-3)len=sizeof b-3;b[len++]='\r';b[len++]='\n';
 unsigned freebytes=(logtail+sizeof logbuf-loghead-1)%sizeof logbuf;
 if(len>freebytes){logdrop++;return;}
 for(size_t i=0;i<len;i++){logbuf[loghead]=b[i];loghead=(loghead+1)%sizeof logbuf;}
}
static void logpoll(void){
 if(!logbusy && logtail!=loghead){logbyte=(uint8_t)logbuf[logtail];logbusy=1;
 if(HAL_UART_Transmit_IT(&huart1,&logbyte,1)==HAL_OK)logtail=(logtail+1)%sizeof logbuf;else logbusy=0;}
}
static int sendcontrol(uint8_t type,uint32_t seq,const uint8_t *p,unsigned n){
 unsigned next=(txhead+1)%4;if(next==txtail){txdrop++;return 0;}
 txlength[txhead]=ct_frame(txqueue[txhead],type,seq,p,n);txhead=next;return 1;
}
static void txpoll(void){
 if(txbusy||txhead==txtail)return;
 unsigned n=txlength[txtail];memcpy(txframe,txqueue[txtail],n);txbusy=1;
 if(HAL_UART_Transmit_IT(&huart2,txframe,n)!=HAL_OK){txbusy=0;txdrop++;}
 else txtail=(txtail+1)%4;
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *h){if(h==&huart1)logbusy=0;if(h==&huart2)txbusy=0;}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *h){if(h!=&huart2)return;
 unsigned next=(rxhead+1)%sizeof rxring;if(next!=rxtail){rxring[rxhead]=rxbyte;rxhead=next;}else rxoverflow++;
 HAL_UART_Receive_IT(&huart2,&rxbyte,1);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *h){if(h==&huart2){rxerror++;HAL_UART_Receive_IT(&huart2,&rxbyte,1);}}
void USART1_IRQHandler(void){HAL_UART_IRQHandler(&huart1);}
void USART2_IRQHandler(void){HAL_UART_IRQHandler(&huart2);}
void SysTick_Handler(void){HAL_IncTick();HAL_SYSTICK_IRQHandler();}
static void uartinit(UART_HandleTypeDef *h,USART_TypeDef *inst){
 h->Instance=inst;h->Init.BaudRate=115200;h->Init.WordLength=UART_WORDLENGTH_8B;h->Init.StopBits=UART_STOPBITS_1;
 h->Init.Parity=UART_PARITY_NONE;h->Init.Mode=UART_MODE_TX_RX;h->Init.HwFlowCtl=UART_HWCONTROL_NONE;h->Init.OverSampling=UART_OVERSAMPLING_16;
 if(HAL_UART_Init(h)!=HAL_OK)fatal();
}
static void hardware(void){
 HAL_Init(); // HSI 8 MHz avoids assuming the board crystal is correct.
 __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_GPIOB_CLK_ENABLE();__HAL_RCC_AFIO_CLK_ENABLE();
 __HAL_AFIO_REMAP_SWJ_NOJTAG(); // Keep SWD; release JTAG only.
 __HAL_RCC_USART1_CLK_ENABLE();__HAL_RCC_USART2_CLK_ENABLE();__HAL_RCC_I2C1_CLK_ENABLE();
 HAL_GPIO_WritePin(GPIOB,BUZZER_PIN,BUZZER_OFF);
 HAL_GPIO_WritePin(GPIOB,GREEN_PIN|RED_PIN,GPIO_PIN_RESET);
 GPIO_InitTypeDef g={0};g.Pin=BUZZER_PIN|GREEN_PIN|RED_PIN;g.Mode=GPIO_MODE_OUTPUT_PP;g.Speed=GPIO_SPEED_FREQ_LOW;HAL_GPIO_Init(GPIOB,&g);
 buzzer_init();
 g.Pin=GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5;g.Mode=GPIO_MODE_INPUT;g.Pull=GPIO_PULLUP;HAL_GPIO_Init(GPIOA,&g);
 // Weak 3.3 V pull-up also supports an open-collector sensor output.
 g.Pin=SENSOR_PIN;g.Pull=GPIO_PULLUP;HAL_GPIO_Init(GPIOB,&g);
 g.Pin=GPIO_PIN_2|GPIO_PIN_9;g.Mode=GPIO_MODE_AF_PP;g.Speed=GPIO_SPEED_FREQ_HIGH;HAL_GPIO_Init(GPIOA,&g);
 g.Pin=GPIO_PIN_3|GPIO_PIN_10;g.Mode=GPIO_MODE_INPUT;g.Pull=GPIO_PULLUP;HAL_GPIO_Init(GPIOA,&g);
 g.Pin=GPIO_PIN_6|GPIO_PIN_7;g.Mode=GPIO_MODE_AF_OD;g.Pull=GPIO_NOPULL;HAL_GPIO_Init(GPIOB,&g);
 uartinit(&huart1,USART1);uartinit(&huart2,USART2);
 HAL_NVIC_SetPriority(USART1_IRQn,2,0);HAL_NVIC_EnableIRQ(USART1_IRQn);
 HAL_NVIC_SetPriority(USART2_IRQn,1,0);HAL_NVIC_EnableIRQ(USART2_IRQn);
 // Conservative speed for the shared breadboard bus and display wiring.
 hi2c1.Instance=I2C1;hi2c1.Init.ClockSpeed=25000;hi2c1.Init.DutyCycle=I2C_DUTYCYCLE_2;
 hi2c1.Init.AddressingMode=I2C_ADDRESSINGMODE_7BIT;hi2c1.Init.DualAddressMode=I2C_DUALADDRESS_DISABLE;
 hi2c1.Init.GeneralCallMode=I2C_GENERALCALL_DISABLE;hi2c1.Init.NoStretchMode=I2C_NOSTRETCH_DISABLE;
 if(HAL_I2C_Init(&hi2c1)!=HAL_OK)fatal();
 HAL_UART_Receive_IT(&huart2,&rxbyte,1);
}
static uint32_t i2c_recoveries;
static void i2c_recover(void){
 // Release a slave left mid-byte after reset, then reset the F1 I2C engine.
 __HAL_I2C_DISABLE(&hi2c1);
 GPIO_InitTypeDef g={0};g.Pin=GPIO_PIN_6|GPIO_PIN_7;g.Mode=GPIO_MODE_OUTPUT_OD;g.Speed=GPIO_SPEED_FREQ_LOW;
 HAL_GPIO_WritePin(GPIOB,g.Pin,GPIO_PIN_SET);HAL_GPIO_Init(GPIOB,&g);HAL_Delay(1);
 for(unsigned i=0;i<9;i++){
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_RESET);HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_SET);HAL_Delay(1);
 }
 // STOP transition also refreshes the F1 analog input filters.
 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_RESET);HAL_Delay(1);
 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_RESET);HAL_Delay(1);
 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_SET);HAL_Delay(1);
 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET);HAL_Delay(1);
 g.Mode=GPIO_MODE_AF_OD;HAL_GPIO_Init(GPIOB,&g);
 SET_BIT(hi2c1.Instance->CR1,I2C_CR1_SWRST);__NOP();CLEAR_BIT(hi2c1.Instance->CR1,I2C_CR1_SWRST);
 hi2c1.State=HAL_I2C_STATE_RESET;HAL_I2C_Init(&hi2c1);i2c_recoveries++;
}
#include "oled_test.h"
static int bcd(uint8_t x){return (x>>4)*10+(x&15);}
static int bcdvalid(uint8_t x){return (x&15)<=9 && (x>>4)<=9;}
// Retained diagnostics can be read through SWD without a USB-TTL adapter.
static struct {
 uint32_t reads, errors, changes, last_change, last_second, stalled, valid, trusted;
 uint8_t registers[19];
 char date[24], time[24], state[24];
} rtc_diag;
// One-shot SWD mailbox. Cleared on reboot; never sets the clock at boot.
// Write BCD data first, then command=0x52544353. result: 2=OK,3=invalid,4=I2C,5=verify.
static volatile struct {
 uint32_t command, result;
 uint8_t data[7];
 uint32_t error;
} rtc_set_request;
static uint8_t rtc_sync_pending;
static uint32_t rtc_sync_started,rtc_sync_epoch;
static void rtc_set_poll(void){
 if(rtc_sync_pending){
  if((uint32_t)(HAL_GetTick()-rtc_sync_started)<2000)return;
  uint8_t b[7],status;
  if(HAL_I2C_Mem_Read(&hi2c1,0x68<<1,0,1,b,7,30)!=HAL_OK)goto failed;
  MenuDate d={(uint16_t)(2000+bcd(b[6])),(uint8_t)bcd(b[5]),(uint8_t)bcd(b[4]),(uint8_t)bcd(b[2]),(uint8_t)bcd(b[1]),(uint8_t)bcd(b[0])};
  uint32_t epoch=calendar_valid(&d)?calendar_seconds(&d):0;
  if(epoch<=rtc_sync_epoch||epoch-rtc_sync_epoch>5){rtc_set_request.result=5;rtc_sync_pending=0;return;}
  if(HAL_I2C_Mem_Read(&hi2c1,0x68<<1,15,1,&status,1,20)!=HAL_OK)goto failed;
  status&=0x7f;
  if(HAL_I2C_Mem_Write(&hi2c1,0x68<<1,15,1,&status,1,20)!=HAL_OK)goto failed;
  if(HAL_I2C_Mem_Read(&hi2c1,0x68<<1,15,1,&status,1,20)!=HAL_OK)goto failed;
  if(status&0x80){rtc_set_request.result=5;rtc_sync_pending=0;return;}
  rtc_sync_pending=0;rtc_set_request.result=2;rtc_health.initialized=0;rtc_health.progress=0;rtc_health.fault=0;
  last_known_epoch=epoch;menu.persist_dirty=1;display_dirty=1;
  logmsg("RTC/INFO","Host/menu sync verified after ticking; OSF cleared");return;
 }
 if(rtc_set_request.command!=0x52544353u)return;
 uint8_t b[7];for(unsigned i=0;i<7;i++)b[i]=rtc_set_request.data[i];
 rtc_set_request.command=0;rtc_set_request.result=1;rtc_set_request.error=0;
 MenuDate d={(uint16_t)(2000+bcd(b[6])),(uint8_t)bcd(b[5]),(uint8_t)bcd(b[4]),(uint8_t)bcd(b[2]),(uint8_t)bcd(b[1]),(uint8_t)bcd(b[0])};
 int valid=calendar_valid(&d)&&b[3]>=1&&b[3]<=7;
 for(unsigned i=0;i<7;i++)if(!bcdvalid(b[i]))valid=0;
 if(!valid){rtc_set_request.result=3;return;}
 analytics_gap(&analytics);
 uint8_t ctrl,verify[7];
 if(HAL_I2C_Mem_Read(&hi2c1,0x68<<1,14,1,&ctrl,1,20)!=HAL_OK)goto failed;
 ctrl&=0x7f;
 if(HAL_I2C_Mem_Write(&hi2c1,0x68<<1,14,1,&ctrl,1,20)!=HAL_OK)goto failed;
 if(HAL_I2C_Mem_Write(&hi2c1,0x68<<1,0,1,b,7,30)!=HAL_OK)goto failed;
 if(HAL_I2C_Mem_Read(&hi2c1,0x68<<1,0,1,verify,7,30)!=HAL_OK)goto failed;
 if(memcmp(b,verify,7)!=0){rtc_set_request.result=5;return;}
 rtc_sync_pending=1;rtc_sync_started=HAL_GetTick();rtc_sync_epoch=calendar_seconds(&d);return;
failed:
 rtc_sync_pending=0;rtc_set_request.error=HAL_I2C_GetError(&hi2c1);rtc_set_request.result=4;i2c_recover();
}
static void rtc_read(void){
 uint8_t fresh[19]={0};uint32_t now=HAL_GetTick();rtc_diag.reads++;
 // Separate time and health reads avoid a long burst through alarm/reserved registers.
 int ok=HAL_I2C_Mem_Read(&hi2c1,0x68<<1,0,1,fresh,7,30)==HAL_OK;
 if(ok)ok=HAL_I2C_Mem_Read(&hi2c1,0x68<<1,14,1,fresh+14,2,20)==HAL_OK;
 if(ok)ok=HAL_I2C_Mem_Read(&hi2c1,0x68<<1,17,1,fresh+17,2,20)==HAL_OK;
 rtc_health_sample(&rtc_health,fresh,ok,now);
 if(!ok){rtc_diag.errors++;i2c_recover();snprintf(rtc_diag.state,sizeof rtc_diag.state,"RTC READ ERROR");}
 else{
  memcpy(rtc_diag.registers,fresh,19);
  if(rtc_health.valid&&rtc_health.trusted&&rtc_health.last_epoch+2<last_known_epoch)rtc_health.trusted=0;
  if(rtc_health.trusted)last_known_epoch=rtc_health.last_epoch;
  snprintf(rtc_diag.state,sizeof rtc_diag.state,"%s",!rtc_health.valid?"INVALID CALENDAR":rtc_health.osf?"OSF SET - SET CLOCK":rtc_health.stalled?"CLOCK STALLED":rtc_health.fault?"CLOCK JUMP SET TIME":rtc_health.progress<3?"CHECKING CLOCK":!rtc_health.trusted?"CLOCK MOVED BACK":"RTC RUNNING");
 }
 rtc_diag.valid=rtc_health.valid;rtc_diag.trusted=rtc_health.trusted;rtc_diag.stalled=rtc_health.stalled;
 rtc_diag.last_change=rtc_health.last_change;rtc_diag.last_second=rtc_health.last_epoch;rtc_diag.changes=rtc_health.progress;
 if(rtc_health.valid){
  const MenuDate *d=&rtc_health.date;
  snprintf(rtc_diag.date,sizeof rtc_diag.date,"%04u-%02u-%02u",d->year,d->month,d->day);
  snprintf(rtc_diag.time,sizeof rtc_diag.time,"%02u:%02u:%02u",d->hour,d->minute,d->second);
 }else{snprintf(rtc_diag.date,sizeof rtc_diag.date,"NO VALID RTC DATA");snprintf(rtc_diag.time,sizeof rtc_diag.time,"SET CLOCK");}
 logmsg("RTC/INFO","%s %s OSF=%u check=%lu/3 trusted=%lu %s",rtc_diag.date,rtc_diag.time,rtc_health.osf,(unsigned long)rtc_health.progress,(unsigned long)rtc_diag.trusted,rtc_diag.state);
 display_dirty=1;
}
static MenuLive menu_live(void){
 MenuLive l={0};l.uptime=HAL_GetTick();l.lid_open=sensor_diag.stable==SENSOR_OPEN_LEVEL;
 l.clock_valid=rtc_diag.valid;l.clock_trusted=rtc_diag.trusted;l.button_mask=button_diag.pressed_mask;
 l.oled_errors=oled_errors;l.rtc_errors=rtc_diag.errors;
 l.alarm_count=alarm_active_count(&alarms);l.storage_ok=storage_ok;l.clock_progress=rtc_health.progress;l.rtc_osf=rtc_health.osf;l.link_up=linked;
 if(l.clock_valid){
  const uint8_t *b=rtc_diag.registers;
  l.date.year=(uint16_t)(2000+bcd(b[6])+((b[5]&0x80)?100:0));l.date.month=(uint8_t)bcd(b[5]&0x1f);l.date.day=(uint8_t)bcd(b[4]);
  l.date.hour=(uint8_t)bcd(b[2]&0x3f);
  if(b[2]&0x40)l.date.hour=(uint8_t)(bcd(b[2]&0x1f)%12+((b[2]&0x20)?12:0));
  l.date.minute=(uint8_t)bcd(b[1]);l.date.second=(uint8_t)bcd(b[0]);
 }
 return l;
}
static void menu_service(void){
 MenuLive l=menu_live();
 if(menu.clock_command){
  menu.clock_command=0;
  if(rtc_set_request.command||rtc_sync_pending){menu_clock_result(&menu,4,&l);}
  else{
   uint8_t b[7];menu_clock_bcd(&menu,b);
   for(unsigned i=0;i<7;i++)rtc_set_request.data[i]=b[i];
   rtc_set_request.result=0;rtc_set_request.command=0x52544353u;menu_clock_pending=1;
  }
  display_dirty=1;
 }
 if(menu_clock_pending&&rtc_set_request.result>=2){
  menu_clock_pending=0;unsigned result=rtc_set_request.result;
  if(result==2){rtc_read();l=menu_live();}
  menu_clock_result(&menu,result,&l);display_dirty=1;
 }
 if(menu.test_sound){
  boot_screen_done=1;menu.test_sound=0;alarm_test(&alarms,HAL_GetTick());
  if(alarms.test_active)menu_record(&menu,EVENT_TEST_ALARM,&l);
  display_dirty=1;
 }
 if(menu.contrast_dirty){
  oled_contrast=(uint8_t)(menu.settings.brightness*25+2);
  if(oled_ready&&oled_page>=OLED_HEIGHT/8){uint8_t cmd[]={0,0x81,oled_contrast};if(oled_write(cmd,sizeof cmd))menu.contrast_dirty=0;}
 }
}
static void app_button(enum MenuButton button){
 boot_screen_done=1;
 MenuLive l=menu_live();
 if(alarms.test_active||(alarms.show&&alarm_active_count(&alarms)))alarm_button(&alarms,button);
 else menu_button(&menu,button,&l);
 display_dirty=1;
}
static void storage_service(void){
 uint32_t now=HAL_GetTick();
 if(!menu.persist_dirty&&!alarms.dirty&&!analytics.dirty)return;
 if(!storage_ok&&storage_changed&&(uint32_t)(now-storage_changed)<5000)return;
 if(!alarms.dirty&&!menu.persist_wait&&!analytics.critical&&(uint32_t)(now-storage_changed)<1500)return;
 buzzer_set_frequency(0); // Never stretch a driven note while flash erase stalls the CPU.
 storage_snapshot(&stored_data,&menu,&alarms,last_known_epoch);stored_data.analytics=analytics.data;
 int ok=storage_save(&stored_data);storage_changed=HAL_GetTick();storage_ok=(uint8_t)ok;
 if(ok){menu.persist_dirty=0;alarms.dirty=0;analytics.dirty=analytics.critical=0;storage_writes++;}
 else storage_errors++;
 menu_storage_result(&menu,ok);display_dirty=1;
 logmsg("STORE/INFO","commit=%d writes=%lu errors=%lu",ok,(unsigned long)storage_writes,(unsigned long)storage_errors);
}
static void alarm_service(void){
 MenuLive l=menu_live();if(!storage_ok)l.clock_trusted=0;
 analytics_tick(&analytics,&l);if(rtc_health.fault||l.rtc_osf)analytics_gap(&analytics);
 uint32_t before=alarms.revision;alarm_poll(&alarms,&l,menu.schedules);
 AlarmEvent event;
 while(alarm_event(&alarms,&event)){
  analytics_alarm(&analytics,&event,menu.schedules,&l);
  const enum MenuEventType types[]={EVENT_ALARM_DUE,EVENT_ALARM_ACK,EVENT_ALARM_MISSED,EVENT_DOWNTIME,EVENT_ALARM_QUIET,EVENT_CLOCK_GAP};
  menu_record(&menu,types[event.type],&l);
  MenuEvent *stored=&menu.history[(menu.history_head+MENU_HISTORY_SIZE-1)%MENU_HISTORY_SIZE];stored->mask=event.mask;stored->due=event.due;
  logmsg("ALARM/INFO","event=%u slots=0x%02X due=%lu",event.type,event.mask,(unsigned long)event.due);
 }
 if(before!=alarms.revision)display_dirty=1;
 storage_service();buzzer_poll(HAL_GetTick());
}
static void display_refresh(void){
 uint32_t now=HAL_GetTick();
 if(!oled_ready||oled_page<OLED_HEIGHT/8)return;
 if(!boot_screen_seen){boot_screen_seen=1;boot_screen_started=now;}
 if(!boot_screen_done&&(uint32_t)(now-boot_screen_started)>=SCREEN_BOOT_MS){boot_screen_done=1;display_dirty=1;}
 int showing_alarm=alarms.test_active||(alarms.show&&alarm_active_count(&alarms));
 if(showing_alarm)boot_screen_done=1;
 if(!boot_screen_done){
  if((uint32_t)(now-screen_tick)<100)return;
  screen_boot(oled_pixels,now-boot_screen_started);screen_tick=now;oled_begin_frame();display_dirty=1;return;
 }
 if(!display_dirty)return;
 MenuLive l=menu_live();screen_render(oled_pixels,&menu,&l,&alarms);
 oled_begin_frame();display_dirty=0;
}
/* One outstanding mutation. ACK only after durable commit or verified RTC set.
   A repeated transaction ID returns its cached result without replaying side effects. */
static uint32_t remote_pending,remote_last;
static uint16_t remote_signature,remote_last_signature;
static uint8_t remote_kind,remote_last_result,remote_reply_wait;
static void remote_result(uint32_t seq,unsigned result){
 uint8_t b[3];uint16_t rev=remote_revision(&menu);b[0]=(uint8_t)result;b[1]=(uint8_t)rev;b[2]=(uint8_t)(rev>>8);
 sendcontrol(CT_RESULT,seq,b,3);
}
static void remote_complete(unsigned result){
 remote_last=remote_pending;remote_last_signature=remote_signature;remote_last_result=(uint8_t)result;
 remote_pending=0;remote_reply_wait=1;
}
static uint32_t history_revision(void){return storage_crc(menu.history,sizeof menu.history)^(uint32_t)menu.history_head^((uint32_t)menu.history_count<<8);}
static void remote_receive(void){
 const uint8_t *p=parser.bytes+9;unsigned n=parser.bytes[4],type=parser.bytes[3];uint32_t seq=ct_u32(parser.bytes+5);
 if(type==CT_HELLO)return;
 if(type==CT_ANALYTICS){
  uint8_t b[64];unsigned size=0;
  if(!n)size=analytics_packet(b,&analytics.data,0,0,0);
  else if(n==6)size=analytics_packet(b,&analytics.data,p[0],p[1],ct_u32(p+2));
  if(size){if(b[0]==0){ct_put32(b+size,history_revision());size+=4;}sendcontrol(CT_ANALYTICS,seq,b,size);}return;
 }
 if(type==CT_HISTORY){
  if((n!=1&&n!=5)||p[0]>=menu.history_count){remote_result(seq,CT_INVALID);return;}
  if(n==5&&ct_u32(p+1)!=history_revision())return;
  const MenuEvent *e=&menu.history[(menu.history_head+MENU_HISTORY_SIZE-1-p[0])%MENU_HISTORY_SIZE];
  uint8_t b[20]={p[0],e->type,e->mask,e->dated};ct_put32(b+4,e->dated?calendar_seconds(&e->date):0);ct_put32(b+8,e->due);ct_put32(b+12,e->uptime);
  if(n==5)ct_put32(b+16,history_revision());
  sendcontrol(CT_EVENT,seq,b,n==5?20:16);return;
 }
 if(type!=CT_SLOT&&type!=CT_SETTINGS&&type!=CT_CLOCK&&type!=CT_TEST&&type!=CT_REPORT){remote_result(seq,CT_INVALID);return;}
 if(!seq){remote_result(seq,CT_INVALID);return;}
 uint16_t signature=med_crc(parser.bytes+3,n+6);
 if(seq==remote_last){remote_result(seq,signature==remote_last_signature?remote_last_result:CT_CONFLICT);return;}
 if(remote_pending){if(seq!=remote_pending||signature!=remote_signature)remote_result(seq,CT_BUSY);return;}
 if(n<2){remote_result(seq,CT_INVALID);return;}
 uint16_t expected=p[0]|p[1]<<8;
 if(expected!=remote_revision(&menu)){remote_result(seq,CT_CONFLICT);return;}
 /* Do not overwrite a local OLED draft or a save already in progress. */
 if(menu.editing||menu.view==UI_CLOCK||menu.view==UI_CLOCK_WAIT||menu.view==UI_SCHEDULE||menu.view==UI_NAME||menu.view==UI_DAYS||menu.persist_wait||menu_clock_pending||rtc_sync_pending||rtc_set_request.command){remote_result(seq,CT_BUSY);return;}
 MenuLive l=menu_live();
 if(type==CT_SLOT){
  MenuSchedule value;if(n!=24||p[2]>=MENU_SLOTS||!remote_slot(&value,p+3,21)){remote_result(seq,CT_INVALID);return;}
  menu.schedules[p[2]]=value;menu_record(&menu,EVENT_SCHEDULE,&l);menu.persist_wait=1;
 }else if(type==CT_SETTINGS){
  MenuSettings value;if(n!=6||!remote_settings(&value,p+2,4)){remote_result(seq,CT_INVALID);return;}
  menu.settings=value;menu.contrast_dirty=1;menu_record(&menu,EVENT_SETTINGS,&l);menu.persist_wait=1;
 }else if(type==CT_CLOCK){
  if(n!=6||ct_u32(p+2)>3155759999u){remote_result(seq,CT_INVALID);return;}
  MenuDate d=calendar_date(ct_u32(p+2));uint8_t b[7];
  b[0]=(uint8_t)(d.second/10*16+d.second%10);b[1]=(uint8_t)(d.minute/10*16+d.minute%10);b[2]=(uint8_t)(d.hour/10*16+d.hour%10);
  b[3]=(uint8_t)(calendar_weekday(&d)+1);b[4]=(uint8_t)(d.day/10*16+d.day%10);b[5]=(uint8_t)(d.month/10*16+d.month%10);unsigned y=d.year-2000;b[6]=(uint8_t)(y/10*16+y%10);
  for(unsigned i=0;i<7;i++)rtc_set_request.data[i]=b[i];
  rtc_set_request.result=0;rtc_set_request.command=0x52544353u;
 }else if(type==CT_REPORT){
  if(n!=8){remote_result(seq,CT_INVALID);return;}
  int result=analytics_report(&analytics,ct_u32(p+2),p[6],p[7],&l);
  if(result<=0){remote_result(seq,result<0?CT_CONFLICT:CT_INVALID);return;}
 }else{
  if(n!=2){remote_result(seq,CT_INVALID);return;}
  alarm_test(&alarms,HAL_GetTick());menu_record(&menu,EVENT_TEST_ALARM,&l);
 }
 remote_pending=seq;remote_signature=signature;remote_kind=(uint8_t)type;display_dirty=1;
}
static void remote_poll(void){
 if(remote_pending){
  if(remote_kind==CT_CLOCK){
   if(rtc_set_request.result>=2){
    if(rtc_set_request.result==2){rtc_read();MenuLive l=menu_live();menu_record(&menu,EVENT_CLOCK_SET,&l);remote_complete(CT_OK);}
    else remote_complete(CT_RTC);
   }
  }else if(remote_kind==CT_TEST)remote_complete(CT_OK);
  else if(!storage_ok)remote_complete(CT_STORAGE);
  else if(!menu.persist_dirty&&!menu.persist_wait&&!analytics.dirty)remote_complete(CT_OK);
 }
 if(remote_reply_wait&&(txhead+1)%4!=txtail){remote_result(remote_last,remote_last_result);remote_reply_wait=0;}
 txpoll();
}
int main(void){
 MenuLive initial={0};menu_init(&menu,&initial);alarm_init(&alarms,0);
 int loaded=storage_load(&stored_data);
 if(loaded==1){storage_restore(&stored_data,&menu,&alarms);last_known_epoch=stored_data.last_known_epoch;menu_record(&menu,EVENT_BOOT,&initial);}
 else if(loaded==0){
  const uint8_t hours[]={6,10,13,16,20,23};
  for(unsigned i=0;i<6;i++){snprintf(menu.schedules[i].name,17,"REMINDER %u",i+1);menu.schedules[i].hour=hours[i];menu.schedules[i].minute=0;menu.schedules[i].enabled=1;menu.schedules[i].days=0x7f;}
 }else{storage_ok=0;menu.persist_dirty=0;}

 hardware();i2c_recover();analytics_init(&analytics,loaded==1?&stored_data.analytics:0,&alarms,HAL_GetTick());logmsg("BOOT/INFO","Firmware=%s HSI=8MHz reset_flags=0x%08lx SAFE bring-up only",FW_VERSION,(unsigned long)RCC->CSR);__HAL_RCC_CLEAR_RESET_FLAGS();
 uint32_t now=HAL_GetTick(),heartbeat=now,rtc=now,stats=now;
 uint32_t changed[5]={0};int raw[5],stable[5];
 for(int i=0;i<5;i++){raw[i]=stable[i]=(i<4?HAL_GPIO_ReadPin(BUTTON_PORT,BUTTON_PINS[i]):HAL_GPIO_ReadPin(SENSOR_PORT,SENSOR_PIN));}
 for(unsigned i=0;i<4;i++)if(!stable[i])button_diag.pressed_mask|=1u<<i;
 sensor_diag.raw=sensor_diag.stable=stable[4];
 logmsg("SENSOR/INFO","Initial raw=%lu calibrated=%d open_level=%d",(unsigned long)sensor_diag.stable,SENSOR_VERIFIED,SENSOR_OPEN_LEVEL);
 uint8_t scan=8;uint32_t repeat_started=0,repeat_last=0,chord_since=0;
 unsigned repeat_mask=0,pending_select=0;uint8_t chord_seen=0,chord_fired=0,chord_used=0;
 for(;;){now=HAL_GetTick();buzzer_poll(now);logpoll();txpoll();oled_poll();rtc_set_poll();
 unsigned pressed_events=0;
 for(int i=0;i<5;i++){
 int v=i<4?HAL_GPIO_ReadPin(BUTTON_PORT,BUTTON_PINS[i]):HAL_GPIO_ReadPin(SENSOR_PORT,SENSOR_PIN);
 if(i==4)sensor_diag.raw=v;
 if(v!=raw[i]){raw[i]=v;changed[i]=now;}
 if(v!=stable[i]&&(uint32_t)(now-changed[i])>=(i<4?BUTTON_DEBOUNCE_MS:SENSOR_FILTER_MS)){
 stable[i]=v;if(i<4){
  if(v){button_diag.pressed_mask&=~(1u<<i);button_diag.releases[i]++;}
  else{button_diag.pressed_mask|=1u<<i;button_diag.presses[i]++;pressed_events|=1u<<i;}
  button_diag.last_button=(uint32_t)i;button_diag.last_pressed=!v;display_dirty=1;
  logmsg("BUTTON/INFO","%s %s",button_names[i],v?"released":"pressed");
 }
 else {
  sensor_diag.stable=v;sensor_diag.changes++;sensor_diag.last_change=now;
  if(v)sensor_diag.high_count++;else sensor_diag.low_count++;
  display_dirty=1;
  if(ENABLE_OPEN_CHIRP && menu.settings.sound && SENSOR_VERIFIED && !alarm_active_count(&alarms) && !alarms.test_active && v==SENSOR_OPEN_LEVEL)buzzer_opened();
  MenuLive event_live=menu_live();
  if(v==SENSOR_OPEN_LEVEL){int reminder=0;for(unsigned g=0;g<MENU_SLOTS;g++)if(alarms.saved.groups[g].active&&alarms.saved.groups[g].armed)reminder=1;analytics_lid(&analytics,reminder,&event_live);}
  menu_record(&menu,v==SENSOR_OPEN_LEVEL?EVENT_LID_OPEN:EVENT_LID_CLOSED,&event_live);
  logmsg("SENSOR/INFO","Stable raw=%d box=%s changes=%lu chirps=%lu",v,v==SENSOR_OPEN_LEVEL?"OPEN":"CLOSED",(unsigned long)sensor_diag.changes,(unsigned long)buzzer_diag.count);
 }
 }}
 // BACK/OK act on release, allowing their held chord without accidental selection.
 unsigned held=button_diag.pressed_mask;uint32_t input_now=HAL_GetTick();
 if(held==12){
  pending_select=0;chord_used=1;
  if(!chord_seen){chord_seen=1;chord_since=input_now;}
  if(!chord_fired&&(uint32_t)(input_now-chord_since)>=700){menu.test_sound=1;chord_fired=1;}
 }else chord_seen=0;
 if(pressed_events && held==pressed_events && !(held&(held-1))&&!chord_used){
  if(held==4||held==8)pending_select=held;
  else{app_button(held==1?MENU_LEFT:MENU_RIGHT);repeat_mask=held;repeat_started=repeat_last=input_now;}
 }
 if(!held){
  if(pending_select&&!chord_used)app_button(pending_select==4?MENU_BACK:MENU_OK);
  pending_select=0;chord_used=0;chord_fired=0;
 }
 if(held!=repeat_mask)repeat_mask=0;
 if((repeat_mask==1||repeat_mask==2)&&menu_can_repeat(&menu)&&!alarms.show&&!alarms.test_active&&
    (uint32_t)(input_now-repeat_started)>=500&&(uint32_t)(input_now-repeat_last)>=140){
  app_button(repeat_mask==1?MENU_LEFT:MENU_RIGHT);repeat_last=input_now;
 }
 menu_service();
 while(rxtail!=rxhead){uint8_t v=rxring[rxtail];rxtail=(rxtail+1)%sizeof rxring;int r=ct_feed(&parser,v,now);
 if(r<0)badframes++;
 if(r>0){lastpeer=now;if(!linked){linked=1;logmsg("UART/INFO","Peer frames received");}
 remote_receive();
 }}
 if(linked&&(uint32_t)(now-lastpeer)>15000){linked=0;logmsg("UART/WARN","Link timeout");}
 if(linked&&(uint32_t)(now-heartbeat)>=1000){heartbeat=now;uint8_t b[200];MenuLive l=menu_live();unsigned n=remote_state(b,&menu,&l,alarms.test_active);sendcontrol(CT_STATE,0,b,n);n=analytics_packet(b,&analytics.data,0,0,0);ct_put32(b+n,history_revision());sendcontrol(CT_ANALYTICS,0,b,n+4);}

 if(ENABLE_I2C_DIAGNOSTICS && scan<120 && (uint32_t)(now-rtc)>=20){rtc=now;if(HAL_I2C_IsDeviceReady(&hi2c1,scan<<1,1,2)==HAL_OK)logmsg("I2C/INFO","Found 7-bit=0x%02X",scan);scan++;}
 if(ENABLE_I2C_DIAGNOSTICS && scan==120){oled_init();scan++;rtc=now-1000;}
 if(ENABLE_I2C_DIAGNOSTICS && scan>120 && (uint32_t)(now-rtc)>=1000){rtc=now;rtc_read();}
 alarm_service();remote_poll();display_refresh();
 if((uint32_t)(now-stats)>=30000){stats=now;logmsg("DIAG/INFO","rx_overflow=%u rx_error=%u tx_drop=%lu log_drop=%u invalid_frame=%lu",rxoverflow,rxerror,(unsigned long)txdrop,logdrop,(unsigned long)badframes);}
 }
}
