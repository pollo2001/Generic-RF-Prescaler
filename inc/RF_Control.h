#ifndef RF_CONTROL_H_
#define RF_CONTROL_H_

#include <stdint.h>
#include "tm1637.h"

void RF_Control_Init(void);
void RF_Set_Divider(uint8_t div_val);
void RF_Control_Process(tm1637_t *display_handle);

#endif /* RF_CONTROL_H_ */
