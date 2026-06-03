/*
 * adc_thermistor.h
 *
 * Direct on-board thermistor acquisition for TMS_MAIN. No satellites, no SPI:
 * the 6 pack thermistors wire straight into this board's ADC (8 pins exist on
 * the schematic; 6 are populated, one per battery pack). We read all 6 raw
 * counts each cycle; conversion to °C + reduction to the J1939 frame happens
 * in thermistor.c.
 *
 * REQUIRES: ADC1 enabled in TMS_MAIN.ioc with the 6 thermistor channels, then
 * regenerate (produces adc.c defining `hadc1` + MX_ADC1_Init). Until that is
 * done this won't link (undefined `hadc1`). Set the real channels in the
 * THERM_CHANNELS table in adc_thermistor.c to match the .ioc pinout.
 */

#ifndef INC_ADC_THERMISTOR_H_
#define INC_ADC_THERMISTOR_H_

#include "main.h"
#include "can_tms.h"   /* CAN_TMS_THERMS_PER_MODULE = pack count (6) */
#include <stdint.h>

#define ADC_THERM_COUNT  CAN_TMS_THERMS_PER_MODULE  /* 6 packs */

extern ADC_HandleTypeDef hadc1;

void adc_thermistor_init(void);

/* Sample all 6 thermistor channels. raw[] gets the raw ADC counts, valid[] is
 * 1 per channel that converted OK (0 on timeout/error). Both arrays must hold
 * ADC_THERM_COUNT entries. */
void adc_thermistor_read_all(uint16_t *raw, uint8_t *valid);

#endif /* INC_ADC_THERMISTOR_H_ */
