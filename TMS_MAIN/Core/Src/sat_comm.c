/*
 * sat_comm.c
 *
 * See sat_comm.h. SPI master, shared-bus two-phase addressed poll.
 *
 * The single CS line is the generated CS_Pin/CS_GPIO_Port (PA8); every
 * satellite NSS ties to it. CS is active-low and framed per phase so each
 * satellite sees clean NSS edges. Between the address and data phases the
 * master waits poll_gap() to let the addressed satellite's ISR drop RXONLY and
 * arm its reply — that gap MUST exceed the satellite reconfigure time (verify
 * on a scope; widen POLL_GAP_LOOPS if phase B reads zeros/garbage).
 */

#include "sat_comm.h"

/* Blocking exchange timeout. The transfer itself is microseconds; this only
 * guards against a wedged/absent bus. */
#define SAT_COMM_SPI_TIMEOUT_MS  5u

/* Inter-phase gap. ~32k NOP-ish iterations at 168 MHz is a few hundred µs,
 * comfortably longer than the satellite's IT reconfigure. Tune on hardware. */
#define POLL_GAP_LOOPS           40000u

static void busy_loops(volatile uint32_t n)
{
    while (n--) {
        __NOP();
    }
}

/* Short CS setup/hold so the slave's NSS edge lands before/after clocking. */
static void cs_settle(void)
{
    busy_loops(32u);
}

static inline void cs_assert(void)
{
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);  /* active low */
    cs_settle();
}

static inline void cs_deassert(void)
{
    cs_settle();
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}

void sat_comm_init(void)
{
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);  /* idle high */
}

HAL_StatusTypeDef sat_comm_poll(uint8_t seg_id, uint16_t *raw_out)
{
    uint8_t addr[SAT_COMM_ADDR_BYTES] = { SAT_COMM_ADDR_HEADER, seg_id };
    uint8_t tx[SAT_COMM_DATA_BYTES]   = {0};
    uint8_t rx[SAT_COMM_DATA_BYTES]   = {0};
    HAL_StatusTypeDef st;

    if (seg_id >= SAT_COMM_SEGMENT_COUNT || raw_out == NULL) {
        return HAL_ERROR;
    }

    /* Phase A: broadcast the address. Every satellite receives it (MISO
     * released); only segment seg_id arms a reply. */
    cs_assert();
    st = HAL_SPI_Transmit(&hspi1, addr, SAT_COMM_ADDR_BYTES, SAT_COMM_SPI_TIMEOUT_MS);
    cs_deassert();
    if (st != HAL_OK) {
        return st;
    }

    /* Let segment seg_id drop RXONLY and arm its data reply. */
    busy_loops(POLL_GAP_LOOPS);

    /* Phase B: clock the reply out of the one armed satellite. */
    cs_assert();
    st = HAL_SPI_TransmitReceive(&hspi1, tx, rx, SAT_COMM_DATA_BYTES,
                                 SAT_COMM_SPI_TIMEOUT_MS);
    cs_deassert();
    if (st != HAL_OK) {
        return st;
    }

    *raw_out = (uint16_t)rx[0] | ((uint16_t)rx[1] << 8);
    return HAL_OK;
}

void sat_comm_poll_round(uint16_t *raw_out, uint8_t *valid_out)
{
    for (uint8_t seg = 0; seg < SAT_COMM_SEGMENT_COUNT; seg++) {
        uint16_t raw = 0;
        HAL_StatusTypeDef st = sat_comm_poll(seg, &raw);

        if (raw_out != NULL) {
            raw_out[seg] = (st == HAL_OK) ? raw : 0u;
        }
        if (valid_out != NULL) {
            valid_out[seg] = (st == HAL_OK) ? 1u : 0u;
        }

        /* Small gap so segment `seg` fully reverts to LISTEN before the next
         * address frame goes out. */
        busy_loops(POLL_GAP_LOOPS);
    }
}
