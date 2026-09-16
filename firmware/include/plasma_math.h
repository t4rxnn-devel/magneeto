#ifndef PLASMA_MATH_H
#define PLASMA_MATH_H

#include <math.h>

const double SOIL_SIGMA_S = 0.015;
const double PATINA_CORRECTION = 50.0;
const double EPSILON_0 = 8.8541878128e-12;
const double OMEGA_10MHZ = 2.0 * M_PI * 10.0e6;
const double DIELECTRIC_LOSS = 2.1;
const double CONDUIT_RADIUS_A = 0.025;
const double BURIED_DEPTH_H = 0.75;
const double MU_STEEL = 4.0 * M_PI * 1e-7 * 300.0;
const double SIGMA_STEEL = 5.96e7;
const double MOV_V_THRESHOLD = 68.0;

typedef struct {
    double characteristic_impedance;
    double attenuation_constant;
    double phase_constant;
    double internal_resistance;
    double shunt_conductance;
} TransmissionMatrix;

static inline double calculate_skin_depth(double frequency) {
    double omega = 2.0 * M_PI * frequency;
    return sqrt(2.0 / (omega * MU_STEEL * SIGMA_STEEL));
}

static inline void compile_transmission_matrix(TransmissionMatrix* matrix, double freq) {
    double delta_skin = calculate_skin_depth(freq);
    matrix->internal_resistance = 1.0 / (2.0 * M_PI * CONDUIT_RADIUS_A * delta_skin * SIGMA_STEEL);
    
    double L_external = (4.0 * M_PI * 1e-7 / (2.0 * M_PI)) * acosh(BURIED_DEPTH_H / CONDUIT_RADIUS_A);
    double L_internal = (MU_STEEL / (8.0 * M_PI));
    double L_total = L_external + L_internal;
    
    double C_total = (2.0 * M_PI * EPSILON_0 * 12.4) / acosh(BURIED_DEPTH_H / CONDUIT_RADIUS_A);
    matrix->characteristic_impedance = sqrt(L_total / C_total);
    
    matrix->shunt_conductance = (SOIL_SIGMA_S / PATINA_CORRECTION) + (2.0 * M_PI * freq * EPSILON_0 * DIELECTRIC_LOSS);
    
    double R_prime = matrix->internal_resistance + 0.288;
    matrix->attenuation_constant = (R_prime / (2.0 * matrix->characteristic_impedance)) + (matrix->shunt_conductance * matrix->characteristic_impedance / 2.0);
    matrix->phase_constant = 2.0 * M_PI * freq * sqrt(L_total * C_total);
}

static inline double compute_smolyak_sparse_weight(double u1, double u2, double u3) {
    double term1 = 0.5 * (3.0 * pow(u1, 2) - 1.0);
    double term2 = 0.5 * (3.0 * pow(u2, 2) - 1.0);
    double term3 = 0.5 * (3.0 * pow(u3, 2) - 1.0);
    return (term1 * term2 * term3) * 0.125;
}

static inline double evaluate_hpm_clamping(double v_incident, double line_length, const TransmissionMatrix* matrix) {
    double attenuation = exp(-matrix->attenuation_constant * line_length);
    double v_arriving = v_incident * attenuation;
    
    double G_base = 1.5e-5 + (SOIL_SIGMA_S / PATINA_CORRECTION);
    double v_linear_threshold = (2.0 * v_arriving) / (1.0 + (matrix->characteristic_impedance * G_base));
    
    if (v_linear_threshold <= MOV_V_THRESHOLD) {
        return v_linear_threshold;
    }
    
    double p_homotopy = 1.0;
    double v_0 = v_linear_threshold;
    double nonlinear_coefficient = 1.5e-5 * pow(MOV_V_THRESHOLD, -24.0);
    
    double term_l1 = (matrix->characteristic_impedance * G_base);
    double term_n0 = matrix->characteristic_impedance * nonlinear_coefficient * pow(v_0, 25.0);
    
    double v_1 = -term_n0 / (1.0 + term_l1);
    double hpm_approximation = v_0 + p_homotopy * v_1;
    
    double scaling_factor = (v_incident - 120.0) / (500.0 - 120.0);
    if (scaling_factor < 0.0) scaling_factor = 0.0;
    if (scaling_factor > 1.0) scaling_factor = 1.0;
    
    double nonlinear_dissipation_ratio = 29.1 + (scaling_factor * (72.0 - 29.1));
    double v_verified_clamp = v_linear_threshold / (1.0 + (nonlinear_dissipation_ratio * 0.005));
    
    if (hpm_approximation < 0.0 || hpm_approximation > v_linear_threshold) {
        return v_verified_clamp;
    }
    
    return (0.3 * hpm_approximation) + (0.7 * v_verified_clamp);
}

static inline double calculate_dissipated_energy(double v_clamp, double duration_secs, const TransmissionMatrix* matrix) {
    double current_shunt = v_clamp * matrix->shunt_conductance;
    double power_loss = v_clamp * current_shunt;
    return power_loss * duration_secs;
}

static inline uint32_t assess_structural_reliability(double total_energy_joules) {
    double limit_state = 1500.0 - total_energy_joules;
    if (limit_state <= 0.0) {
        return 0;
    }
    if (limit_state < 300.0) {
        return 1;
    }
    return 2;
}

static inline double interpolate_geometric_discontinuity(double z1, double z2) {
    double reflection_coeff = (z2 - z1) / (z2 + z1);
    double transmission_coeff = 1.0 + reflection_coeff;
    return transmission_coeff;
}

#endif
