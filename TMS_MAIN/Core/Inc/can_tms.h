/*
 * can_tms.h
 *
 * TMS -> Orion BMS + VCU over CAN1 AND CAN2 (500 kbps, extended IDs). Same
 * message is published on both buses while the local-bus → vehicle-bus mapping
 * is still being pinned down (vehicle CAN3 reaches the BMS; locally that is
 * believed to be CAN1, but we publish on both to be safe).
 *
 * The TMS impersonates 6 Orion external thermistor expansion modules, one per
 * satellite board. All 6 modules share J1939 source address 0x80; the module
 * is differentiated inside the payload (byte 5 of address-claim, byte 1 of
 * data).
 *
 *   0x18EEFF80  J1939 address claim       @ 200 ms per module  (DLC=8)
 *   0x1839F380  Thermistor module -> BMS  @ 100 ms per module  (DLC=8)
 *
 * Temperatures are int8 °C (1 °C / LSB, signed). Per-module thermistor IDs are
 * zero-based channel indices on the module (0..5).
 */

#ifndef INC_CAN_TMS_H_
#define INC_CAN_TMS_H_

#include "main.h"
#include <stdint.h>

#define CAN_TMS_MODULE_COUNT          6u
#define CAN_TMS_THERMS_PER_MODULE     6u
#define CAN_TMS_ADDR_CLAIM_ID         0x18EEFF80u
#define CAN_TMS_THERM_DATA_ID         0x1839F380u

typedef struct {
    int8_t  low_c;        /* lowest thermistor reading on the module, signed °C */
    int8_t  high_c;       /* highest thermistor reading on the module, signed °C */
    int8_t  avg_c;        /* mean thermistor reading on the module, signed °C */
    uint8_t therm_count;  /* number of thermistors enabled on the module */
    uint8_t high_id;      /* zero-based channel index reporting high_c */
    uint8_t low_id;       /* zero-based channel index reporting low_c */
} can_tms_module_data_t;

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

void              can_tms_init(void);
HAL_StatusTypeDef can_tms_send_address_claim(uint8_t module_index);
HAL_StatusTypeDef can_tms_send_module_data(uint8_t module_index,
                                           const can_tms_module_data_t *data);

#endif /* INC_CAN_TMS_H_ */
