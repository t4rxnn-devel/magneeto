#include <stdint.h>

#define IMXRT_LPSPI1_PARAM       ((volatile uint32_t*)0x40394004)
#define IMXRT_LPSPI1_CR          ((volatile uint32_t*)0x40394010)
#define IMXRT_LPSPI1_SR          ((volatile uint32_t*)0x40394014)
#define IMXRT_LPSPI1_IER         ((volatile uint32_t*)0x40394018)
#define IMXRT_LPSPI1_DER         ((volatile uint32_t*)0x4039401C)
#define IMXRT_LPSPI1_CFGR0       ((volatile uint32_t*)0x40394020)
#define IMXRT_LPSPI1_CFGR1       ((volatile uint32_t*)0x40394024)
#define IMXRT_LPSPI1_TCR         ((volatile uint32_t*)0x40394030)
#define IMXRT_LPSPI1_TDR         ((volatile uint32_t*)0x40394034)
#define IMXRT_LPSPI1_RSR         ((volatile uint32_t*)0x40394040)
#define IMXRT_LPSPI1_RDR         ((volatile uint32_t*)0x40394044)

#define AD7606_CONVST_A_PIN      (1U << 12)
#define AD7606_CONVST_B_PIN      (1U << 13)
#define AD7606_RESET_PIN         (1U << 14)
#define AD7606_BUSY_PIN          (1U << 15)

#define IMXRT_GPIO3_DR           ((volatile uint32_t*)0x401C0000)
#define IMXRT_GPIO3_GDIR         ((volatile uint32_t*)0x401C0004)
#define IMXRT_GPIO3_PSR          ((volatile uint32_t*)0x401C0008)

static inline void microsecond_hardware_delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count * 120; i++) {
        __asm__ __volatile__ ("nop");
    }
}

void init_ad7606_hardware_spi(void) {
    __asm__ __volatile__ ("dsb" ::: "memory");
    *IMXRT_GPIO3_GDIR |= (AD7606_CONVST_A_PIN | AD7606_CONVST_B_PIN | AD7606_RESET_PIN);
    *IMXRT_GPIO3_GDIR &= ~AD7606_BUSY_PIN;
    *IMXRT_GPIO3_DR |= AD7606_RESET_PIN;
    microsecond_hardware_delay(10);
    *IMXRT_GPIO3_DR &= ~AD7606_RESET_PIN;
    microsecond_hardware_delay(100);
    *IMXRT_LPSPI1_CR = 0x00000000;
    *IMXRT_LPSPI1_CFGR1 = 0x00000001;
    *IMXRT_LPSPI1_TCR = (15U << 0) | (0U << 24) | (1U << 27);
    *IMXRT_LPSPI1_CR = 0x00000001;
    __asm__ __volatile__ ("isb" ::: "memory");
}

void trigger_ad7606_conversion_pulse(void) {
    __asm__ __volatile__ ("dsb" ::: "memory");
    *IMXRT_GPIO3_DR &= ~(AD7606_CONVST_A_PIN | AD7606_CONVST_B_PIN);
    __asm__ __volatile__ ("nop");
    __asm__ __volatile__ ("nop");
    *IMXRT_GPIO3_DR |= (AD7606_CONVST_A_PIN | AD7606_CONVST_B_PIN);
    __asm__ __volatile__ ("isb" ::: "memory");
}

uint32_t await_ad7606_conversion_ready(void) {
    uint32_t watch_cycles = 0;
    while ((*IMXRT_GPIO3_PSR & AD7606_BUSY_PIN)) {
        watch_cycles++;
        if (watch_cycles > 5000) {
            return 0;
        }
    }
    return 1;
}

int32_t read_spi_channel_raw_sample(uint32_t channel_index) {
    if (channel_index >= 8) {
        return 0;
    }
    while (!(*IMXRT_LPSPI1_SR & (1U << 0)));
    *IMXRT_LPSPI1_TDR = 0x00000000;
    while (!(*IMXRT_LPSPI1_SR & (1U << 1)));
    uint32_t spi_raw_frame = *IMXRT_LPSPI1_RDR;
    int16_t sign_extended_data = (int16_t)(spi_raw_frame & 0xFFFF);
    return (int32_t)sign_extended_data;
}

void execute_full_spi_adc_burst(int32_t* output_buffer) {
    trigger_ad7606_conversion_pulse();
    if (!await_ad7606_conversion_ready()) {
        for (uint32_t i = 0; i < 8; i++) {
            output_buffer[i] = -99999;
        }
        return;
    }
    __asm__ __volatile__ ("dsb" ::: "memory");
    for (uint32_t i = 0; i < 8; i++) {
        while (!(*IMXRT_LPSPI1_SR & (1U << 0)));
        *IMXRT_LPSPI1_TDR = 0x00000000;
        while (!(*IMXRT_LPSPI1_SR & (1U << 1)));
        int16_t sample = (int16_t)(*IMXRT_LPSPI1_RDR & 0xFFFF);
        output_buffer[i] = (int32_t)sample;
    }
    __asm__ __volatile__ ("isb" ::: "memory");
}

uint32_t diagnostic_verify_spi_interface(void) {
    *IMXRT_LPSPI1_CR = 0x00000000;
    *IMXRT_LPSPI1_CFGR0 = 0x00000000;
    *IMXRT_LPSPI1_CR = 0x00000001;
    trigger_ad7606_conversion_pulse();
    microsecond_hardware_delay(5);
    if (!(*IMXRT_GPIO3_PSR & AD7606_BUSY_PIN)) {
        return 0x000000A1;
    }
    return 0x00000000;
}
