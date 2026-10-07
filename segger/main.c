// lab5_main.c
// Rebecca Kong
// rkong@hmc.edu
// 10/6/2026

#include "main.h"
#include "STM32L432KC.h"
#include <stdio.h>

// global variables
volatile int direction;
volatile int pulse;
volatile float velocity;

// motor digital sensor status for direction sensing
int pulse_a;
int pulse_b;

// Function used by printf to send characters to the laptop
int _write(int file, char *ptr, int len) {
  int i = 0;
  for (i = 0; i < len; i++) {
    ITM_SendChar((*ptr++));
  }
  return len;
}

int main(void) {
    // Enable encoders as inputs
    gpioEnable(GPIO_PORT_A);
    pinMode(ENCODE_A_PIN, GPIO_INPUT);
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

    // Set interrupt priority 
    // NVIC priority interrupts so that clocks are synced & ensures NVIC detects interrupt
    __NVIC_SetPriority(TIM2_IRQn, 1);     // Clock top priority
    __NVIC_SetPriority(EXTI9_5_IRQn, 2);

    // We do on all edges for more accurate timing 
    // Configure interrupt for falling edge of GPIO pin for PA8
    EXTI->IMR1 |= (1 << gpioPinOffset(ENCODE_A_PIN));  // 1. Configure mask bit
    EXTI->RTSR1 |= (1 << gpioPinOffset(ENCODE_A_PIN)); // 2. Enable rising edge trigger
    EXTI->FTSR1 |= (1 << gpioPinOffset(ENCODE_A_PIN)); // 3. Enable falling edge trigger

    // Configure interrupt for falling edge of GPIO pin for PA9
    EXTI->IMR1 |= (1 << gpioPinOffset(ENCODE_B_PIN));  // 1. Configure mask bit
    EXTI->RTSR1 |= (1 << gpioPinOffset(ENCODE_B_PIN)); // 2. Enable rising edge trigger
    EXTI->FTSR1 |= (1 << gpioPinOffset(ENCODE_B_PIN)); // 3. Enable falling edge trigger

    NVIC->ISER[0] |= (1 << 23);                        // 4. Turn on EXTI interrupt in NVIC_ISER (EXTI9_5 is IRQ 23)

    while(1){
        // Find speed in rev/s
        // 4 edges per period so divide by 4
        velocity = ((float)pulse) / (4.0f*408.0f);

        /*while(1) {
         printf("A=%d B=%d\n",
         digitalRead(ENCODE_A_PIN),
         digitalRead(ENCODE_B_PIN));
        }*/

        // check the velocity and direction each second
        if(TIMER->CNT == 10000) {
          printf("Speed (rev/s): %f\n ", velocity);
          printf("Direction: %d\n ", direction);
          //printf("pulse: %d\n ", pulse);

          // Reset count and timer and pulse
          pulse = 0;
          TIMER->SR &= ~(1<<0);
          TIMER->CNT = 0;
        }
    }

}

// EXTI lines 5-9 share this handler
void EXTI9_5_IRQHandler(void){

    // Read the values of encoder signals to find direction
    pulse_a = digitalRead(ENCODE_A_PIN);
    pulse_b = digitalRead(ENCODE_B_PIN);

    // Check that the A_sesnor was what triggered our interrupt
    if (EXTI->PR1 & (1 << gpioPinOffset(ENCODE_A_PIN))){
        // If so, clear the interrupt (NB: Write 1 to reset.)
        EXTI->PR1 = (1 << gpioPinOffset(ENCODE_A_PIN));

        // Check direction of the motor - if it is CW A rises first and vice versa
        // if on the rising edge of A
        if (pulse_a == 1) {
          // b following
          if (pulse_b == 0) {
            direction = CW;
          }
          // b leads
          else {
            direction = CCW;
          }
        }
        else{
          // b leads
          if (pulse_b == 0) {
            direction = CCW;
          }
          // b following
          else {
            direction = CW;
          }
        }

        // Increment count of rising edge
        pulse++;

    }

    // Check that the B_sensor was what triggered our interrupt
    if (EXTI->PR1 & (1 << gpioPinOffset(ENCODE_B_PIN))){
        // If so, clear the interrupt (NB: Write 1 to reset.)
        EXTI->PR1 = (1 << gpioPinOffset(ENCODE_B_PIN));

        // Check direction of the motor - if it is CW A rises first and vice versa
        // if on the rising edge of B
        if (pulse_b == 1) {
          // a follows
          if (pulse_a == 0) {
            direction = CCW;
          }
          // a leading
          else {
            direction = CW;
          }
        }
        else{
          // a following
          if (pulse_a == 0) {
            direction = CW;
          }
          // a leading
          else {
            direction = CCW;
          }
        }
        // Increment count of rising edge
        pulse++;
        
    }
}
