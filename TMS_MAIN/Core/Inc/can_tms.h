/*
 * can_tms.h
 *
 * TMS CAN tx: publishes the hottest reading from each of the 6 accumulator
 * segments on CAN2 (500 kbps) at 4 Hz.
 *
 *   Frame 0x30 (DLC=8): segments 0..3, 2 bytes each, little-endian
 *   Frame 0x31 (DLC=4): segments 4..5, 2 bytes each, little-endian
 *
 * Wire format is int16_t decidegrees Celsius (0.1 C / LSB, signed).
 * Layout matches the legacy EV26-TMS frame format.
 */

#ifndef INC_CAN_TMS_H_
#define INC_CAN_TMS_H_

#include "main.h"
#include <stdint.h>

#define CAN_TMS_SEG_COUNT            6
#define CAN_TMS_FRAME_SEG_0_3_ID     0x30u
#define CAN_TMS_FRAME_SEG_4_5_ID     0x31u

extern CAN_HandleTypeDef hcan2;

void can_tms_init(void);
HAL_StatusTypeDef can_tms_send_segment_temps(const int16_t segment_temp_decideg[CAN_TMS_SEG_COUNT]);

#endif /* INC_CAN_TMS_H_ */
