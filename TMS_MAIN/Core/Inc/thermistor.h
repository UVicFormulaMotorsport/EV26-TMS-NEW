/*
 * thermistor.h
 *
 * Stage 3: raw ADC -> temperature, and reduction of the 6 battery packs into
 * the single Orion thermistor-module data frame TMS_MAIN broadcasts.
 *
 * The 6 segment values are each a pack's *hottest* reading, so the resulting
 * low/avg run warm relative to true per-cell data (high is exact). This coarse
 * picture is an accepted, hardware-constrained design limit (the VTC6 modules
 * only expose hottest-cell). See the J1939 spec: bytes low/high/avg + IDs.
 *
 * !! The raw->°C curve here is an UNCALIBRATED PLACEHOLDER. It must be replaced
 *    with the real calibration (LUT or Beta/Steinhart-Hart + divider R, Vref,
 *    ADC bits) from the chief eng before this means anything. !!
 */

#ifndef INC_THERMISTOR_H_
#define INC_THERMISTOR_H_

#include "can_tms.h"
#include <stdint.h>

/* Convert one raw ADC count to signed °C (1 °C/LSB on the wire). PLACEHOLDER. */
int8_t thermistor_raw_to_c(uint16_t raw);

/* Convert + reduce a full poll round into the J1939 module payload. raw[] and
 * valid[] are SAT_COMM_SEGMENT_COUNT long; invalid segments are excluded from
 * low/high/avg. therm_count is always 6 per spec. high_id/low_id index the
 * hottest/coldest segment (0..5). */
void thermistor_build_module_data(const uint16_t *raw, const uint8_t *valid,
                                  can_tms_module_data_t *out);

#endif /* INC_THERMISTOR_H_ */
