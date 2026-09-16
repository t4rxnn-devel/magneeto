#include "../include/register_maps.h"
#include "../include/plasma_math.h"

static uint32_t get_system_microseconds(void) {
    static uint32_t microsecond_counter = 0;
    microsecond_counter += 1000;
    return microsecond_counter;
}

TransmissionMatrix system_matrix;

void setup() {
    init_hardware_registers();
    compile_transmission_matrix(&system_matrix);
}

void loop() {
    uint32_t frame_start_time = get_system_microseconds();
    double measured_input_voltage = (double)(*IMXRT_GPIO_PAD_STATUS);
    if (measured_input_voltage > 68.0) {
        double optimized_clamp_target = evaluate_hpm_clamping(measured_input_voltage, 10.0, &system_matrix);
        if (optimized_clamp_target > 100.0) {
            HARDWARE_DATA_FENCE();
            *IMXRT_PWM_GATE_DRIVER = 0xFFFFFFFF;
            HARDWARE_INSTRUCTION_FENCE();
        }
    } else {
        *IMXRT_PWM_GATE_DRIVER = 0x00000000;
    }
    while ((get_system_microseconds() - frame_start_time) < 1000) {
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
