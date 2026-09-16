#ifndef REGISTER_MAPS_H
#define REGISTER_MAPS_H

#include <stdint.h>

#define IMXRT_GPIO1_BASE          0x401B8000
#define IMXRT_GPIO2_BASE          0x401BC000
#define IMXRT_FLEXPWM1_BASE       0x403DC000
#define IMXRT_ADC1_BASE           0x403B0000
#define IMXRT_ADC2_BASE           0x403B4000
#define IMXRT_RTWDOG_BASE         0x400BC000

#define GPIO_DR(base)             ((volatile uint32_t*)((base) + 0x00))
#define GPIO_GDIR(base)           ((volatile uint32_t*)((base) + 0x04))
#define GPIO_PSR(base)            ((volatile uint32_t*)((base) + 0x08))
#define GPIO_ICR1(base)           ((volatile uint32_t*)((base) + 0x0C))
#define GPIO_ICR2(base)           ((volatile uint32_t*)((base) + 0x10))
#define GPIO_IMR(base)            ((volatile uint32_t*)((base) + 0x14))
#define GPIO_ISR(base)            ((volatile uint32_t*)((base) + 0x18))
#define GPIO_EDGE_SEL(base)       ((volatile uint32_t*)((base) + 0x1C))

#define PWM_SM0CTRL(base)         ((volatile uint16_t*)((base) + 0x00))
#define PWM_SM0CTRL2(base)        ((volatile uint16_t*)((base) + 0x02))
#define PWM_SM0INIT(base)         ((volatile uint16_t*)((base) + 0x04))
#define PWM_SM0CTRL1(base)        ((volatile uint16_t*)((base) + 0x06))
#define PWM_SM0VAL0(base)         ((volatile uint16_t*)((base) + 0x08))
#define PWM_SM0VAL1(base)         ((volatile uint16_t*)((base) + 0x0A))
#define PWM_SM0VAL2(base)         ((volatile uint16_t*)((base) + 0x0C))
#define PWM_SM0VAL3(base)         ((volatile uint16_t*)((base) + 0x0E))
#define PWM_SM0VAL4(base)         ((volatile uint16_t*)((base) + 0x10))
#define PWM_SM0VAL5(base)         ((volatile uint16_t*)((base) + 0x12))
#define PWM_SM0OCTRL(base)        ((volatile uint16_t*)((base) + 0x14))
#define PWM_SM0STS(base)          ((volatile uint16_t*)((base) + 0x16))
#define PWM_SM0INTEN(base)        ((volatile uint16_t*)((base) + 0x18))
#define PWM_OUTEN(base)           ((volatile uint16_t*)((base) + 0x180))
#define PWM_MASK(base)            ((volatile uint16_t*)((base) + 0x182))
#define PWM_SWCOUT(base)          ((volatile uint16_t*)((base) + 0x184))
#define PWM_MCTRL(base)           ((volatile uint16_t*)((base) + 0x186))

#define ADC_HC0(base)             ((volatile uint32_t*)((base) + 0x00))
#define ADC_HS(base)              ((volatile uint32_t*)((base) + 0x08))
#define ADC_R0(base)              ((volatile uint32_t*)((base) + 0x0C))
#define ADC_CFG(base)             ((volatile uint32_t*)((base) + 0x14))
#define ADC_GC(base)              ((volatile uint32_t*)((base) + 0x18))
#define ADC_GS(base)              ((volatile uint32_t*)((base) + 0x1C))
#define ADC_CV(base)              ((volatile uint32_t*)((base) + 0x20))
#define ADC_OFS(base)             ((volatile uint32_t*)((base) + 0x24))

#define WDOG_CS(base)             ((volatile uint32_t*)((base) + 0x00))
#define WDOG_CNT(base)            ((volatile uint32_t*)((base) + 0x04))
#define WDOG_TOVAL(base)          ((volatile uint32_t*)((base) + 0x08))
#define WDOG_WIN(base)            ((volatile uint32_t*)((base) + 0x0C))
#define WDOG_CNT_REFRESH          ((volatile uint16_t*)((IMXRT_RTWDOG_BASE) + 0x12))

#define HARDWARE_DATA_FENCE()         __asm__ __volatile__ ("dsb" ::: "memory")
#define HARDWARE_INSTRUCTION_FENCE()  __asm__ __volatile__ ("isb" ::: "memory")

