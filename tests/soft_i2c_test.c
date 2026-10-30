#include "soft_i2c.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned samples[128],count,index_,delays,clock_reads,stretch;
static int scl,sda,stuck_scl;
static void clock_(int release){scl=release;}
static void data_(int release){sda=release;}
static int read_clock(void){clock_reads++;if(stretch){stretch--;return 0;}return !stuck_scl;}
static int read_data(void){assert(index_<count);return (int)samples[index_++];}
static void delay_(void){delays++;}
static const SoftI2c bus={clock_,data_,read_clock,read_data,delay_};
static void reset(const unsigned *p,unsigned n){
 memcpy(samples,p,n*sizeof *p);count=n;index_=delays=clock_reads=stretch=0;stuck_scl=0;scl=sda=1;
}
int main(void){
 const uint8_t value=0xa5;
 unsigned write_ok[]={1,0,0,1};reset(write_ok,4);
 assert(soft_i2c_write(&bus,0x3c,&value,1)==0);assert(index_==4&&scl&&sda);
 unsigned nack[]={1,1,1};reset(nack,3);
 assert(soft_i2c_write(&bus,0x3c,&value,1)==SOFT_I2C_NACK);assert(index_==3&&scl&&sda);
 unsigned reg_write[]={1,0,0,0,1};reset(reg_write,5);
 assert(soft_i2c_write_register(&bus,0x68,15,&value,1)==0);assert(index_==5);
 unsigned read_ok[]={1,0,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0,1};
 uint8_t bytes[2];reset(read_ok,22);stretch=3;
 assert(soft_i2c_read_register(&bus,0x68,0,bytes,2)==0);
 assert(bytes[0]==0xa5&&bytes[1]==0x5a&&index_==22&&scl&&sda);
 unsigned held_data[]={0,0};reset(held_data,2);
 assert(soft_i2c_write(&bus,0x68,0,0)==SOFT_I2C_SDA_LOW);assert(scl&&sda);
 unsigned released[]={1};reset(released,1);stuck_scl=1;
 assert(soft_i2c_write(&bus,0x68,0,0)==SOFT_I2C_SCL_LOW);
 assert(clock_reads==400&&delays<410&&scl&&sda);
 reset(released,1);assert(soft_i2c_recover(&bus)==0);assert(clock_reads==10&&scl&&sda);
 reset(released,1);stuck_scl=1;
 assert(soft_i2c_recover(&bus)==SOFT_I2C_SCL_LOW);assert(clock_reads==400);
 puts("PASS: software I2C ACK/NACK, register writes, repeated-start reads, stretch, stuck-line bounds and recovery");
}
