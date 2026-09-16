#ifndef REGISTER_MAPS_H
#define REGISTER_MAPS_H

#include <stdint.h>

// Direct Memory-Mapped I/O (MMIO) Registers for Cortex-M7 Peripherals
#define IMXRT_GPIO_PAD_STATUS   ((volatile uint32_t*)0x401B8000) // High-speed analog input pad
#define IMXRT_PWM_GATE_DRIVER   ((volatile uint32_t*)0x400F4004) // FlexPWM drive registers
#define IMXRT_WATCHDOG_REFRESH  ((volatile uint32_t*)0x400B8008) // RTWDOG refresh register

// Assembly-level compiler fences to prevent instruction reordering or cache lag
#define HARDWARE_DATA_FENCE()         __asm__ __volatile__ ("dsb" ::: "memory")
#define HARDWARE_INSTRUCTION_FENCE()  __asm__ __volatile__ ("isb" ::: "memory")

/**
 * Initializes direct bare-metal peripheral configurations
 */
static inline void init_hardware_registers(void) {
    HARDWARE_DATA_FENCE();
    *IMXRT_PWM_GATE_DRIVER = 0x00000000; // Reset gate array into safe, non-conducting isolation mode
    *IMXRT_WATCHDOG_REFRESH = 0x0000A815; // Initial hardware watchdog sequence key
    HARDWARE_INSTRUCTION_FENCE();
}

/**
 * Ticks the internal 100ms safety watchdog to prevent systemic trip
 */
static inline void refresh_hardware_watchdog(void) {
    // Standard ARM hardware watchdog sequence to prove execution health
    *IMXRT_WATCHDOG_REFRESH = 0xB480;
    *IMXRT_WATCHDOG_REFRESH = 0xA815;
}

#endif // REGISTER_MAPS_H
