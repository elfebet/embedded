#include "dwt_timer.h"
#include "stm32f4xx.h"

void dwt_reset_timer() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // Enable DWT
    DWT->CYCCNT = 0;                                // Clear counter
    DWT->CTRL = DWT_CTRL_CYCCNTENA_Msk;             // Enable counter
}

uint32_t dwt_get_time() {
    return DWT->CYCCNT;
}

uint32_t dwt_elapsed_us(uint32_t start_time) {
    return (uint32_t)(((uint64_t)(DWT->CYCCNT - start_time) * 1000000U) / SystemCoreClock);
}

uint32_t dwt_elapsed_ms(uint32_t start_time) {
    return (uint32_t)(((uint64_t)(DWT->CYCCNT - start_time) * 1000U) / SystemCoreClock);
}

uint32_t dwt_cycles_to_us(uint32_t cycles) {
    return (uint32_t)((uint64_t)cycles * 1000000U / SystemCoreClock);
}

uint32_t dwt_cycles_to_ms(uint32_t cycles) {
    return (uint32_t)((uint64_t)cycles * 1000U / SystemCoreClock);
}

uint32_t dwt_ms_to_cycles(uint32_t ms) {
    return (SystemCoreClock / 1000) * ms;
}
