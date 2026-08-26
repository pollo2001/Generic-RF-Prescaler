#include <stdint.h>
#include "main.h"
#include "RF_Control.h"
#include "tm1637.h"

// standard 7-segment digit lookup table
static const uint8_t DIGIT_MAP[10] = {
    0x3F, // '0'
    0x06, // '1'
    0x5B, // '2'
    0x4F, // '3'
    0x66, // '4'
    0x6D, // '5'
    0x7D, // '6'
    0x07, // '7'
    0x7F, // '8'
    0x6F  // '9'
};

// display driver instance
tm1637_t display = {
    .gpio_clk = GPIOA,
    .gpio_dat = GPIOA,
    .pin_clk  = GPIO_PIN_11,
    .pin_dat  = GPIO_PIN_12,
    .seg_cnt  = 3
};

// splits integer to 3 digits and pushes to display
void Display_Update(tm1637_t *handle, uint8_t val, uint8_t show_dp) {
    uint8_t d_100 = (val / 100) % 10;
    uint8_t d_10  = (val / 10)  % 10;
    uint8_t d_1   = val % 10;

    uint8_t seg_100 = DIGIT_MAP[d_100];
    uint8_t seg_10  = DIGIT_MAP[d_10];
    uint8_t seg_1   = DIGIT_MAP[d_1];

    if (show_dp) {
        seg_1 |= 0x80;
    }

    tm1637_start(handle);
    tm1637_write(handle, 0xC0); // standard grid address
    tm1637_write(handle, seg_100);
    tm1637_stop(handle);

    tm1637_start(handle);
    tm1637_write(handle, 0xC1);
    tm1637_write(handle, seg_10);
    tm1637_stop(handle);

    tm1637_start(handle);
    tm1637_write(handle, 0xC2);
    tm1637_write(handle, seg_1);
    tm1637_stop(handle);
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_0);
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv              = RCC_HSI_DIV4;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.SYSCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) { Error_Handler(); }
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // inputs: button (pa7), encoder phase a (pb6), encoder phase b (pb7)
    GPIO_InitStruct.Pin  = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // display interface: clk (pa11), dio (pa12)
    GPIO_InitStruct.Pin   = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 7-bit parallel output bus: pa0 - pa6
    GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                            GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    tm1637_init(&display);
    tm1637_brightness(&display, 3);
    RF_Control_Init();

    while (1) {
        RF_Control_Process(&display);
    }
}
