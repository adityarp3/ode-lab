// Non-autonomous linear ODE: dx/dt = t - x
//
// Since the right-hand side depends explicitly on t, this can't be reduced
// to a phase line — it needs a genuine t-x slope field. Solved analytically
// via integrating factor: x(t) = t - 1 + (x0+1)*e^(-t). We plot the slope
// field, the isocline dx/dt=0 (which is exactly the line x=t), and three
// solution trajectories from different initial conditions all converging
// toward that isocline — a nice visual of why isoclines matter.
#include <iostream>
#include "odelab/expression.hpp"
#include "odelab/direction_field.hpp"
#include "odelab/solver.hpp"
#include "odelab/rk4.hpp"
#include "odelab/io.hpp"

using namespace odelab;

int main() {
    ODEFunc f = make_ode_func({"t - x"});

    DirectionField field = compute_slope_field_1d(f, 0.0, 5.0, 26, -3.0, 6.0, 46);
    write_csv("output/nonautonomous_slope_field.csv", field);

    // Isocline where dx/dt = 0 -> should trace the line x = t.
    std::vector<IsoclinePoint> zero_iso = extract_isocline(field, 0, 0.0);
    write_csv("output/nonautonomous_isocline_0.csv", zero_iso);

    RK4Integrator rk4;
    double x0_values[] = {-2.0, 0.0, 4.0};
    for (double x0 : x0_values) {
        Trajectory traj = solve(f, rk4, State{x0}, 0.0, 5.0, 0.02);
        write_csv("output/nonautonomous_traj_x0_" + std::to_string(static_cast<int>(x0)) + ".csv", traj);
    }

    std::cout << "Non-autonomous: dx/dt = t - x\n";
    std::cout << "Analytic solution: x(t) = t - 1 + (x0+1)*exp(-t)\n";
    std::cout << "Wrote slope field, isocline dx/dt=0 (the line x=t), and 3 trajectories to output/\n";
    return 0;
}
