#ifndef REGISTER_MAPS_H
#define REGISTER_MAPS_H

#include <stdint.h>

#define IMXRT_GPIO_PAD_STATUS   ((volatile uint32_t*)0x401B8000)
#define IMXRT_PWM_GATE_DRIVER   ((volatile uint32_t*)0x400F4004)
#define IMXRT_WATCHDOG_REFRESH  ((volatile uint32_t*)0x400B8008)

#define HARDWARE_DATA_FENCE()         __asm__ __volatile__ ("dsb" ::: "memory")
#define HARDWARE_INSTRUCTION_FENCE()  __asm__ __volatile__ ("isb" ::: "memory")

static inline void init_hardware_registers(void) {
    HARDWARE_DATA_FENCE();
    *IMXRT_PWM_GATE_DRIVER = 0x00000000;
    *IMXRT_WATCHDOG_REFRESH = 0x0000A815;
    HARDWARE_INSTRUCTION_FENCE();
}

static inline void refresh_hardware_watchdog(void) {
    *IMXRT_WATCHDOG_REFRESH = 0xB480;
    *IMXRT_WATCHDOG_REFRESH = 0xA815;
}

#endif
