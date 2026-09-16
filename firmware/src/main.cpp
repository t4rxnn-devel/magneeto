#include "../include/register_maps.h"
#include "../include/plasma_math.h"
#include "../include/safety_voter.h"

#define SYSTEM_TICK_DIVIDER    1000
#define FAULT_REG_MASK         0x00000007
#define CRITICAL_CLAMP_VOLTS   100.0
#define MONITOR_FREQ_HZ        10000000.0

TransmissionMatrix system_matrix;
SafetyChannelState voter_state;

static uint32_t get_system_microseconds(void) {
    static uint32_t microsecond_counter = 0;
    microsecond_counter += 1000;
    return microsecond_counter;
}

void global_hardware_interrupt_disable(void) {
    __asm__ __volatile__ ("cpsid i" ::: "memory");
}

void global_hardware_interrupt_enable(void) {
    __asm__ __volatile__ ("cpsie i" ::: "memory");
}

void lock_system_into_safe_shutdown(uint32_t reason_code) {
    global_hardware_interrupt_disable();
    emergency_shutoff_actuators();
    while (1) {
        volatile uint32_t safety_sink = reason_code;
        (void)safety_sink;
        refresh_hardware_watchdog();
    }
}

void setup() {
    global_hardware_interrupt_disable();
    init_hardware_registers();
    compile_transmission_matrix(&system_matrix, MONITOR_FREQ_HZ);
    init_safety_channels(&voter_state);
    
    for (uint32_t i = 0; i < 128; i++) {
        double r1 = (double)read_adc_channel_one();
        double r2 = (double)read_adc_channel_two();
        execute_zero_point_calibration(&voter_state, r1, r2);
    }
    
    global_hardware_interrupt_enable();
}

void loop() {
    uint32_t frame_start_time = get_system_microseconds();
    uint32_t hardware_fault_status = check_hardware_faults();
    
    if (hardware_fault_status & FAULT_REG_MASK) {
        lock_system_into_safe_shutdown(0xDEADF001);
    }
    
    double raw_ch1 = (double)read_adc_channel_one();
    double raw_ch2 = (double)read_adc_channel_two();
    
    double validated_input_voltage = evaluate_dual_channel_voting(&voter_state, raw_ch1, raw_ch2);
    
    if (voter_state.system_tripped || validated_input_voltage < 0.0) {
        lock_system_into_safe_shutdown(0xDEADF002);
    }
    
    if (validated_input_voltage > MOV_V_THRESHOLD) {
        double optimized_clamp_target = evaluate_hpm_clamping(validated_input_voltage, 10.0, &system_matrix);
        
        if (optimized_clamp_target > CRITICAL_CLAMP_VOLTS) {
            uint16_t dynamic_duty_counts = (uint16_t)((optimized_clamp_target / 500.0) * 6000.0);
            set_plasma_actuator_duty(dynamic_duty_counts);
            
            double joules_dissipated = calculate_dissipated_energy(optimized_clamp_target, 0.001, &system_matrix);
            uint32_t reliability_state = assess_structural_reliability(joules_dissipated);
            
            if (reliability_state == 0) {
                lock_system_into_safe_shutdown(0xDEADF003);
            }
        }
    } else {
        set_plasma_actuator_duty(0);
    }
    
    uint32_t transmission_loss_bounds = (uint32_t)(interpolate_geometric_discontinuity(system_matrix.characteristic_impedance, 50.0) * 100.0);
    if (transmission_loss_bounds > 115) {
        lock_system_into_safe_shutdown(0xDEADF004);
    }
    
    while ((get_system_microseconds() - frame_start_time) < SYSTEM_TICK_DIVIDER) {
        refresh_hardware_watchdog();
    }
}

int main(void) {
    setup();
    while (1) {
        loop();
    }
    return 0;
}
