/*
 * sat_adc.h
 *
 * Reads the six thermistor channels (T1..T6) on the satellite into a shared
 * raw-reading buffer. Channels per the .ioc pinout:
 *   T1 = ADC_CH0  (PA0)
 *   T2 = ADC_CH1  (PA1)
 *   T3 = ADC_CH2  (PA2)
 *   T4 = ADC_CH3  (PA3)
 *   T5 = ADC_CH8  (PB0)
 *   T6 = ADC_CH9  (PB1)
 *
 * The buffer is published as little-endian uint16_t in memory, which is also
 * the wire format the SPI slave streams to TMS_MAIN.
 */

#ifndef INC_SAT_ADC_H_
#define INC_SAT_ADC_H_

#include "main.h"
#include <stdint.h>

#define SAT_ADC_THERMISTOR_COUNT  6

extern ADC_HandleTypeDef hadc;

extern volatile uint16_t sat_adc_thermistor_raw[SAT_ADC_THERMISTOR_COUNT];

void sat_adc_init(void);
void sat_adc_sample_all(void);

#endif /* INC_SAT_ADC_H_ */
