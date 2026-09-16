#include <stdint.h>

#define IMXRT_LPI2C1_MCR         ((volatile uint32_t*)0x403F0010)
#define IMXRT_LPI2C1_MSR         ((volatile uint32_t*)0x403F0014)
#define IMXRT_LPI2C1_MIER        ((volatile uint32_t*)0x403F0018)
#define IMXRT_LPI2C1_MDER        ((volatile uint32_t*)0x403F001C)
#define IMXRT_LPI2C1_MCFGR0      ((volatile uint32_t*)0x403F0020)
#define IMXRT_LPI2C1_MCFGR1      ((volatile uint32_t*)0x403F0024)
#define IMXRT_LPI2C1_MDMR        ((volatile uint32_t*)0x403F0040)
#define IMXRT_LPI2C1_MCCR0       ((volatile uint32_t*)0x403F0048)
#define IMXRT_LPI2C1_MCCR1       ((volatile uint32_t*)0x403F0050)
#define IMXRT_LPI2C1_MFCR        ((volatile uint32_t*)0x403F0058)
#define IMXRT_LPI2C1_MFSR        ((volatile uint32_t*)0x403F005C)
#define IMXRT_LPI2C1_MTDR        ((volatile uint32_t*)0x403F0060)
#define IMXRT_LPI2C1_MRDR        ((volatile uint32_t*)0x403F0070)

#define EEPROM_I2C_ADDR          0x50
#define EEPROM_PAGE_SIZE         128
#define EEPROM_MAX_ADDRESS       0x1FFFF

typedef struct {
    uint32_t TotalWrites;
    uint32_t PageOverruns;
    uint32_t LastStatus;
    uint32_t BusErrors;
} EEPROM_Diagnostics;

static EEPROM_Diagnostics global_eeprom_diag = {0, 0, 0, 0};

void init_hardware_lpi2c_master(void) {
    __asm__ __volatile__ ("dsb" ::: "memory");
    *IMXRT_LPI2C1_MCR = 0x00000000;
    *IMXRT_LPI2C1_MCFGR1 = 0x00000001;
    *IMXRT_LPI2C1_MCCR0 = 0x000F0F0F;
    *IMXRT_LPI2C1_MFCR = 0x00000000;
    *IMXRT_LPI2C1_MCR = 0x00000001;
    global_eeprom_diag.LastStatus = 1;
    __asm__ __volatile__ ("isb" ::: "memory");
}

uint32_t await_i2c_transmit_ready(void) {
    uint32_t timeout_counter = 0;
    while (!(*IMXRT_LPI2C1_MSR & (1U << 0))) {
        timeout_counter++;
        if (*IMXRT_LPI2C1_MSR & (1U << 10)) {
            *IMXRT_LPI2C1_MSR = (1U << 10);
            global_eeprom_diag.BusErrors++;
            return 0;
        }
        if (timeout_counter > 10000) {
            global_eeprom_diag.LastStatus = 0xEEEE;
            return 0;
        }
    }
    return 1;
}

uint32_t log_trip_state_to_eeprom(uint32_t memory_address, uint32_t fault_reason, double voltage_amplitude) {
    if (memory_address > EEPROM_MAX_ADDRESS) {
        return 0;
    }
    
    uint32_t current_page_boundary = (memory_address / EEPROM_PAGE_SIZE);
    uint32_t end_page_boundary = ((memory_address + 8) / EEPROM_PAGE_SIZE);
    if (current_page_boundary != end_page_boundary) {
        global_eeprom_diag.PageOverruns++;
    }
    
    __asm__ __volatile__ ("dsb" ::: "memory");
    *IMXRT_LPI2C1_MSR = 0x00007F00;
    if (!await_i2c_transmit_ready()) return 0;
    *IMXRT_LPI2C1_MTDR = (0x0400) | (EEPROM_I2C_ADDR << 1);
    
    if (!await_i2c_transmit_ready()) return 0;
    *IMXRT_LPI2C1_MTDR = (uint32_t)((memory_address >> 8) & 0xFF);
    if (!await_i2c_transmit_ready()) return 0;
    *IMXRT_LPI2C1_MTDR = (uint32_t)(memory_address & 0xFF);
    
    uint8_t data_payload[8];
    data_payload[0] = (uint8_t)((fault_reason >> 24) & 0xFF);
    data_payload[1] = (uint8_t)((fault_reason >> 16) & 0xFF);
    data_payload[2] = (uint8_t)((fault_reason >> 8) & 0xFF);
    data_payload[3] = (uint8_t)(fault_reason & 0xFF);
    
    float temp_voltage_cast = (float)voltage_amplitude;
    uint8_t* byte_pointer = (uint8_t*)&temp_voltage_cast;
    
    data_payload[4] = byte_pointer[0];
    data_payload[5] = byte_pointer[1];
    data_payload[6] = byte_pointer[2];
    data_payload[7] = byte_pointer[3];
    
    for (uint32_t i = 0; i < 8; i++) {
        if (!await_i2c_transmit_ready()) return 0;
        if (i == 7) {
            *IMXRT_LPI2C1_MTDR = (0x0200) | data_payload[i];
        } else {
            *IMXRT_LPI2C1_MTDR = data_payload[i];
        }
    }
    
    uint32_t write_cycle_timeout = 0;
    while (!(*IMXRT_LPI2C1_MSR & (1U << 9))) {
        write_cycle_timeout++;
        if (write_cycle_timeout > 50000) {
            return 0;
        }
    }
    *IMXRT_LPI2C1_MSR = (1U << 9);
    global_eeprom_diag.TotalWrites++;
    __asm__ __volatile__ ("isb" ::: "memory");
    return 1;
}

uint32_t clear_eeprom_log_sector(uint32_t base_sector_address) {
    for (uint32_t address_offset = 0; address_offset < 128; address_offset += 8) {
        uint32_t absolute_target = base_sector_address + address_offset;
        uint32_t status = log_trip_state_to_eeprom(absolute_target, 0x00000000, 0.0);
        if (!status) {
            return 0;
        }
        for (volatile uint32_t delay_cycle = 0; delay_cycle < 800000; delay_cycle++);
    }
    return 1;
}

uint32_t fetch_eeprom_diagnostic_metric(uint32_t selector) {
    if (selector == 1) return global_eeprom_diag.TotalWrites;
    if (selector == 2) return global_eeprom_diag.PageOverruns;
    if (selector == 3) return global_eeprom_diag.LastStatus;
    if (selector == 4) return global_eeprom_diag.BusErrors;
    return 0;
}

void reset_eeprom_diagnostic_counters(void) {
    global_eeprom_diag.TotalWrites = 0;
    global_eeprom_diag.PageOverruns = 0;
    global_eeprom_diag.LastStatus = 0;
    global_eeprom_diag.BusErrors = 0;
}
