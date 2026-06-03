/*
 * thermistor.c
 *
 * See thermistor.h. Reduction is done in the °C domain (convert first, then
 * min/max/avg) because the thermistor curve is nonlinear and, for an NTC, the
 * raw-count ordering can even be inverted relative to temperature.
 */

#include "thermistor.h"

#warning "thermistor_raw_to_c() is an UNCALIBRATED PLACEHOLDER - replace with the real curve before relying on TMS temperatures"

int8_t thermistor_raw_to_c(uint16_t raw)
{
    /* PLACEHOLDER ONLY. Linear stand-in so the pipeline produces plausible,
     * monotonic, in-range numbers for bench/CAN-analyzer bring-up. It is NOT a
     * real temperature. Replace with the calibrated conversion (LUT or
     * Beta/Steinhart-Hart) once the curve + divider details are available.
     *
     * 12-bit raw (0..4095) -> 0..~127 °C via >>5. */
    int32_t c = (int32_t)(raw >> 5);
    if (c > 127) {
        c = 127;
    }
    return (int8_t)c;
}

void thermistor_build_module_data(const uint16_t *raw, const uint8_t *valid,
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
            continue;   /* skip a failed poll rather than feed in a fake 0 */
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

    out->therm_count = (uint8_t)CAN_TMS_THERMS_PER_MODULE;  /* always 6 per spec */
    out->high_id = high_id;
    out->low_id = low_id;
}
