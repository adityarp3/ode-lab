#include <cassert>
#include <cmath>
#include <iostream>
#include "odelab/state.hpp"
#include "odelab/euler.hpp"
#include "odelab/rk4.hpp"
#include "odelab/solver.hpp"

using namespace odelab;

int main() {
    // dx/dt = -x, analytic solution x(t) = x0 * exp(-t)
    ODEFunc f = [](double, const State& x) -> State {
        return State{ -x[0] };
    };

    EulerIntegrator euler;
    RK4Integrator rk4;

    double t0 = 0.0, t1 = 1.0, dt = 0.01;
    State x0{1.0};

    Trajectory te = solve(f, euler, x0, t0, t1, dt);
    Trajectory tr = solve(f, rk4, x0, t0, t1, dt);

    double analytic = std::exp(-1.0);

    double err_euler = std::abs(te.x.back()[0] - analytic);
    double err_rk4 = std::abs(tr.x.back()[0] - analytic);

    std::cout << "Euler error: " << err_euler << "\n";
    std::cout << "RK4 error:   " << err_rk4 << "\n";

    assert(err_euler < 0.01);   // Euler: reasonably close at dt=0.01
    assert(err_rk4 < 1e-6);     // RK4: much more accurate at same dt
    assert(err_rk4 < err_euler);

    // Sanity check on a 3D system (Lorenz-shaped) to confirm N-dim works.
    ODEFunc lorenz = [](double, const State& s) -> State {
        double sigma = 10.0, rho = 28.0, beta = 8.0 / 3.0;
        double x = s[0], y = s[1], z = s[2];
        return State{
            sigma * (y - x),
            x * (rho - z) - y,
            x * y - beta * z
        };
    };
    Trajectory t3 = solve(lorenz, rk4, State{1.0, 1.0, 1.0}, 0.0, 1.0, 0.01);
    assert(t3.x.back().size() == 3);

    std::cout << "All tests passed.\n";
    return 0;
}
