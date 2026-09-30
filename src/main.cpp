#include <iostream>
#include <cmath>
#include "odelab/state.hpp"
#include "odelab/system.hpp"
#include "odelab/euler.hpp"
#include "odelab/rk4.hpp"
#include "odelab/solver.hpp"
#include "odelab/io.hpp"

using namespace odelab;

int main() {
    // logistic growth: dx/dt = r*x*(1 - x/K)
    double r = 1.0;
    double K = 10.0;
    double x0 = 1.0;
    double t0 = 0.0;
    double t1 = 10.0;
    double dt = 0.1;

    ODEFunc f = [r, K](double /*t*/, const State& x) -> State {
        return State{ r * x[0] * (1.0 - x[0] / K) };
    };

    EulerIntegrator euler;
    RK4Integrator rk4;

    Trajectory traj_euler = solve(f, euler, State{x0}, t0, t1, dt);
    Trajectory traj_rk4   = solve(f, rk4,   State{x0}, t0, t1, dt);

    // analytic solution for comparison
    auto analytic = [&](double t) {
        return K / (1.0 + ((K - x0) / x0) * std::exp(-r * t));
    };

    write_csv("euler_logistic.csv", traj_euler);
    write_csv("rk4_logistic.csv", traj_rk4);

    std::cout << "Logistic ODE: dx/dt = r*x*(1 - x/K), r=" << r << ", K=" << K << "\n\n";
    std::cout << "t\tEuler\t\tRK4\t\tAnalytic\n";
    for (size_t i = 0; i < traj_euler.t.size(); i += 10) {
        double t = traj_euler.t[i];
        std::cout << t << "\t"
                  << traj_euler.x[i][0] << "\t"
                  << traj_rk4.x[i][0] << "\t"
                  << analytic(t) << "\n";
    }

    std::cout << "\nWrote euler_logistic.csv and rk4_logistic.csv\n";
    return 0;
}