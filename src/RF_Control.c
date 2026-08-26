#include "RF_Control.h"
#include "stm32c0xx_hal.h"

#define TIMEOUT_RESET_MS 5000
#define CLICK_WINDOW_MS  300

// forward declaration
extern void Display_Update(tm1637_t *handle, uint8_t val, uint8_t show_dp);

// prescaler state
static uint8_t active_div = 0;
static uint8_t draft_div  = 0;

// display cache
static uint8_t prev_disp_val = 0xFF;
static uint8_t prev_disp_dp  = 0xFF;

// ui state tracking
static uint8_t is_editing         = 0;
static uint32_t last_edit_time    = 0;
static uint32_t last_flicker_time = 0;
static uint8_t flicker_state      = 0;

// encoder tracking
static uint8_t last_phase_a = 1;

// button tracking
static uint32_t btn_press_start = 0;
static uint8_t btn_last_state   = 1;
static uint8_t click_count      = 0;
static uint32_t last_click_time = 0;

// write 7-bit divider word to parallel gpio bus (pa0-pa6)
void RF_Set_Divider(uint8_t div_val) {
    active_div = div_val & 0x7F;
    GPIOA->ODR = (GPIOA->ODR & ~0x7F) | active_div;
}

void RF_Control_Init(void) {
    RF_Set_Divider(active_div);
}

void RF_Control_Process(tm1637_t *display_handle) {
    uint32_t now = HAL_GetTick();

    // 1. quadrature encoder polling
    uint8_t curr_phase_a = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6);
    uint8_t curr_phase_b = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7);

    if (last_phase_a == 1 && curr_phase_a == 0) {
        if (!is_editing) {
            is_editing = 1;
            draft_div = active_div;
        }

        if (curr_phase_b == 1) {
            if (draft_div < 127) draft_div++;
        } else {
            if (draft_div > 0) draft_div--;
        }
        last_edit_time = now;
    }
    last_phase_a = curr_phase_a;

    // 2. button gestures
    uint8_t btn_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7);

    if (btn_last_state == 1 && btn_state == 0) {
        btn_press_start = now;
    }

    if (btn_state == 0 && (now - btn_press_start >= TIMEOUT_RESET_MS)) {
        NVIC_SystemReset();
    }

    if (btn_last_state == 0 && btn_state == 1) {
        if ((now - btn_press_start) < 1000) {
            click_count++;
            last_click_time = now;
        }
    }
    btn_last_state = btn_state;

    if (click_count > 0 && (now - last_click_time > CLICK_WINDOW_MS)) {
        if (click_count >= 3) {
            draft_div = 0;
            RF_Set_Divider(0);
            is_editing = 0;
        } else if (click_count == 1) {
            if (is_editing) {
                RF_Set_Divider(draft_div);
                is_editing = 0;
            }
        }
        click_count = 0;
    }

    // 3. timeout & visual feedback
    if (is_editing && (now - last_edit_time >= TIMEOUT_RESET_MS)) {
        is_editing = 0;
        draft_div = active_div;
    }

    if (is_editing && (now - last_flicker_time >= CLICK_WINDOW_MS / 2)) {
        flicker_state = !flicker_state;
        last_flicker_time = now;
    }

    uint8_t target_val = is_editing ? draft_div : active_div;
    uint8_t target_dp  = is_editing ? flicker_state : 0;

    if (target_val != prev_disp_val || target_dp != prev_disp_dp) {
        Display_Update(display_handle, target_val, target_dp);
        prev_disp_val = target_val;
        prev_disp_dp  = target_dp;
    }
}
