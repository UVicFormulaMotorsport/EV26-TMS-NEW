/*
 * sat_comm.c
 *
 * See sat_comm.h. SPI master polling of the TMS_SATELLITE boards.
 *
 * CS is active-low. The master owns the timing: it drives the target CS low,
 * issues a blocking HAL_SPI_TransmitReceive (it is the one generating SCK, so
 * no need to coordinate with the slave's foreground ADC loop), then raises CS.
 * TX content is don't-care; the slave ships its raw[] regardless.
 */

#include "sat_comm.h"
#include <string.h>

/* Blocking exchange timeout. The transfer itself is microseconds; this only
 * guards against a wedged/absent satellite. */
#define SAT_COMM_SPI_TIMEOUT_MS  5u

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
} sat_cs_t;

/* One entry per addressable satellite. ORDER == module index 0..5.
 *
 * Only satellite 0's CS (PA8, generated as CS_Pin/CS_GPIO_Port) exists in the
 * .ioc today. When the remaining 5 CS GPIOs are added in CubeMX, append them
 * here in module order, e.g.:
 *     { CS1_GPIO_Port, CS1_Pin },   // satellite 1
 *     ... through satellite 5
 * sat_comm_satellite_count() tracks this array's length automatically. */
static const sat_cs_t sat_cs[] = {
    { CS_GPIO_Port, CS_Pin },   /* satellite 0 */
};

#define SAT_CS_COUNT  (sizeof(sat_cs) / sizeof(sat_cs[0]))

/* Short CS setup/hold so the slave's NSS edge is seen before/after clocking.
 * A handful of cycles at 168 MHz; kept as a tiny busy-wait to avoid pulling in
 * HAL_Delay's 1 ms granularity. */
static void cs_settle(void)
{
    for (volatile uint32_t i = 0; i < 32; i++) {
        __NOP();
    }
}

static inline void cs_assert(const sat_cs_t *cs)
{
    HAL_GPIO_WritePin(cs->port, cs->pin, GPIO_PIN_RESET);  /* active low */
    cs_settle();
}

static inline void cs_deassert(const sat_cs_t *cs)
{
    cs_settle();
    HAL_GPIO_WritePin(cs->port, cs->pin, GPIO_PIN_SET);
}

uint8_t sat_comm_satellite_count(void)
{
    return (uint8_t)SAT_CS_COUNT;
}

void sat_comm_init(void)
{
    /* Idle all CS lines high (deasserted). MX_GPIO_Init leaves CS low. */
    for (size_t i = 0; i < SAT_CS_COUNT; i++) {
        HAL_GPIO_WritePin(sat_cs[i].port, sat_cs[i].pin, GPIO_PIN_SET);
    }
}

HAL_StatusTypeDef sat_comm_poll(uint8_t sat_index, sat_comm_reading_t *out)
{
    uint8_t tx[SAT_COMM_FRAME_BYTES] = {0};
    uint8_t rx[SAT_COMM_FRAME_BYTES] = {0};

    if (sat_index >= SAT_CS_COUNT || out == NULL) {
        if (out != NULL) {
            out->valid = 0;
        }
        return HAL_ERROR;
    }

    const sat_cs_t *cs = &sat_cs[sat_index];

    cs_assert(cs);
    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(&hspi1, tx, rx,
                                                   SAT_COMM_FRAME_BYTES,
                                                   SAT_COMM_SPI_TIMEOUT_MS);
    cs_deassert(cs);

    if (st != HAL_OK) {
        out->valid = 0;
        return st;
    }

    /* rx is 6 little-endian uint16 in T1..T6 order; both ends are LE so a
     * straight copy reproduces the satellite's raw[] array. */
    memcpy(out->raw, rx, SAT_COMM_FRAME_BYTES);
    out->valid = 1;
    return HAL_OK;
}

void sat_comm_poll_all(sat_comm_reading_t *out)
{
    for (uint8_t i = 0; i < SAT_CS_COUNT; i++) {
        (void)sat_comm_poll(i, &out[i]);
    }
}
