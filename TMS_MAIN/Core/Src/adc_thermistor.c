/*
 * adc_thermistor.c
 *
 * Per-channel polled sampling on hadc1. We reconfigure regular rank 1 to each
 * thermistor channel in turn (no scan mode / DMA needed at this cadence),
 * matching how the old satellite ADC worked — just on the main board now.
 *
 * Long sample time (480 cycles) because thermistor dividers are relatively
 * high-impedance and need settling time on the ADC sample cap.
 */

#include "adc_thermistor.h"

/* One ADC channel per battery pack. Array index == pack number (0..5), which
 * is what surfaces as the pack id in the J1939 frame (high_id/low_id), so the
 * ORDER must follow pack numbering. Use the HAL channel macros, not pin names:
 * on the STM32F407, ADC1 INx maps to pins PA0..PA5 = ADC_CHANNEL_0..5.
 * Verify these match the channels enabled in TMS_MAIN.ioc. */
static const uint32_t THERM_CHANNELS[ADC_THERM_COUNT] = {
    ADC_CHANNEL_0,   /* pack 0  -> PA0 */
    ADC_CHANNEL_1,   /* pack 1  -> PA1 */
    ADC_CHANNEL_2,   /* pack 2  -> PA2 */
    ADC_CHANNEL_3,   /* pack 3  -> PA3 */
    ADC_CHANNEL_4,   /* pack 4  -> PA4 */
    ADC_CHANNEL_5,   /* pack 5  -> PA5 */
};

void adc_thermistor_init(void)
{
    /* MX_ADC1_Init() (CubeMX) handles peripheral setup. F4 needs no explicit
     * calibration step. */
}

void adc_thermistor_read_all(uint16_t *raw, uint8_t *valid)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;

    for (int i = 0; i < ADC_THERM_COUNT; i++) {
        sConfig.Channel = THERM_CHANNELS[i];
        raw[i]   = 0;
        valid[i] = 0;

        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            continue;
        }
        if (HAL_ADC_Start(&hadc1) != HAL_OK) {
            continue;
        }
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            raw[i]   = (uint16_t)HAL_ADC_GetValue(&hadc1);
            valid[i] = 1;
        }
        HAL_ADC_Stop(&hadc1);
    }
}
