/*
 * sat_spi.h
 *
 * Shared-bus SPI slave protocol (one bus for all 6 satellites).
 *
 * HARDWARE REALITY: every satellite's NSS ties to a single shared CS line from
 * TMS_MAIN, and MISO/MOSI/SCK are common. So whenever the master clocks, ALL
 * satellites are selected at once. To keep exactly one talker, idle satellites
 * sit in RX-only mode (SPI_CR1 RXONLY = 1) which RECEIVES but releases MISO
 * (high-Z) — the ST reference manual's documented "multislave" use of RXONLY.
 * Only the addressed satellite drops RXONLY to drive its reply, then restores
 * it. No GPIO pin remux, no per-board CS, no FreeRTOS, no ISO-SPI.
 *
 * Two-phase addressed poll (master drives both phases; see sat_comm.c):
 *   Phase A (address): master clocks [SAT_ADDR_HEADER, seg_id]. All satellites
 *     receive it with MISO released. The one whose SAT_SEG_ID == seg_id arms a
 *     reply (drops RXONLY); the rest stay listening.
 *   Phase B (data): master clocks 2 dummy bytes. Only the addressed satellite
 *     drives MISO with its 2-byte little-endian hottest raw ADC count; it then
 *     restores RXONLY. Everyone re-arms for the next address.
 *
 * The header byte lets a desynced satellite ignore phase-B bytes (they don't
 * start with SAT_ADDR_HEADER) and resync on the next real address frame.
 *
 * SAT_SEG_ID identifies this board's battery pack (0..5). Set it per board at
 * flash time (e.g. -DSAT_SEG_ID=3 in the build, or override below).
 */

#ifndef INC_SAT_SPI_H_
#define INC_SAT_SPI_H_

#include "main.h"
#include <stdint.h>

#ifndef SAT_SEG_ID
#define SAT_SEG_ID            0u    /* THIS board's pack index (0..5). Per-board. */
#endif

#define SAT_ADDR_HEADER       0xA5u /* phase-A frame marker */
#define SAT_SPI_ADDR_BYTES    2u    /* [header, seg_id] */
#define SAT_SPI_DATA_BYTES    2u    /* uint16 hottest raw, little-endian */

extern SPI_HandleTypeDef hspi1;

void sat_spi_init(void);

#endif /* INC_SAT_SPI_H_ */
