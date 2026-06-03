/*
 * sat_spi.c
 *
 * Shared-bus SPI slave with RXONLY-gated MISO. See sat_spi.h for the protocol.
 *
 * State machine, driven entirely by HAL SPI IT callbacks:
 *
 *   LISTEN  (RXONLY=1, MISO high-Z): armed with HAL_SPI_Receive_IT for a
 *           2-byte address frame. On completion (RxCplt):
 *             - [SAT_ADDR_HEADER, SAT_SEG_ID]  -> we're addressed: load reply,
 *               drop RXONLY, arm HAL_SPI_TransmitReceive_IT, go RESPOND.
 *             - anything else                  -> re-arm LISTEN.
 *   RESPOND (RXONLY=0, MISO driven): armed with HAL_SPI_TransmitReceive_IT for
 *           the 2-byte data frame. On completion (TxRxCplt): restore RXONLY,
 *           re-arm LISTEN.
 *
 * RXONLY is toggled by writing SPI_CR1 directly with the peripheral disabled
 * (the only safe time to touch CR1). We deliberately do NOT call HAL_SPI_Init()
 * to switch direction — that re-runs HAL_SPI_MspInit() (GPIO/clock setup) on
 * every poll. Init.Direction is kept in sync so the HAL transfer calls take the
 * matching code path.
 *
 * BENCH-VALIDATE (logic analyzer): (1) during phase A, MISO is high-Z on ALL
 * boards; (2) during phase B, exactly one board drives MISO; (3) the master's
 * inter-phase gap (sat_comm.c) is longer than this ISR's reconfigure time.
 */

#include "sat_spi.h"
#include "sat_adc.h"

static uint8_t addr_rx[SAT_SPI_ADDR_BYTES];
static uint8_t data_tx[SAT_SPI_DATA_BYTES];
static uint8_t data_rx[SAT_SPI_DATA_BYTES];

/* Toggle RXONLY (MISO release vs drive). CR1 is writable only with SPE=0. */
static void spi_set_rxonly(uint8_t on)
{
    __HAL_SPI_DISABLE(&hspi1);
    if (on) {
        SET_BIT(hspi1.Instance->CR1, SPI_CR1_RXONLY);
        hspi1.Init.Direction = SPI_DIRECTION_2LINES_RXONLY;
    } else {
        CLEAR_BIT(hspi1.Instance->CR1, SPI_CR1_RXONLY);
        hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    }
    __HAL_SPI_ENABLE(&hspi1);
}

/* Enter LISTEN: MISO released, armed for a 2-byte address frame. */
static void arm_listen(void)
{
    spi_set_rxonly(1u);
    (void)HAL_SPI_Receive_IT(&hspi1, addr_rx, SAT_SPI_ADDR_BYTES);
}

/* Enter RESPOND: load this pack's hottest raw (LE), drive MISO, arm data frame. */
static void arm_respond(void)
{
    uint16_t raw = sat_adc_hottest_raw();
    data_tx[0] = (uint8_t)(raw & 0xFFu);
    data_tx[1] = (uint8_t)((raw >> 8) & 0xFFu);

    spi_set_rxonly(0u);
    (void)HAL_SPI_TransmitReceive_IT(&hspi1, data_tx, data_rx, SAT_SPI_DATA_BYTES);
}

void sat_spi_init(void)
{
    arm_listen();
}

/* Phase-A completion: decode the address frame. */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance != SPI1) {
        return;
    }
    if (addr_rx[0] == SAT_ADDR_HEADER && addr_rx[1] == (uint8_t)SAT_SEG_ID) {
        arm_respond();      /* it's our turn */
    } else {
        arm_listen();       /* not addressed (or stray phase-B bytes): keep listening */
    }
}

/* Phase-B completion: we just drove our reply; release the bus. */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance != SPI1) {
        return;
    }
    arm_listen();
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        HAL_SPI_Abort(hspi);
        arm_listen();       /* resync: back to releasing MISO and listening */
    }
}
