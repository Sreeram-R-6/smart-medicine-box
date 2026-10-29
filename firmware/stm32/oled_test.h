#pragma once
// SSD1306-compatible 128x64 trial driver. No controller autodetection claimed.
static uint8_t oled_address, oled_ready, oled_page, oled_chunk;
static uint32_t oled_errors, oled_frames;
static uint32_t oled_attempts, oled_last_attempt, oled_last_health;
static uint32_t oled_last_transfer;
static uint8_t oled_started;
static uint8_t oled_contrast=127,oled_dirty_pages,oled_force_redraw=1;
static uint8_t oled_pixels[1024];
static uint8_t oled_sent[1024];
static int oled_write(uint8_t *p,unsigned n){
 oled_last_transfer=HAL_GetTick();
 if(HAL_I2C_Master_Transmit(&hi2c1,oled_address<<1,p,n,40)==HAL_OK)return 1;
 oled_errors++;oled_ready=0;oled_last_attempt=HAL_GetTick();
 logmsg("OLED/WARN","I2C write failed error=0x%lx; retry in 2s",(unsigned long)HAL_I2C_GetError(&hi2c1));i2c_recover();return 0;
}
static void oled_init(void){
 oled_started=1;oled_attempts++;oled_last_attempt=HAL_GetTick();
 oled_ready=0;oled_address=0;
 for(uint8_t a=0x3c;a<=0x3d;a++)if(HAL_I2C_IsDeviceReady(&hi2c1,a<<1,1,5)==HAL_OK){oled_address=a;break;}
 if(!oled_address){logmsg("OLED/WARN","No ACK at 0x3C/0x3D; retry in 2s");i2c_recover();return;}
 uint8_t init[]={0,0xae,0xd5,0x80,0xa8,OLED_HEIGHT-1,0xd3,0,0x40,
  0x8d,0x14,0x20,2,0xa1,0xc8,0xda,0x12,0x81,oled_contrast,0xd9,0xf1,0xdb,0x40,0xa4,0xa6,0xaf};
 oled_ready=oled_write(init,sizeof init);oled_page=OLED_HEIGHT/8;oled_chunk=0;
 if(oled_ready){display_dirty=1;oled_force_redraw=1;oled_last_health=HAL_GetTick();}
 logmsg("OLED/INFO","Address=0x%02X SSD1306 128x64 trial ready=%u; verify screen visually",oled_address,oled_ready);
}
static void oled_next_page(void){
 for(oled_page=0;oled_page<OLED_HEIGHT/8;oled_page++)if(oled_dirty_pages&(1u<<oled_page))break;
 oled_chunk=0;
}
static void oled_begin_frame(void){
 oled_dirty_pages=0;
 for(unsigned p=0;p<OLED_HEIGHT/8;p++)if(oled_force_redraw||memcmp(oled_pixels+p*128,oled_sent+p*128,128)!=0)oled_dirty_pages|=(uint8_t)(1u<<p);
 oled_force_redraw=0;oled_next_page();
}
static void oled_poll(void){
 if(!oled_ready){
  if(oled_started && (uint32_t)(HAL_GetTick()-oled_last_attempt)>=2000)oled_init();
  return;
 }
 if((uint32_t)(HAL_GetTick()-oled_last_transfer)<2)return;
 if(oled_page>=OLED_HEIGHT/8){
  if((uint32_t)(HAL_GetTick()-oled_last_health)>=2000){
   oled_last_health=HAL_GetTick();uint8_t on[]={0,0xaf};oled_write(on,sizeof on);
  }
  return;
 }
 if(oled_chunk==0){uint8_t cmd[]={0,(uint8_t)(0xb0+oled_page),0,0x10};if(oled_write(cmd,sizeof cmd))oled_chunk=1;return;}
 uint8_t packet[17]={0x40};memcpy(packet+1,oled_pixels+oled_page*128+(oled_chunk-1)*16,16);
 if(!oled_write(packet,sizeof packet))return;
 memcpy(oled_sent+oled_page*128+(oled_chunk-1)*16,packet+1,16);
 if(++oled_chunk==9){oled_dirty_pages&=(uint8_t)~(1u<<oled_page);oled_next_page();if(oled_page==OLED_HEIGHT/8)oled_frames++;}
}
