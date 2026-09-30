// Lorenz attractor: dx/dt = sigma*(y-x), dy/dt = x*(rho-z) - y, dz/dt = x*y - beta*z
//
// The showcase for N-dim support: a genuinely 3D, chaotic system, solved
// with the exact same solve()/RK4Integrator used for the 1D logistic demo
// back at the start of this project. We also take a 2D direction-field
// slice through the attractor at fixed z (using the `fixed_state` grid
// feature) to show that the same 2D tool built for Van der Pol works on a
// slice of a higher-dimensional system too.
#include <iostream>
#include "odelab/expression.hpp"
#include "odelab/direction_field.hpp"
#include "odelab/solver.hpp"
#include "odelab/rk4.hpp"
#include "odelab/io.hpp"

using namespace odelab;

int main() {
    double sigma = 10.0, rho = 28.0, beta = 8.0 / 3.0;
    Context params{{"sigma", sigma}, {"rho", rho}, {"beta", beta}};
    ODEFunc f = make_ode_func(
        {"sigma*(y - x)", "x*(rho - z) - y", "x*y - beta*z"}, params);

    RK4Integrator rk4;
    Trajectory traj = solve(f, rk4, State{1.0, 1.0, 1.0}, 0.0, 40.0, 0.005);
    write_csv("output/lorenz_trajectory.csv", traj);

    std::cout << "Lorenz attractor: sigma=" << sigma << " rho=" << rho << " beta=" << beta << "\n";
    std::cout << "Integrated " << traj.t.size() << " steps with RK4\n";

    // 2D slice through the attractor at z=25 (roughly its mean height),
    // gridding x and y with z held fixed via fixed_state.
    DirectionField slice = compute_direction_field(
        f, -25.0, 25.0, 31, -30.0, 30.0, 31,
        /*axis1_is_time=*/false, /*fixed_state=*/State{25.0});
    write_csv("output/lorenz_xy_slice_z25.csv", slice);

    std::cout << "Wrote 3D trajectory and an x-y direction-field slice at z=25 to output/\n";
    return 0;
}
