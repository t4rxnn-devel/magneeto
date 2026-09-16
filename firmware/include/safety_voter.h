#ifndef SAFETY_VOTER_H
#define SAFETY_VOTER_H

#include <stdint.h>
#include <math.h>

typedef struct {
    double primary_reading;
    double secondary_reading;
    double calibrated_offset;
    uint32_t fault_counter;
    uint32_t system_tripped;
} SafetyChannelState;

static inline void init_safety_channels(SafetyChannelState* state) {
    state->primary_reading = 0.0;
    state->secondary_reading = 0.0;
    state->calibrated_offset = 0.0;
    state->fault_counter = 0;
    state->system_tripped = 0;
}

static inline double evaluate_dual_channel_voting(SafetyChannelState* state, double raw_ch1, double raw_ch2) {
    state->primary_reading = raw_ch1;
    state->secondary_reading = raw_ch2 - state->calibrated_offset;
    double reading_delta = fabs(state->primary_reading - state->secondary_reading);
    if (reading_delta > 15.0) {
        state->fault_counter++;
        if (state->fault_counter >= 10) {
            state->system_tripped = 1;
            return -1.0;
        }
        return state->primary_reading;
    }
    if (state->fault_counter > 0) {
        state->fault_counter--;
    }
    return (state->primary_reading + state->secondary_reading) / 2.0;
}

#endif
