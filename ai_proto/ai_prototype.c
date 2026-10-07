```c
#include "main.h"

volatile int32_t encoder_position = 0;

/*
 * Previous 2-bit encoder state:
 *
 *   bit 1 = Encoder B
 *   bit 0 = Encoder A
 */
static uint8_t encoder_previous_state = 0;


/*
 * Quadrature transition table.
 *
 * Index = (previous_state << 2) | current_state
 *
 * Valid transitions:
 *
 *   00 -> 01 -> 11 -> 10 -> 00  = clockwise
 *   00 -> 10 -> 11 -> 01 -> 00  = counter-clockwise
 *
 * Invalid transitions are ignored.
 */
static const int8_t quadrature_table[16] =
{
     0,  // 0000: 00 -> 00
    -1,  // 0001: 00 -> 01
     1,  // 0010: 00 -> 10
     0,  // 0011: 00 -> 11 (invalid)

     1,  // 0100: 01 -> 00
     0,  // 0101: 01 -> 01
     0,  // 0110: 01 -> 10 (invalid)
    -1,  // 0111: 01 -> 11

    -1,  // 1000: 11 -> 00 (invalid)
     0,  // 1001: 11 -> 01
     0,  // 1010: 11 -> 10
     1,  // 1011: 11 -> 11

     0,  // 1100: 10 -> 00
     1,  // 1101: 10 -> 01 (invalid)
    -1,  // 1110: 10 -> 10
     0   // 1111: 10 -> 11
};


/*
 * Call this whenever either encoder input changes.
 */
static void Encoder_Update(void)
{
    uint8_t A = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    uint8_t B = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);

    uint8_t current_state = (B << 1) | A;

    uint8_t index =
        (encoder_previous_state << 2) | current_state;

    encoder_position += quadrature_table[index];

    encoder_previous_state = current_state;
}


/*
 * STM32 HAL EXTI callback.
 *
 * This gets called whenever PA0 or PA1 changes state.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0 ||
        GPIO_Pin == GPIO_PIN_1)
    {
        Encoder_Update();
    }
}


/*
 * EXTI0 interrupt handler for PA0.
 */
void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}


/*
 * EXTI1 interrupt handler for PA1.
 */
void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}
```

### Reading the position

You can then simply use:

```c
int32_t position = encoder_position;
```

For example:

```c
while (1)
{
    printf("Encoder: %ld\r\n", encoder_position);

    HAL_Delay(100);
}
```

The important part is that **you don't determine direction based solely on which interrupt fired**. Instead, you look at the previous A/B state and the new A/B state. That's considerably more robust.

### One important correction

There is an even better solution on the **STM32L432KC**: use **TIM2 encoder mode** on the appropriate alternate-function pins. The timer can decode A/B in hardware, increment/decrement its counter, and eliminate essentially all of the CPU interrupt overhead.

For a rotary encoder that you're turning frequently, I'd strongly recommend:

**Encoder A → TIM2_CH1**
**Encoder B → TIM2_CH2**

and then read:

```c
int32_t position = (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
```

You don't actually need an interrupt for every encoder transition.

If you're using **CubeIDE/CubeMX**, I can also give you the exact **STM32L432KC pin/alternate-function configuration and complete `MX_GPIO_Init()` / `MX_TIM2_Init()` code** for the hardware-decoded approach.
