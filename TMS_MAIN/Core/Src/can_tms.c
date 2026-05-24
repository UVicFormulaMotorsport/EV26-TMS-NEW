/*
 * can_tms.c
 *
 * See can_tms.h for the on-wire format. Assumes CubeMX has already provided
 * MX_CAN2_Init() and the hcan2 handle (i.e. .ioc has been regenerated with the
 * 500 kbps timing). can_tms_init() only does the application-layer setup:
 * filter, start, and pre-built TxHeaders.
 */

#include "can_tms.h"

static CAN_TxHeaderTypeDef tx_header_seg_0_3;
static CAN_TxHeaderTypeDef tx_header_seg_4_5;

void can_tms_init(void)
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
    filter.FilterBank           = 14;
    filter.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(&hcan2, &filter) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_CAN_Start(&hcan2) != HAL_OK) {
        Error_Handler();
    }

    tx_header_seg_0_3.StdId              = CAN_TMS_FRAME_SEG_0_3_ID;
    tx_header_seg_0_3.ExtId              = 0;
    tx_header_seg_0_3.IDE                = CAN_ID_STD;
    tx_header_seg_0_3.RTR                = CAN_RTR_DATA;
    tx_header_seg_0_3.DLC                = 8;
    tx_header_seg_0_3.TransmitGlobalTime = DISABLE;

    tx_header_seg_4_5.StdId              = CAN_TMS_FRAME_SEG_4_5_ID;
    tx_header_seg_4_5.ExtId              = 0;
    tx_header_seg_4_5.IDE                = CAN_ID_STD;
    tx_header_seg_4_5.RTR                = CAN_RTR_DATA;
    tx_header_seg_4_5.DLC                = 4;
    tx_header_seg_4_5.TransmitGlobalTime = DISABLE;
}

static void pack_le_int16(uint8_t *dst, int16_t value)
{
    uint16_t raw = (uint16_t)value;
    dst[0] = (uint8_t)(raw & 0xFF);
    dst[1] = (uint8_t)((raw >> 8) & 0xFF);
}

HAL_StatusTypeDef can_tms_send_segment_temps(const int16_t segment_temp_decideg[CAN_TMS_SEG_COUNT])
{
    uint8_t  payload_seg_0_3[8];
    uint8_t  payload_seg_4_5[4];
    uint32_t mailbox;

    for (int i = 0; i < 4; i++) {
        pack_le_int16(&payload_seg_0_3[i * 2], segment_temp_decideg[i]);
    }
    for (int i = 0; i < 2; i++) {
        pack_le_int16(&payload_seg_4_5[i * 2], segment_temp_decideg[4 + i]);
    }

    if (HAL_CAN_AddTxMessage(&hcan2, &tx_header_seg_0_3, payload_seg_0_3, &mailbox) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_CAN_AddTxMessage(&hcan2, &tx_header_seg_4_5, payload_seg_4_5, &mailbox) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}
