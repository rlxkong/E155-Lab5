// lab5_main.c
// Rebecca Kong
// rkong@hmc.edu
// 10/6/2026

#include "main.h"

int main(void) {
    // Enable encoders as inputs
    gpioEnable(GPIO_PORT_A);
    pinMode(ENCODE_A_PIN , GPIO_INPUT);
    pinMode(ENCODE_B_PIN, GPIO_INPUT);

    GPIOA->PUPDR |= (0b01 << 2*gpioPinOffset(ENCODE_A_PIN)); // Set PA8 as pull-up (PUPD8 = 01)
    GPIOA->PUPDR |= (0b01 << 2*gpioPinOffset(ENCODE_B_PIN)); // Set PA9 as pull-up (PUPD9 = 01)

    // Initialize timer
    RCC->APB1ENR1 |= (1 << 0); // TIM2EN
    initTIM(TIMER, 1e4);       // 80MHz/8kHz = 10kHz

    // 1. Enable SYSCFG clock domain in RCC
    RCC->APB2ENR |= (1 << 0); // SYSCFGEN
    // 2. Configure EXTICR for the input button interrupt
    // EXTI8 is bits 2:0 of EXTICR3 (EXTICR[2] in C). Port A is 0b000, so clearing the field selects PA8.
    SYSCFG->EXTICR[2] &= ~(0b111 << 0);
    // EXTI8 is bits 6:4 of EXTICR3 (EXTICR[2] in C). Port A is 0b000, so clearing the field selects PA9.
    SYSCFG->EXTICR[2] &= ~(0b111 << 4);

    // Enable interrupts globally
    __enable_irq();

    // Configure interrupt for falling edge of GPIO pin for button
    EXTI->IMR1 |= (1 << gpioPinOffset(BUTTON_PIN));   // 1. Configure mask bit
    EXTI->RTSR1 &= ~(1 << gpioPinOffset(BUTTON_PIN)); // 2. Disable rising edge trigger
    EXTI->FTSR1 |= (1 << gpioPinOffset(BUTTON_PIN));  // 3. Enable falling edge trigger
    NVIC->ISER[0] |= (1 << 23);                       // 4. Turn on EXTI interrupt in NVIC_ISER (EXTI9_5 is IRQ 23)

    while(1){
        delay_millis(DELAY_TIM, 200);
    }

}

// EXTI lines 5-9 share this handler
void EXTI9_5_IRQHandler(void){
    // Check that the button was what triggered our interrupt
    if (EXTI->PR1 & (1 << gpioPinOffset(BUTTON_PIN))){
        // If so, clear the interrupt (NB: Write 1 to reset.)
        EXTI->PR1 = (1 << gpioPinOffset(BUTTON_PIN));

        // Then toggle the LED
        togglePin(LED_PIN);

    }
}
