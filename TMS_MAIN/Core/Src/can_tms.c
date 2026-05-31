/*
 * can_tms.c
 *
 * See can_tms.h. Assumes CubeMX has provided MX_CAN1_Init() + MX_CAN2_Init()
 * and both handles at 500 kbps (Prescaler=4, BS1=12TQ, BS2=8TQ). Both buses
 * carry the same outgoing frames while the vehicle-side mapping is unsettled.
 *
 * On STM32F4 dual-CAN parts the 28 filter banks are split between CAN1 and
 * CAN2 by SlaveStartFilterBank. We give CAN1 bank 0 and CAN2 bank 14. Filters
 * accept everything; TMS is tx-only today, but the peripherals still need a
 * filter configured before HAL_CAN_Start() will accept frames into the RX
 * FIFOs in the future.
 */

#include "can_tms.h"
#include <stddef.h>

static CAN_TxHeaderTypeDef tx_header_addr_claim;
static CAN_TxHeaderTypeDef tx_header_therm_data;

static CAN_HandleTypeDef * const tx_buses[] = { &hcan1, &hcan2 };
#define TX_BUS_COUNT (sizeof(tx_buses) / sizeof(tx_buses[0]))

static void configure_bus(CAN_HandleTypeDef *hcan, uint32_t filter_bank)
{
    CAN_FilterTypeDef filter = {0};
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterIdHigh         = 0x0000;
    filter.FilterIdLow          = 0x0000;
    filter.FilterMaskIdHigh     = 0x0000;
    filter.FilterMaskIdLow      = 0x0000;
    filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    filter.FilterActivation     = ENABLE;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.FilterBank           = filter_bank;
    filter.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(hcan, &filter) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_CAN_Start(hcan) != HAL_OK) {
        Error_Handler();
    }
}

void can_tms_init(void)
{
    configure_bus(&hcan1, 0);
    configure_bus(&hcan2, 14);

    tx_header_addr_claim.StdId              = 0;
    tx_header_addr_claim.ExtId              = CAN_TMS_ADDR_CLAIM_ID;
    tx_header_addr_claim.IDE                = CAN_ID_EXT;
    tx_header_addr_claim.RTR                = CAN_RTR_DATA;
    tx_header_addr_claim.DLC                = 8;
    tx_header_addr_claim.TransmitGlobalTime = DISABLE;

    tx_header_therm_data.StdId              = 0;
    tx_header_therm_data.ExtId              = CAN_TMS_THERM_DATA_ID;
    tx_header_therm_data.IDE                = CAN_ID_EXT;
    tx_header_therm_data.RTR                = CAN_RTR_DATA;
    tx_header_therm_data.DLC                = 8;
    tx_header_therm_data.TransmitGlobalTime = DISABLE;
}

/* Publish one frame on every configured bus. Spins briefly when a bus's 3
 * hardware tx mailboxes are full; worst-case wait at 500 kbps is ~250 us per
 * frame, bounded. Returns the worst HAL status across buses. */
static HAL_StatusTypeDef send_on_all_buses(CAN_TxHeaderTypeDef *header,
                                           uint8_t *payload)
{
    HAL_StatusTypeDef worst = HAL_OK;
    uint32_t mailbox;

    for (size_t i = 0; i < TX_BUS_COUNT; i++) {
        while (HAL_CAN_GetTxMailboxesFreeLevel(tx_buses[i]) == 0) { /* spin */ }
        HAL_StatusTypeDef st = HAL_CAN_AddTxMessage(tx_buses[i], header,
                                                    payload, &mailbox);
        if (st != HAL_OK) {
            worst = st;
        }
    }
    return worst;
}

HAL_StatusTypeDef can_tms_send_address_claim(uint8_t module_index)
{
    uint8_t payload[8];

    /* Spec: bytes 1-3 = constant "unique identifier" {0xF3, 0x00, 0x80}.
     * Byte 4 = BMS target address (0xF3 default).
     * Byte 5 = module# << 3. Bytes 6-8 = constants {0x40, 0x1E, 0x90}. */
    payload[0] = 0xF3u;
    payload[1] = 0x00u;
    payload[2] = 0x80u;
    payload[3] = 0xF3u;
    payload[4] = (uint8_t)((module_index & 0x1Fu) << 3);
    payload[5] = 0x40u;
    payload[6] = 0x1Eu;
    payload[7] = 0x90u;

    return send_on_all_buses(&tx_header_addr_claim, payload);
}

HAL_StatusTypeDef can_tms_send_module_data(uint8_t module_index,
                                           const can_tms_module_data_t *data)
{
    uint8_t  payload[8];
    uint16_t sum;

    payload[0] = module_index;
    payload[1] = (uint8_t)data->low_c;
    payload[2] = (uint8_t)data->high_c;
    payload[3] = (uint8_t)data->avg_c;
    payload[4] = data->therm_count;
    payload[5] = data->high_id;
    payload[6] = data->low_id;

    /* Checksum byte = low 8 bits of (sum of bytes 1..7 + 0x39 + length). */
    sum = (uint16_t)payload[0] + payload[1] + payload[2] + payload[3]
        + payload[4] + payload[5] + payload[6] + 0x39u + 0x08u;
    payload[7] = (uint8_t)(sum & 0xFFu);

    return send_on_all_buses(&tx_header_therm_data, payload);
}
