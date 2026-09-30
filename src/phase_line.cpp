#include "odelab/phase_line.hpp"
#include <cmath>
#include <limits>

namespace odelab {

namespace {

// Wraps every call to the user-supplied f so a thrown exception (e.g. a
// user expression hitting division by zero) or a silently non-finite
// result becomes a NaN the caller can detect, rather than crashing the
// whole phase-line computation over one bad sample.
double safe_call(const ScalarField1D& f, double x) {
    try {
        return f(x); // may legitimately be NaN/Inf; caller checks
    } catch (...) {
        return std::numeric_limits<double>::quiet_NaN();
    }
}

double bisect_root(const ScalarField1D& f, double a, double b, int max_iter = 60) {
    double fa = safe_call(f, a);
    for (int i = 0; i < max_iter; ++i) {
        double m = 0.5 * (a + b);
        double fm = safe_call(f, m);
        if (std::isnan(fm)) {
            // Can't safely refine further; return the best estimate so far.
            return m;
        }
        if (std::abs(fm) < 1e-14 || (b - a) < 1e-12) return m;
        if (std::isnan(fa) || (fa > 0) == (fm > 0)) {
            a = m;
            fa = fm;
        } else {
            b = m;
        }
    }
    return 0.5 * (a + b);
}

// Stable: flow points inward from both sides (f>0 to the left, f<0 to the right).
// Unstable: flow points outward from both sides.
// Semi-stable: same sign on both sides (flow passes through in one direction).
Stability classify(const ScalarField1D& f, double x_root, double h) {
    double f_left = safe_call(f, x_root - h);
    double f_right = safe_call(f, x_root + h);

    if (std::isnan(f_left) || std::isnan(f_right)) {
        return Stability::SemiStable; // can't determine flow direction reliably
    }

    bool left_positive = f_left > 0;
    bool right_positive = f_right > 0;

    if (left_positive && !right_positive) return Stability::Stable;
    if (!left_positive && right_positive) return Stability::Unstable;
    return Stability::SemiStable;
}

} // namespace

PhaseLineResult compute_phase_line(const ScalarField1D& f,
                                    double x_min, double x_max,
                                    int n_samples) {
    PhaseLineResult result;
    if (n_samples < 2) n_samples = 2;

    result.x.reserve(n_samples);
    result.xdot.reserve(n_samples);

    double step = (x_max - x_min) / (n_samples - 1);
    for (int i = 0; i < n_samples; ++i) {
        double x = x_min + i * step;
        double val = safe_call(f, x);
        result.x.push_back(x);
        result.xdot.push_back(val);
        if (std::isnan(val)) result.has_invalid_samples = true;
    }

    double classify_h = step > 1e-6 ? step * 0.1 : 1e-6;
    for (int i = 0; i + 1 < n_samples; ++i) {
        double f0 = result.xdot[i];
        double f1 = result.xdot[i + 1];
        if (std::isnan(f0) || std::isnan(f1)) continue; // can't test this interval

        if (f0 == 0.0) {
            // Exact zero landed on this grid point — record it once, and
            // don't let the sign-change branch below re-detect it via the
            // neighboring interval.
            if (i == 0 || result.xdot[i - 1] != 0.0) {
                double root = result.x[i];
                result.fixed_points.push_back({root, classify(f, root, classify_h)});
            }
            continue;
        }
        if (f1 == 0.0) {
            // Will be handled as f0 on the next iteration instead.
            continue;
        }
        if ((f0 > 0) != (f1 > 0)) {
            double root = bisect_root(f, result.x[i], result.x[i + 1]);
            result.fixed_points.push_back({root, classify(f, root, classify_h)});
        }
    }

    // The loop above never visits the last sample as f0 — check it separately.
    int last = n_samples - 1;
    if (last >= 0 && !std::isnan(result.xdot[last]) && result.xdot[last] == 0.0 &&
        (last == 0 || result.xdot[last - 1] != 0.0)) {
        double root = result.x[last];
        result.fixed_points.push_back({root, classify(f, root, classify_h)});
    }

    return result;
}

const char* to_string(Stability s) {
    switch (s) {
        case Stability::Stable: return "stable";
        case Stability::Unstable: return "unstable";
        case Stability::SemiStable: return "semi-stable";
    }
    return "unknown";
}

ODEFunc to_ode_func(const ScalarField1D& f) {
    return [f](double /*t*/, const State& x) -> State {
        return State{f(x[0])};
    };
}

} // namespace odelab
