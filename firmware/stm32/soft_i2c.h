#pragma once
#include <stdint.h>
/* Single-master, open-drain transport. delay() supplies a >=10 us half period.
 * Releasing a line must never drive it HIGH. External 3.3 V pull-ups required. */
typedef struct {
 void (*scl)(int release);
 void (*sda)(int release);
 int (*read_scl)(void);
 int (*read_sda)(void);
 void (*delay)(void);
} SoftI2c;
enum { SOFT_I2C_OK, SOFT_I2C_NACK, SOFT_I2C_SCL_LOW, SOFT_I2C_SDA_LOW };
int soft_i2c_write(const SoftI2c *b,uint8_t address,const uint8_t *p,unsigned n);
int soft_i2c_read_register(const SoftI2c *b,uint8_t address,uint8_t reg,uint8_t *p,unsigned n);
int soft_i2c_write_register(const SoftI2c *b,uint8_t address,uint8_t reg,const uint8_t *p,unsigned n);
int soft_i2c_recover(const SoftI2c *b);
