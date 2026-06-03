/*
 * sat_adc.c
 *
 * The current .ioc only configures regular rank 1 = ADC_CHANNEL_0. We
 * reconfigure rank 1 to each thermistor channel in turn rather than
 * requiring the .ioc be re-keyed for scan mode. Faster sampling can be
 * achieved later by switching to scan mode + DMA, but this is fine for the
 * 4 Hz master poll cadence.
 */

#include "sat_adc.h"
#include <string.h>

volatile uint16_t sat_adc_thermistor_raw[SAT_ADC_THERMISTOR_COUNT] = {0};

static const uint32_t thermistor_channels[SAT_ADC_THERMISTOR_COUNT] = {
    ADC_CHANNEL_0,
    ADC_CHANNEL_1,
    ADC_CHANNEL_2,
    ADC_CHANNEL_3,
    ADC_CHANNEL_8,
    ADC_CHANNEL_9,
};

void sat_adc_init(void)
{
    /* MX_ADC_Init() (CubeMX) handles peripheral setup. No L1-specific
     * calibration step is required here. */
}

void sat_adc_sample_all(void)
{
    uint16_t local[SAT_ADC_THERMISTOR_COUNT];
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_4CYCLES;

    for (int i = 0; i < SAT_ADC_THERMISTOR_COUNT; i++) {
        sConfig.Channel = thermistor_channels[i];

        if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK) {
            local[i] = 0;
            continue;
        }
        if (HAL_ADC_Start(&hadc) != HAL_OK) {
            local[i] = 0;
            continue;
        }
        if (HAL_ADC_PollForConversion(&hadc, 10) == HAL_OK) {
            local[i] = (uint16_t)HAL_ADC_GetValue(&hadc);
        } else {
            local[i] = 0;
        }
        HAL_ADC_Stop(&hadc);
    }

    /* Publish the new set atomically so the SPI ISR never snapshots a
     * half-updated buffer. */
    __disable_irq();
    memcpy((void *)sat_adc_thermistor_raw, local, sizeof(local));
    __enable_irq();
}

uint16_t sat_adc_hottest_raw(void)
{
    /* See the POLARITY CAVEAT in sat_adc.h. Currently: hottest == max raw. */
    uint16_t hottest = 0;

    __disable_irq();
    for (int i = 0; i < SAT_ADC_THERMISTOR_COUNT; i++) {
        if (sat_adc_thermistor_raw[i] > hottest) {
            hottest = sat_adc_thermistor_raw[i];
        }
    }
    __enable_irq();

    return hottest;
}
