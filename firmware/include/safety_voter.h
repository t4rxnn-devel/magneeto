#ifndef SAFETY_VOTER_H
#define SAFETY_VOTER_H

#include <stdint.h>
#include <math.h>

const double VOTER_MAX_DELTA = 15.0;
const uint32_t VOTER_FAULT_THRESHOLD = 10;
const double EMA_ALPHA_SMOOTH = 0.15;
const uint32_t DRIFT_WINDOW_SIZE = 16;

typedef struct {
    double primary_reading;
    double secondary_reading;
    double primary_filtered;
    double secondary_filtered;
    double calibrated_offset;
    double running_variance;
    double drift_buffer_ch1[DRIFT_WINDOW_SIZE];
    double drift_buffer_ch2[DRIFT_WINDOW_SIZE];
    uint32_t buffer_index;
    uint32_t fault_counter;
    uint32_t static_fault_counter_ch1;
    uint32_t static_fault_counter_ch2;
    uint32_t system_tripped;
    uint32_t calibration_complete;
} SafetyChannelState;

static inline void init_safety_channels(SafetyChannelState* state) {
    state->primary_reading = 0.0;
    state->secondary_reading = 0.0;
    state->primary_filtered = 0.0;
    state->secondary_filtered = 0.0;
    state->calibrated_offset = 0.0;
    state->running_variance = 0.0;
    state->buffer_index = 0;
    state->fault_counter = 0;
    state->static_fault_counter_ch1 = 0;
    state->static_fault_counter_ch2 = 0;
    state->system_tripped = 0;
    state->calibration_complete = 0;
    for (uint32_t i = 0; i < DRIFT_WINDOW_SIZE; i++) {
        state->drift_buffer_ch1[i] = 0.0;
        state->drift_buffer_ch2[i] = 0.0;
    }
}

static inline void execute_zero_point_calibration(SafetyChannelState* state, double raw_ch1, double raw_ch2) {
    double accum_offset = 0.0;
    for (uint32_t i = 0; i < 64; i++) {
        accum_offset += (raw_ch2 - raw_ch1);
    }
    state->calibrated_offset = accum_offset / 64.0;
    state->primary_filtered = raw_ch1;
    state->secondary_filtered = raw_ch2 - state->calibrated_offset;
    state->calibration_complete = 1;
}

static inline void apply_exponential_filter(SafetyChannelState* state, double raw_ch1, double raw_ch2) {
    double corrected_ch2 = raw_ch2 - state->calibrated_offset;
    state->primary_filtered = (EMA_ALPHA_SMOOTH * raw_ch1) + ((1.0 - EMA_ALPHA_SMOOTH) * state->primary_filtered);
    state->secondary_filtered = (EMA_ALPHA_SMOOTH * corrected_ch2) + ((1.0 - EMA_ALPHA_SMOOTH) * state->secondary_filtered);
}

static inline void update_drift_variance_buffers(SafetyChannelState* state, double ch1, double ch2) {
    state->drift_buffer_ch1[state->buffer_index] = ch1;
    state->drift_buffer_ch2[state->buffer_index] = ch2;
    state->buffer_index = (state->buffer_index + 1) % DRIFT_WINDOW_SIZE;
    
    double mean_ch1 = 0.0;
    double mean_ch2 = 0.0;
    for (uint32_t i = 0; i < DRIFT_WINDOW_SIZE; i++) {
        mean_ch1 += state->drift_buffer_ch1[i];
        mean_ch2 += state->drift_buffer_ch2[i];
    }
    mean_ch1 /= DRIFT_WINDOW_SIZE;
    mean_ch2 /= DRIFT_WINDOW_SIZE;
    
    double variance_sum = 0.0;
    for (uint32_t i = 0; i < DRIFT_WINDOW_SIZE; i++) {
        double d1 = state->drift_buffer_ch1[i] - mean_ch1;
        double d2 = state->drift_buffer_ch2[i] - mean_ch2;
        variance_sum += pow(d1 - d2, 2);
    }
    state->running_variance = variance_sum / (DRIFT_WINDOW_SIZE - 1);
}

static inline uint32_t evaluate_sensor_stuck_faults(SafetyChannelState* state) {
    uint32_t stuck_ch1 = 1;
    uint32_t stuck_ch2 = 1;
    for (uint32_t i = 1; i < DRIFT_WINDOW_SIZE; i++) {
        if (state->drift_buffer_ch1[i] != state->drift_buffer_ch1[0]) stuck_ch1 = 0;
        if (state->drift_buffer_ch2[i] != state->drift_buffer_ch2[0]) stuck_ch2 = 0;
    }
    if (stuck_ch1) state->static_fault_counter_ch1++;
    else if (state->static_fault_counter_ch1 > 0) state->static_fault_counter_ch1--;
    
    if (stuck_ch2) state->static_fault_counter_ch2++;
    else if (state->static_fault_counter_ch2 > 0) state->static_fault_counter_ch2--;
    
    if (state->static_fault_counter_ch1 > 50 || state->static_fault_counter_ch2 > 50) {
        return 1;
    }
    return 0;
}

static inline double evaluate_dual_channel_voting(SafetyChannelState* state, double raw_ch1, double raw_ch2) {
    if (state->system_tripped) {
        return -1.0;
    }
    if (!state->calibration_complete) {
        execute_zero_point_calibration(state, raw_ch1, raw_ch2);
    }
    state->primary_reading = raw_ch1;
    state->secondary_reading = raw_ch2 - state->calibrated_offset;
    apply_exponential_filter(state, raw_ch1, raw_ch2);
    update_drift_variance_buffers(state, state->primary_filtered, state->secondary_filtered);
    
    if (evaluate_sensor_stuck_faults(state)) {
        state->system_tripped = 1;
        return -3.0;
    }
    
    double reading_delta = fabs(state->primary_filtered - state->secondary_filtered);
    if (reading_delta > VOTER_MAX_DELTA || state->running_variance > 50.0) {
        state->fault_counter++;
        if (state->fault_counter >= VOTER_FAULT_THRESHOLD) {
            state->system_tripped = 1;
            return -2.0;
        }
        return state->primary_filtered;
    }
    
    if (state->fault_counter > 0) {
        state->fault_counter--;
    }
    return (state->primary_filtered + state->secondary_filtered) / 2.0;
}

static inline void force_manual_voter_reset(SafetyChannelState* state) {
    state->fault_counter = 0;
    state->static_fault_counter_ch1 = 0;
    state->static_fault_counter_ch2 = 0;
    state->system_tripped = 0;
}

#endif
