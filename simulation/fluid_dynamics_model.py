import numpy as np

class MHDFluidEngine:
    def __init__(self, channels=12, grid_points=64):
        self.channels = channels
        self.grid_points = grid_points
        self.rho = 1.225
        self.mu_0 = 4.0 * np.pi * 1e-7
        self.u = np.ones(self.grid_points) * 343.0
        self.B = np.zeros(self.grid_points)
        self.J = np.zeros(self.grid_points)

    def apply_lorentz_field_stepping(self, current_amps, gate_status):
        if not gate_status:
            self.B.fill(0.0)
            self.J.fill(0.0)
            return self.u
        spatial_mesh = np.linspace(0.0, 0.05, self.grid_points)
        for i in range(self.grid_points):
            self.B[i] = (self.mu_0 * current_amps) / (2.0 * np.pi * (spatial_mesh[i] + 0.001))
            self.J[i] = current_amps / 0.0025
            lorentz_force_density = self.J[i] * self.B[i]
            acceleration = lorentz_force_density / self.rho
            self.u[i] += acceleration * 0.001
        return self.u

    def evaluate_boundary_pressure_drop(self):
        kinetic_energy_density = 0.5 * self.rho * (self.u ** 2)
        total_inlet_pressure = kinetic_energy_density[0]
        total_outlet_pressure = kinetic_energy_density[-1]
        return float(total_inlet_pressure - total_outlet_pressure)
