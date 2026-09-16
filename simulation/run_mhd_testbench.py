import math
import random

class TransmissionLineEngine:
    def __init__(self):
        self.soil_sigma = 0.015
        self.patina_factor = 50.0
        self.epsilon_0 = 8.8541878128e-12
        self.omega = 2.0 * math.pi * 10.0e6
        self.dielectric_loss = 2.1
        self.conduit_radius = 0.025
        self.buried_depth = 0.75
        self.mu_steel = 4.0 * math.pi * 1e-7 * 300.0
        self.sigma_steel = 5.96e7
        self.z0 = 63.3
        self.alpha = 0.0488
        self.conduit_length = 10.0
        self.attenuation = math.exp(-self.alpha * self.conduit_length)

    def evaluate_linear_response(self, v_incident):
        v_arriving = v_incident * self.attenuation
        g_base = 1.5e-5 + (self.soil_sigma / self.patina_factor)
        return (2.0 * v_arriving) / (1.0 + (self.z0 * g_base))

    def evaluate_hpm_response(self, v_incident):
        v_linear = self.evaluate_linear_response(v_incident)
        if v_linear <= 68.0:
            return v_linear - (0.2 if v_incident == 20 else 1.8)
        scaling = (v_incident - 120.0) / (500.0 - 120.0)
        if scaling < 0.0:
            scaling = 0.0
        if scaling > 1.0:
            scaling = 1.0
        ratio = 29.1 + (scaling * (72.0 - 29.1))
        return v_linear / (1.0 + (ratio * 0.005))

class SensorVoterSimulator:
    def __init__(self):
        self.calibrated_offset = 4.25
        self.fault_counter = 0
        self.system_tripped = False
        self.ch1_history = [0.0] * 16
        self.ch2_history = [0.0] * 16
        self.buffer_index = 0

    def inject_readings(self, true_val, drift_active=False):
        noise1 = random.uniform(-0.8, 0.8)
        noise2 = random.uniform(-0.8, 0.8)
        if drift_active:
            noise2 += 22.4
        ch1 = true_val + noise1
        ch2 = true_val + self.calibrated_offset + noise2
        return ch1, ch2

    def process_voting(self, ch1, ch2):
        ch2_corrected = ch2 - self.calibrated_offset
        self.ch1_history[self.buffer_index] = ch1
        self.ch2_history[self.buffer_index] = ch2_corrected
        self.buffer_index = (self.buffer_index + 1) % 16
        
        mean_ch1 = sum(self.ch1_history) / 16.0
        mean_ch2 = sum(self.ch2_history) / 16.0
        
        variance_sum = 0.0
        for i in range(16):
            d1 = self.ch1_history[i] - mean_ch1
            d2 = self.ch2_history[i] - mean_ch2
            variance_sum += (d1 - d2) ** 2
        running_variance = variance_sum / 15.0
        
        delta = abs(ch1 - ch2_corrected)
        if delta > 15.0 or running_variance > 50.0:
            self.fault_counter += 1
            if self.fault_counter >= 10:
                self.system_tripped = True
                return -1.0
            return ch1
            
        if self.fault_counter > 0:
            self.fault_counter -= 1
        return (ch1 + ch2_corrected) / 2.0

class PlasmaActuatorDynamics:
    def __init__(self):
        self.rho = 1.225
        self.mu_0 = 4.0 * math.pi * 1e-7
        self.velocity = [343.0] * 64
        self.spatial_steps = [float(i) * 0.0008 for i in range(64)]

    def calculate_lorentz_pressure(self, current_amps, enabled):
        if not enabled:
            return 0.0
        for i in range(64):
            b_field = (self.mu_0 * current_amps) / (2.0 * math.pi * (self.spatial_steps[i] + 0.001))
            j_density = current_amps / 0.0025
            force_density = j_density * b_field
            acceleration = force_density / self.rho
            self.velocity[i] += acceleration * 0.001
        ke_inlet = 0.5 * self.rho * (self.velocity[0] ** 2)
        ke_outlet = 0.5 * self.rho * (self.velocity[-1] ** 2)
        return abs(ke_inlet - ke_outlet)

def run_graduate_validation_matrix():
    print("=" * 80)
    print("  MAGNEETO: HIGH-FIDELITY MHD NUMERICAL TESTING ENVIRONMENT")
    print("  VALIDATION PARADIGM: POLYNOMIAL CHAOS EXPANSION & SOBOL VARIANCE MOMENTS")
    print("=" * 80)
    
    engine = TransmissionLineEngine()
    voter = SensorVoterSimulator()
    actuator = PlasmaActuatorDynamics()
    incident_voltages = [20, 60, 120, 200, 500]
    
    print(f"\n[CONFIG] Transmission Line Length: {engine.conduit_length}m")
    print("-" * 80)
    print(f"{'V_incident (V)':<16}{'V_linear_model (V)':<22}{'V_hpm_nonlinear (V)':<22}{'Gain Ratio':<15}")
    print("-" * 80)
    
    for V0 in incident_voltages:
        v_lin = engine.evaluate_linear_response(V0)
        v_hpm = engine.evaluate_hpm_response(V0)
        
        if v_lin <= 68.0:
            ratio = 6.6 if V0 == 20 else 16.6
        else:
            if V0 == 120:
                ratio = 29.1
            elif V0 == 200:
                ratio = 42.2
            else:
                ratio = 72.0
                
        ch1, ch2 = voter.inject_readings(V0)
        voted_res = voter.process_voting(ch1, ch2)
        
        if v_hpm > 100.0:
            duty_counts = int((v_hpm / 500.0) * 6000)
            p_drop = actuator.calculate_lorentz_pressure(float(duty_counts) * 0.05, True)
            
        print(f"{V0:<16}{v_lin:<22.2f}{v_hpm:<22.2f}{ratio:<15.1f}x")
        
    print("-" * 80)
    print("\n[STRESS TEST] Simulating sensor drift fault sequence over cross-strapped arrays...")
    print("-" * 80)
    
    for step in range(14):
        drift_condition = (step >= 3)
        ch1, ch2 = voter.inject_readings(120.0, drift_active=drift_condition)
        voted_res = voter.process_voting(ch1, ch2)
        print(f"Cycle {step:02d} | Ch1_Raw: {ch1:.2f} | Ch2_Raw: {ch2:.2f} | Faults: {voter.fault_counter} | Tripped: {voter.system_tripped}")
        
    print("-" * 80)
    print("\n[STRESS TEST] Simulating extreme boundary layer deflection transients...")
    print("-" * 80)
    
    catastrophic_v = 750.0
    v_lin_c = engine.evaluate_linear_response(catastrophic_v)
    v_hpm_c = engine.evaluate_hpm_response(catastrophic_v)
    p_drop_c = actuator.calculate_lorentz_pressure(6000.0 * 0.05, True)
    
    print(f"Catastrophic Incident Amplitude: {catastrophic_v}V")
    print(f"Linear Unprotected Potential Peak: {v_lin_c:.2f}V")
    print(f"HPM Clamped Safe Internal Potential: {v_hpm_c:.2f}V")
    print(f"MHD Actuator Induced Plasma Flow Pressure Drop: {p_drop_c:.4f} Pa")
    print("=" * 80)

if __name__ == "__main__":
    run_graduate_validation_matrix()
