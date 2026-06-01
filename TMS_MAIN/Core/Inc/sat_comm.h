/*
 * sat_comm.h
 *
 * TMS_MAIN SPI-master link to the TMS_SATELLITE boards ("send to main" stage).
 *
 * Each satellite is a dumb sensor: it samples 6 thermistors into a local
 * uint16_t[6] and keeps an SPI slave transaction permanently armed. The master
 * (this code) drives that satellite's chip-select low, clocks a fixed 12-byte
 * exchange, and reads back 6 little-endian uint16 raw ADC counts in T1..T6
 * order. No framing, no request byte, no CRC -- the byte layout falls out of
 * both ends being little-endian Cortex parts.
 *
 *   rx layout: [T1_lo,T1_hi, T2_lo,T2_hi, ... T6_lo,T6_hi]
 *
 * This module stops at raw counts. Converting raw ADC -> int8 °C and reducing
 * to per-module low/high/avg + IDs (the can_tms_module_data_t the CAN layer
 * wants) is the NEXT stage and lives elsewhere -- see the TODO in main.c.
 *
 * One CS line per satellite: hardware-NSS slaves cannot share a CS, so each
 * board needs its own GPIO. Today only satellite 0's CS (PA8) exists in the
 * .ioc; see sat_cs[] in sat_comm.c.
 */

#ifndef INC_SAT_COMM_H_
#define INC_SAT_COMM_H_

#include "main.h"
#include <stdint.h>

#define SAT_COMM_THERMS_PER_SAT   6u   /* T1..T6 raw counts per satellite */
#define SAT_COMM_FRAME_BYTES      (SAT_COMM_THERMS_PER_SAT * 2u)  /* 12 */

/* One satellite's most recent poll result. */
typedef struct {
    uint16_t raw[SAT_COMM_THERMS_PER_SAT];  /* raw ADC counts, T1..T6 order */
    uint8_t  valid;                         /* 1 if the last poll succeeded */
} sat_comm_reading_t;

extern SPI_HandleTypeDef hspi1;

/* Number of satellites we can actually address today (= populated CS lines).
 * Grows to 6 once the remaining CS GPIOs are assigned in CubeMX. */
uint8_t sat_comm_satellite_count(void);

/* Deassert every CS line (idle high). Call once after MX_GPIO_Init(). */
void sat_comm_init(void);

/* Poll one satellite: CS low -> 12-byte exchange -> CS high -> decode raw[].
 * Returns HAL_OK and fills *out (valid=1) on success; on SPI error fills
 * out->valid=0 and returns the HAL status. sat_index must be <
 * sat_comm_satellite_count(). */
HAL_StatusTypeDef sat_comm_poll(uint8_t sat_index, sat_comm_reading_t *out);

/* Poll all addressable satellites in turn into out[]. out must hold at least
 * sat_comm_satellite_count() entries. */
void sat_comm_poll_all(sat_comm_reading_t *out);

#endif /* INC_SAT_COMM_H_ */