static inline void init_hardware_registers(void) {
    HARDWARE_DATA_FENCE();
    *GPIO_GDIR(IMXRT_GPIO1_BASE) &= ~(1U << 16);
    *GPIO_GDIR(IMXRT_GPIO1_BASE) &= ~(1U << 17);
    *GPIO_GDIR(IMXRT_GPIO2_BASE) |= (1U << 4);
    *PWM_MCTRL(IMXRT_FLEXPWM1_BASE) = 0x0000;
    *PWM_SM0CTRL(IMXRT_FLEXPWM1_BASE) = 0x0400;
    *PWM_SM0CTRL2(IMXRT_FLEXPWM1_BASE) = 0x2000;
    *PWM_SM0INIT(IMXRT_FLEXPWM1_BASE) = 0x0000;
    *PWM_SM0VAL1(IMXRT_FLEXPWM1_BASE) = 6000;
    *PWM_SM0VAL2(IMXRT_FLEXPWM1_BASE) = 0;
    *PWM_SM0VAL3(IMXRT_FLEXPWM1_BASE) = 3000;
    *PWM_OUTEN(IMXRT_FLEXPWM1_BASE) |= 0x0100;
    *ADC_CFG(IMXRT_ADC1_BASE) = 0x00000098;
    *ADC_CFG(IMXRT_ADC2_BASE) = 0x00000098;
    *ADC_HC0(IMXRT_ADC1_BASE) = 0x00000010;
    *ADC_HC0(IMXRT_ADC2_BASE) = 0x00000011;
    *WDOG_TOVAL(IMXRT_RTWDOG_BASE) = 60000;
    *WDOG_CS(IMXRT_RTWDOG_BASE) = 0x00002521;
    HARDWARE_INSTRUCTION_FENCE();
}

static inline void refresh_hardware_watchdog(void) {
    HARDWARE_DATA_FENCE();
    *WDOG_CNT_REFRESH = 0xB480;
    *WDOG_CNT_REFRESH = 0xA815;
    HARDWARE_INSTRUCTION_FENCE();
}

static inline uint32_t read_adc_channel_one(void) {
    if (!(*ADC_HS(IMXRT_ADC1_BASE) & 0x01)) {
        return *ADC_R0(IMXRT_ADC1_BASE) & 0xFFF;
    }
    *ADC_HC0(IMXRT_ADC1_BASE) = 0x00000010;
    return *ADC_R0(IMXRT_ADC1_BASE) & 0xFFF;
}

static inline uint32_t read_adc_channel_two(void) {
    if (!(*ADC_HS(IMXRT_ADC2_BASE) & 0x01)) {
        return *ADC_R0(IMXRT_ADC2_BASE) & 0xFFF;
    }
    *ADC_HC0(IMXRT_ADC2_BASE) = 0x00000011;
    return *ADC_R0(IMXRT_ADC2_BASE) & 0xFFF;
}

static inline void set_plasma_actuator_duty(uint16_t duty_counts) {
    HARDWARE_DATA_FENCE();
    if (duty_counts > 6000) {
        duty_counts = 6000;
    }
    *PWM_SM0VAL3(IMXRT_FLEXPWM1_BASE) = duty_counts;
    *PWM_MCTRL(IMXRT_FLEXPWM1_BASE) |= 0x0001;
    HARDWARE_INSTRUCTION_FENCE();
}

static inline void emergency_shutoff_actuators(void) {
    HARDWARE_DATA_FENCE();
    *PWM_MCTRL(IMXRT_FLEXPWM1_BASE) &= ~0x0001;
    *PWM_SWCOUT(IMXRT_FLEXPWM1_BASE) = 0x0000;
    *GPIO_DR(IMXRT_GPIO2_BASE) &= ~(1U << 4);
    HARDWARE_INSTRUCTION_FENCE();
}

static inline uint32_t check_hardware_faults(void) {
    uint32_t fault_vector = 0;
    if (*PWM_SM0STS(IMXRT_FLEXPWM1_BASE) & 0x0002) {
        fault_vector |= 0x01;
    }
    if (*ADC_GS(IMXRT_ADC1_BASE) & 0x02) {
        fault_vector |= 0x02;
    }
    if (*ADC_GS(IMXRT_ADC2_BASE) & 0x02) {
        fault_vector |= 0x04;
    }
    return fault_vector;
}

#endif
