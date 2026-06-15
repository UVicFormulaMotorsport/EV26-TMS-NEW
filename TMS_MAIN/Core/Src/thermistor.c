/*
 * thermistor.c
 *
 * See thermistor.h. Reduction is done in the °C domain (convert first, then
 * min/max/avg) because the thermistor curve is nonlinear and, for an NTC, the
 * raw-count ordering can even be inverted relative to temperature.
 */

#include "thermistor.h"

#define ADS1256_FULL_SCALE_16BIT 32767
#define ADS1256_VREF_MV 2500

/* Table 5: Temperature-Voltage LUT for the thermistor
 * Covers -40 °C to 120 °C with 5 °C steps (33 entries). 1 °C/LSB on the wire, so we can store as int8_t in the frame.
 */
static const int16_t temp_c_lut[] = {
    -40, -35, -30, -25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40,
    45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100, 105, 110, 115, 120
};

static const uint16_t volt_mv_lut[] = {
    2440, 2420, 2400, 2380, 2350, 2320, 2270, 2230, 2170, 2110, 2050, 1990,
    1920, 1860, 1800, 1740, 1680, 1630, 1590, 1550, 1510, 1480, 1450, 1430,
    1400, 1380, 1370, 1350, 1340, 1330, 1320, 1310, 1300
};

#define TEMP_LUT_SIZE (sizeof(temp_c_lut) / sizeof(temp_c_lut[0]))

/* Convert raw ADC counts (upper 16 bits of 24-bit code) to millivolts.
 * Assuming ADS1256 with VREF = 2.5V. */
static uint16_t thermistor_raw_to_mv(uint16_t raw)
{
    uint32_t mv = ((uint32_t)raw * 6600) / 4095; //changed to reflect layout of borrowed VCU board
//    if (mv > ADS1256_VREF_MV) {
//        mv = ADS1256_VREF_MV;
//    }
    return (uint16_t)mv;
}

/* Linearly interpolate voltage to temperature using the LUT.
 * If voltage is below the coldest entry, return min temp.
 * If above the warmest, return max temp. */
static int8_t thermistor_mv_to_c(uint16_t mv)
{
    /* LUT is indexed from cold to warm, but voltage decreases with temperature
     * (NTC thermistor). So volt_mv_lut[0] is at temp_c_lut[0]=-40°C (2440 mV),
     * and volt_mv_lut[32] is at temp_c_lut[32]=120°C (1300 mV). */

    if (mv >= volt_mv_lut[0]) {
        /* Voltage higher than table range: clamp to coldest temperature. */
        return temp_c_lut[0];
    }
    if (mv <= volt_mv_lut[TEMP_LUT_SIZE - 1]) {
        /* Voltage lower than table range: clamp to hottest temperature. */
        return temp_c_lut[TEMP_LUT_SIZE - 1];
    }

    /* Find the two adjacent LUT points: volt_mv_lut[i] > mv >= volt_mv_lut[i+1] */
    for (uint8_t i = 0; i < TEMP_LUT_SIZE - 1; i++) {
        if (volt_mv_lut[i] >= mv && mv >= volt_mv_lut[i + 1]) {
            /* Linear interpolation: temp = T0 + (V0 - V) * (T1 - T0) / (V0 - V1) */
            int16_t temp0 = temp_c_lut[i];
            int16_t temp1 = temp_c_lut[i + 1];
            uint16_t v0 = volt_mv_lut[i];
            uint16_t v1 = volt_mv_lut[i + 1];

            int32_t dtemp = temp1 - temp0;  // temperature delta 
            uint32_t dv = v0 - v1;           // voltage delta (positive) 
            uint32_t v_offset = v0 - mv;    // voltage offset from v0

            int32_t interp = (v_offset * dtemp) / dv;
            int32_t result = temp0 + interp;

            if (result > 127) {
                result = 127;
            }
            if (result < -128) {
                result = -128;
            }
            return (int8_t)result;
        }
    }

    /* Should not reach here, but return a safe default. */
    return 0;
}

/* Convert raw ADC count to signed °C (1 °C/LSB on the wire). */
int8_t thermistor_raw_to_c(uint16_t raw)
{
    uint16_t mv = thermistor_raw_to_mv(raw);
    return thermistor_mv_to_c(mv);
}

void thermistor_build_module_data(const uint32_t *raw, const uint8_t *valid,
                                  can_tms_module_data_t *out)
{
    int16_t low = 127;
    int16_t high = -128;
    int32_t sum = 0;
    uint8_t count = 0;
    uint8_t high_id = 0;
    uint8_t low_id = 0;

    for (uint8_t seg = 0; seg < CAN_TMS_THERMS_PER_MODULE; seg++) {
        if (valid != NULL && !valid[seg]) {
            continue;   // skip a failed poll rather than feed in a fake 0 
        }

        int16_t c = thermistor_raw_to_c(raw[seg]);
        if (c > high) {
            high = c;
            high_id = seg;
        }
        if (c < low) {
            low = c;
            low_id = seg;
        }
        sum += c;
        count++;
    }

    if (count == 0) {
        /* No valid segments this round: emit a benign, self-consistent frame. */
        out->low_c = 0;
        out->high_c = 0;
        out->avg_c = 0;
    } else {
        out->low_c = (int8_t)low;
        out->high_c = (int8_t)high;
        out->avg_c = (int8_t)(sum / count);
    }

    out->therm_count = (uint8_t)CAN_TMS_THERMS_PER_MODULE;  // always 6 per spec 
    out->high_id = high_id;
    out->low_id = low_id;
}
