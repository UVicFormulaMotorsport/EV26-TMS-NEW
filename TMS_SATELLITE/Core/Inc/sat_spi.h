/*
 * sat_spi.h
 *
 * SPI slave protocol. Hardware NSS slave mode (per .ioc): when TMS_MAIN
 * asserts the satellite's NSS line and clocks 12 bytes, the slave streams
 * the latest six raw thermistor readings.
 *
 *   TX (slave -> master): 12 bytes = 6 × uint16_t raw ADC counts, little-endian
 *   RX (master -> slave): ignored
 *
 * The slave is permanently armed: each completed transaction re-arms the
 * next one from the TxRxCplt callback.
 */

#ifndef INC_SAT_SPI_H_
#define INC_SAT_SPI_H_

#include "main.h"
#include <stdint.h>

#define SAT_SPI_PAYLOAD_BYTES  12

extern SPI_HandleTypeDef hspi1;

void sat_spi_init(void);

#endif /* INC_SAT_SPI_H_ */
