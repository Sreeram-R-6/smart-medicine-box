#include "soft_i2c.h"
volatile uint32_t soft_i2c_write_failed_byte;
static int high(const SoftI2c *b){
 b->scl(1);
 for(unsigned i=0;i<200;i++){if(b->read_scl()){b->delay();return 0;}b->delay();}
 return SOFT_I2C_SCL_LOW;
}
static int stop(const SoftI2c *b){
 b->scl(0);b->sda(0);b->delay();int e=high(b);
 b->sda(1);b->delay();
 if(e)return e;
 return b->read_sda()?0:SOFT_I2C_SDA_LOW;
}
static int start(const SoftI2c *b){
 b->sda(1);b->delay();int e=high(b);if(e)return e;
 if(!b->read_sda())return SOFT_I2C_SDA_LOW;
 b->sda(0);b->delay();b->scl(0);return 0;
}
static int put(const SoftI2c *b,uint8_t x){
 for(unsigned i=0;i<8;i++){
  b->sda(!!(x&0x80));b->delay();int e=high(b);if(e)return e;
  b->scl(0);x<<=1;
 }
 b->sda(1);b->delay();int e=high(b);if(e)return e;
 int ack=!b->read_sda();b->scl(0);return ack?0:SOFT_I2C_NACK;
}
static int get(const SoftI2c *b,uint8_t *x,int last){
 *x=0;b->sda(1);
 for(unsigned i=0;i<8;i++){
  b->delay();int e=high(b);if(e)return e;
  *x=(uint8_t)((*x<<1)|!!b->read_sda());b->scl(0);
 }
 b->sda(last);b->delay();int e=high(b);b->scl(0);b->sda(1);return e;
}
static int finish(const SoftI2c *b,int error){int e=stop(b);return error?error:e;}
int soft_i2c_write(const SoftI2c *b,uint8_t a,const uint8_t *p,unsigned n){
 int e=start(b);if(e)soft_i2c_write_failed_byte=UINT32_MAX;
 if(!e){e=put(b,(uint8_t)(a<<1));if(e)soft_i2c_write_failed_byte=0;}
 for(unsigned i=0;!e&&i<n;i++){e=put(b,p[i]);if(e)soft_i2c_write_failed_byte=i+1;}
 int result=finish(b,e);if(!e&&result)soft_i2c_write_failed_byte=UINT32_MAX;
 return result;
}
int soft_i2c_write_register(const SoftI2c *b,uint8_t a,uint8_t reg,const uint8_t *p,unsigned n){
 int e=start(b);if(!e)e=put(b,(uint8_t)(a<<1));if(!e)e=put(b,reg);
 for(unsigned i=0;!e&&i<n;i++)e=put(b,p[i]);
 return finish(b,e);
}
int soft_i2c_read_register(const SoftI2c *b,uint8_t a,uint8_t reg,uint8_t *p,unsigned n){
 int e=start(b);if(!e)e=put(b,(uint8_t)(a<<1));if(!e)e=put(b,reg);
 if(!e)e=start(b);
 if(!e)e=put(b,(uint8_t)((a<<1)|1));
 for(unsigned i=0;!e&&i<n;i++)e=get(b,p+i,i+1==n);
 return finish(b,e);
}
int soft_i2c_recover(const SoftI2c *b){
 b->sda(1);int e=0;
 for(unsigned i=0;i<9;i++){b->scl(0);b->delay();e=high(b);if(e)break;}
 int tail=stop(b);return e?e:tail;
}
