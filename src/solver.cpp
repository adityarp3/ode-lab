#include "odelab/solver.hpp"
#include <cmath>

namespace odelab {

namespace {
bool all_finite(const State& s) {
    for (double v : s) {
        if (!std::isfinite(v)) return false;
    }
    return true;
}
} // namespace

Trajectory solve(const ODEFunc& f, const Integrator& integrator,
                  State x0, double t0, double t1, double dt) {
    Trajectory traj;
    if (dt <= 0.0) return traj;

    double t = t0;
    State x = x0;
    traj.t.push_back(t);
    traj.x.push_back(x);

    int n_steps = static_cast<int>((t1 - t0) / dt + 0.5);
    for (int i = 0; i < n_steps; ++i) {
        double new_t = t + dt;
        State new_x;
        try {
            new_x = integrator.step(f, t, x, dt);
        } catch (const std::exception& e) {
            traj.diverged = true;
            traj.diverged_at_t = new_t;
            traj.divergence_reason = e.what();
            break;
        }

        if (!all_finite(new_x)) {
            traj.diverged = true;
            traj.diverged_at_t = new_t;
            traj.divergence_reason = "Solution diverged to a non-finite value (NaN/Inf)";
            break;
        }

        x = new_x;
        t = new_t;
        traj.t.push_back(t);
        traj.x.push_back(x);
    }
    return traj;
}

} // namespace odelab
