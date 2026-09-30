#pragma once
#include <vector>
#include <string>
#include "odelab/system.hpp"
#include "odelab/integrator.hpp"

namespace odelab {

// Result of integrating dx/dt = f(t,x) over a time span — this is
// exactly the data the "t vs x(t)" tool plots, and what phase-plane
// tools sample x(t), y(t), ... from.
struct Trajectory {
    std::vector<double> t;
    std::vector<State> x;

    // If the solution diverged (a component became NaN/Inf) or f() threw
    // (e.g. a user expression hit a division by zero or domain error mid-
    // integration), the trajectory is truncated at the last good point
    // rather than filled with garbage, and these are set so a caller (a
    // web UI, in particular) can show why instead of just cutting off.
    bool diverged = false;
    double diverged_at_t = 0.0;
    std::string divergence_reason;
};

// Integrate from t0 to t1 with fixed step dt, starting at x0.
Trajectory solve(const ODEFunc& f, const Integrator& integrator,
                  State x0, double t0, double t1, double dt);

} // namespace odelab
