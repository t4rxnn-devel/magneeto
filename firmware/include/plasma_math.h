#ifndef PLASMA_MATH_H
#define PLASMA_MATH_H

#include <math.h>

const double SOIL_SIGMA_S = 0.015;
const double PATINA_CORRECTION = 50.0;
const double EPSILON_0 = 8.8541878128e-12;
const double OMEGA_10MHZ = 2.0 * M_PI * 10.0e6;
const double DIELECTRIC_LOSS = 2.1;

typedef struct {
    double characteristic_impedance;
    double attenuation_constant;
} TransmissionMatrix;

static inline void compile_transmission_matrix(TransmissionMatrix* matrix) {
    double L_per_meter = 0.7495e-6;
    double C_per_meter = 187.0e-12;
    matrix->characteristic_impedance = sqrt(L_per_meter / C_per_meter);
    double G_effective = (SOIL_SIGMA_S / PATINA_CORRECTION) + (OMEGA_10MHZ * EPSILON_0 * DIELECTRIC_LOSS);
    double R_series = 0.288;
    matrix->attenuation_constant = (R_series / (2.0 * matrix->characteristic_impedance)) + (G_effective * matrix->characteristic_impedance / 2.0);
}

static inline double evaluate_hpm_clamping(double v_incident, double line_length, const TransmissionMatrix* matrix) {
    double attenuation = exp(-matrix->attenuation_constant * line_length);
    double v_arriving = v_incident * attenuation;
    double G_base = 1.5e-5 + (SOIL_SIGMA_S / PATINA_CORRECTION);
    double v_linear_threshold = (2.0 * v_arriving) / (1.0 + (matrix->characteristic_impedance * G_base));
    if (v_linear_threshold <= 68.0) {
        return v_linear_threshold;
    }
    double scaling_factor = (v_incident - 120.0) / (500.0 - 120.0);
    if (scaling_factor < 0.0) scaling_factor = 0.0;
    if (scaling_factor > 1.0) scaling_factor = 1.0;
    double nonlinear_dissipation_ratio = 29.1 + (scaling_factor * (72.0 - 29.1));
    return v_linear_threshold / (1.0 + (nonlinear_dissipation_ratio * 0.005));
}

#endif
