/*
 * can_tms.h
 *
 * TMS -> Orion BMS + VCU over CAN1 AND CAN2 (500 kbps, extended IDs). Same
 * message is published on both buses while the local-bus → vehicle-bus mapping
 * is still being pinned down (vehicle CAN3 reaches the BMS; locally that is
 * believed to be CAN1, but we publish on both to be safe).
 *
 * The TMS impersonates ONE Orion external thermistor expansion module whose 6
 * "thermistors" are the 6 battery packs (one hottest-cell reading per pack,
 * collected over the shared SPI bus). J1939 source address 0x80, module# 0.
 *
 *   0x18EEFF80  J1939 address claim       @ 200 ms  (DLC=8)
 *   0x1839F380  Thermistor module -> BMS  @ 100 ms  (DLC=8)
 *
 * Temperatures are int8 °C (1 °C / LSB, signed). high_id/low_id are the pack
 * (segment) indices 0..5 reporting the high/low. low_c and avg_c run warm
 * because each pack value is itself a hottest-cell reading — an accepted,
 * hardware-constrained coarsening (see thermistor.c).
 */

#ifndef INC_CAN_TMS_H_
#define INC_CAN_TMS_H_

#include "main.h"
#include <stdint.h>

#define CAN_TMS_MODULE_INDEX          0u   /* single module, module# 0 */
#define CAN_TMS_THERMS_PER_MODULE     5u   /* = the 5 battery packs */
#define CAN_TMS_ADDR_CLAIM_ID         0x18EEFF80u
#define CAN_TMS_THERM_DATA_ID         0x1839F380u

typedef struct {
    int8_t  low_c;        /* lowest pack reading, signed °C */
    int8_t  high_c;       /* highest pack reading, signed °C */
    int8_t  avg_c;        /* mean of the pack readings, signed °C */
    uint8_t therm_count;  /* number of packs reported (always 6) */
    uint8_t high_id;      /* pack index (0..5) reporting high_c */
    uint8_t low_id;       /* pack index (0..5) reporting low_c */
} can_tms_module_data_t;

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

void              can_tms_init(void);
HAL_StatusTypeDef can_tms_send_address_claim(uint8_t module_index);
HAL_StatusTypeDef can_tms_send_module_data(uint8_t module_index,
                                           const can_tms_module_data_t *data);

#endif /* INC_CAN_TMS_H_ */
