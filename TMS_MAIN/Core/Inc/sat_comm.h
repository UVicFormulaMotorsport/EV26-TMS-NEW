/*
 * sat_comm.h
 *
 * TMS_MAIN SPI-master link to the TMS_SATELLITE boards over ONE shared bus.
 *
 * All six satellites share SCK/MOSI/MISO and a single CS line (their NSS pins
 * are tied together). The master cannot pick a talker with chip-select, so it
 * names one by sequence number instead and the satellites gate their own MISO
 * (see sat_spi.h on the satellite for the RXONLY mechanism).
 *
 * Each satellite reports ONE value: the hottest raw ADC count in its battery
 * pack (a uint16, the Enepaq VTC6 modules expose only a hottest-cell reading).
 *
 * Poll of segment k is two phases, master-clocked, framed by the shared CS:
 *   Phase A: CS low, clock [SAT_ADDR_HEADER, k], CS high.   (address)
 *   <gap so segment k can drop RXONLY and arm its reply>
 *   Phase B: CS low, clock 2 dummy bytes, read 2 bytes, CS high.  (data)
 *   raw = rx[0] | (rx[1] << 8).
 *
 * Converting raw -> int8 °C and reducing the 6 segments to the J1939 frame
 * lives in thermistor.c, called from main.c.
 */

#ifndef INC_SAT_COMM_H_
#define INC_SAT_COMM_H_

#include "main.h"
#include <stdint.h>

#define SAT_COMM_SEGMENT_COUNT   6u    /* battery packs / satellites, seg_id 0..5 */
#define SAT_COMM_ADDR_HEADER     0xA5u /* must match SAT_ADDR_HEADER on satellite */
#define SAT_COMM_ADDR_BYTES      2u
#define SAT_COMM_DATA_BYTES      2u

extern SPI_HandleTypeDef hspi1;

/* Idle the shared CS line high. Call once after MX_GPIO_Init(). */
void sat_comm_init(void);

/* Poll one segment (seg_id 0..5). On success returns HAL_OK and writes the raw
 * hottest count to *raw_out. On SPI error returns the HAL status and leaves
 * *raw_out unchanged. */
HAL_StatusTypeDef sat_comm_poll(uint8_t seg_id, uint16_t *raw_out);

/* Poll segments 0..SAT_COMM_SEGMENT_COUNT-1 into raw_out[] (must hold that
 * many). valid_out[i] is set to 1/0 per segment. Either array may be NULL only
 * if you don't need it — both are expected in normal use. */
void sat_comm_poll_round(uint16_t *raw_out, uint8_t *valid_out);

#endif /* INC_SAT_COMM_H_ */
