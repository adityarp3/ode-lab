#pragma once
#include <vector>
#include <functional>
#include "odelab/system.hpp"

namespace odelab {

// Right-hand side of an autonomous 1D ODE: dx/dt = f(x). Separate from
// ODEFunc because phase-line analysis is specifically about systems with
// no explicit t-dependence (dx/dt = f(x), not f(t,x)).
using ScalarField1D = std::function<double(double x)>;

enum class Stability { Stable, Unstable, SemiStable };

struct FixedPoint {
    double x;
    Stability stability;
};

struct PhaseLineResult {
    std::vector<double> x;       // sampled x values
    std::vector<double> xdot;    // f(x) at each sample (NaN where f() failed)
    std::vector<FixedPoint> fixed_points;

    // True if f() threw or returned a non-finite value at any sampled x.
    // Those samples are recorded as NaN (visible as gaps if plotted) and
    // skipped during fixed-point search rather than crashing the whole
    // computation — important once f() can be an arbitrary user expression.
    bool has_invalid_samples = false;
};

// Samples f(x) over [x_min, x_max], finds every dx/dt = 0 crossing via
// bisection, and classifies each fixed point by the flow direction on
// either side. This is the "xdot vs x" phase-line tool (Strogatz ch. 2).
PhaseLineResult compute_phase_line(const ScalarField1D& f,
                                    double x_min, double x_max,
                                    int n_samples = 500);

const char* to_string(Stability s);

// Wraps a 1D autonomous ScalarField1D as an ODEFunc (t is accepted but
// ignored), so a point clicked on a phase-line plot can be fed straight
// into solve() to animate where that starting value flows to — no new
// integration logic needed, just adapting the function signature to the
// one solve() already expects.
ODEFunc to_ode_func(const ScalarField1D& f);

} // namespace odelab
