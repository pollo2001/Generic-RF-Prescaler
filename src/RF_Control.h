#ifndef RF_CONTROL_H_
#define RF_CONTROL_H_

#include <stdint.h>
#include "tm1637.h"

// generic board abstraction mapping
typedef struct {
    uint8_t addr_disp1;     // digit 1 address (hundreds)
    uint8_t addr_disp2;     // digit 2 address (tens)
    uint8_t addr_disp3;     // digit 3 address (ones)
    const uint8_t *seg_map; // pointer to segment remapping table
} BoardProfile_t;

void RF_Control_Init(void);
void RF_Apply_Hardware_Divider(uint8_t div_val);
void RF_Control_Process(tm1637_t *display_handle, const BoardProfile_t *board_config);

#endif /* RF_CONTROL_H_ */
