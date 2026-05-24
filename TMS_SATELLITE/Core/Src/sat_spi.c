/*
 * sat_spi.c
 *
 * Uses HAL_SPI_TransmitReceive_IT in slave mode. The peripheral is armed once
 * at boot and re-armed at the end of every completed master transaction, so
 * the master can poll at any time without coordinating with the slave's
 * foreground work.
 *
 * Cortex-M3 is little-endian, so casting the uint16_t reading buffer to
 * uint8_t* and shifting it out byte-by-byte gives [T1_lo, T1_hi, T2_lo,
 * T2_hi, ...] on the wire — matches what TMS_MAIN expects to read.
 */

#include "sat_spi.h"
#include "sat_adc.h"
#include <string.h>

static uint8_t spi_tx_buffer[SAT_SPI_PAYLOAD_BYTES];
static uint8_t spi_rx_buffer[SAT_SPI_PAYLOAD_BYTES];

static void arm_next_transaction(void)
{
    /* Snapshot the current ADC readings into the TX buffer. Because
     * sat_adc_sample_all() publishes its buffer with IRQs disabled, this
     * memcpy always sees a consistent 12-byte set. */
    memcpy(spi_tx_buffer, (const void *)sat_adc_thermistor_raw, SAT_SPI_PAYLOAD_BYTES);

    (void)HAL_SPI_TransmitReceive_IT(&hspi1, spi_tx_buffer, spi_rx_buffer, SAT_SPI_PAYLOAD_BYTES);
}

void sat_spi_init(void)
{
    arm_next_transaction();
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        arm_next_transaction();
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        HAL_SPI_Abort(hspi);
        arm_next_transaction();
    }
}
