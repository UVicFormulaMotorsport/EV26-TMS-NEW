/*
 * ads1256.h
 *
 * External thermistor front-end for TMS_MAIN: a TI ADS1256 24-bit delta-sigma
 * ADC read over SPI1 (master). This REPLACES the old on-chip ADC path
 * (adc_thermistor.c) — the STM32 ADC is no longer used. Everything downstream
 * is unchanged: we still hand thermistor.c a raw[6]/valid[6] pair, one entry
 * per battery pack, and it converts + reduces to the single Orion J1939 frame.
 *
 * Wiring (TMS_MAIN.ioc):
 *   PA5  SPI1_SCK     PA6  SPI1_MISO (ADS DOUT)   PA7  SPI1_MOSI (ADS DIN)
 *   PA8  CS   (GPIO out, active low)     PD8  DRDY (GPIO in, active low)
 *   ADS1256 AIN0..AIN5 = packs 0..5 single-ended vs AINCOM.
 *
 * SPI must be mode 1 (CPOL=0, CPHA=2Edge) and <= ~1.9 MHz SCLK — set in the
 * .ioc to /128 (~0.66 MHz). Mode 0 / fast clock will NOT communicate.
 */

#ifndef INC_ADS1256_H_
#define INC_ADS1256_H_

#include "main.h"
#include "can_tms.h"   /* CAN_TMS_THERMS_PER_MODULE = pack count (6) */
#include <stdint.h>

#define ADS1256_THERM_COUNT  CAN_TMS_THERMS_PER_MODULE  /* 6 packs, AIN0..AIN5 */

extern SPI_HandleTypeDef hspi1;

/* Reset + configure the ADS1256 (rate, gain, buffer) and self-calibrate.
 * Call once after MX_SPI1_Init(). */
void ads1256_init(void);

/* Read all 6 single-ended channels (AIN0..AIN5). raw[i] gets the upper 16 bits
 * of the channel's 24-bit code (see note in ads1256.c); valid[i] is 1 when the
 * channel converted within timeout, 0 otherwise. Both arrays must hold
 * ADS1256_THERM_COUNT entries. Drop-in replacement for adc_thermistor_read_all. */
void ads1256_read_all(uint16_t *raw, uint8_t *valid);

#endif /* INC_ADS1256_H_ */
