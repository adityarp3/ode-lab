#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include "odelab/expression.hpp"
#include "odelab/solver.hpp"
#include "odelab/euler.hpp"
#include "odelab/rk4.hpp"
#include "odelab/phase_line.hpp"
#include "odelab/direction_field.hpp"
#include "odelab/io.hpp"

using namespace odelab;

namespace {

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

} // namespace

int main() {
    // --- Expression hardening: clear errors instead of silent NaN/crash ---
    {
        bool threw = false;
        try {
            ScalarField1D f = make_scalar_field_1d("1 / x");
            f(0.0);
        } catch (const std::runtime_error& e) {
            threw = true;
            assert(contains(e.what(), "Division by zero"));
        }
        assert(threw);
    }
    {
        bool threw = false;
        try {
            ScalarField1D f = make_scalar_field_1d("sqrt(x)");
            f(-4.0);
        } catch (const std::runtime_error& e) {
            threw = true;
            assert(contains(e.what(), "sqrt"));
        }
        assert(threw);
    }
    {
        bool threw = false;
        try {
            ScalarField1D f = make_scalar_field_1d("log(x)");
            f(-1.0);
        } catch (const std::runtime_error& e) {
            threw = true;
            assert(contains(e.what(), "log"));
        }
        assert(threw);
    }
    {
        // pow(negative, fractional) doesn't throw inside std::pow, it just
        // returns NaN — the catch-all finite check in make_scalar_field_1d
        // should turn that into a clear exception too.
        bool threw = false;
        try {
            ScalarField1D f = make_scalar_field_1d("(-4)^0.5");
            f(0.0);
        } catch (const std::runtime_error& e) {
            threw = true;
            assert(contains(e.what(), "non-finite"));
        }
        assert(threw);
    }
    std::cout << "expression hardening tests passed\n";

    // --- Solver divergence: deterministic case (hardcoded lambda blows up) ---
    {
        int call_count = 0;
        ODEFunc f = [&call_count](double, const State&) -> State {
            ++call_count;
            if (call_count > 3) return State{std::numeric_limits<double>::infinity()};
            return State{1.0};
        };
        RK4Integrator rk4;
        Trajectory traj = solve(f, rk4, State{0.0}, 0.0, 10.0, 1.0);
        assert(traj.diverged);
        assert(!traj.divergence_reason.empty());
        assert(traj.t.size() < 11); // truncated well before the full 10 steps
        std::cout << "solver divergence test passed (deterministic Inf case)\n";
    }

    // --- Solver divergence: real domain error mid-integration via expression ---
    {
        // dx/dt = -sqrt(x), x0=1, with a large Euler step that overshoots
        // past x=0 — the next step's sqrt(negative) should be caught by
        // the expression layer and surfaced as a divergence, not a crash.
        ODEFunc f = make_ode_func({"-sqrt(x)"});
        EulerIntegrator euler;
        Trajectory traj = solve(f, euler, State{1.0}, 0.0, 5.0, 0.6);
        assert(traj.diverged);
        assert(contains(traj.divergence_reason, "sqrt"));
        std::cout << "solver divergence test passed (expression domain error mid-integration)\n";
    }

    // --- Phase line: bad samples don't crash the whole computation ---
    {
        // dx/dt = 1/x has a real discontinuity at x=0 that lands exactly on
        // a grid sample when sampling a symmetric range with an odd count.
        ScalarField1D f = make_scalar_field_1d("1 / x");
        PhaseLineResult result = compute_phase_line(f, -2.0, 2.0, 401); // hits x=0 exactly
        assert(result.has_invalid_samples);
        // The rest of the (finite) samples should still be populated correctly.
        bool found_finite = false;
        for (double v : result.xdot) {
            if (std::isfinite(v)) { found_finite = true; break; }
        }
        assert(found_finite);
        std::cout << "phase line hardening test passed (division-by-zero sample doesn't crash scan)\n";
    }

    // --- Direction field: bad samples flagged, isoclines skip them cleanly ---
    {
        // dx/dt = x, dy/dt = 1/y — the y-nullcline itself passes straight
        // through the line where 1/y is undefined (y=0).
        ODEFunc f = make_ode_func({"x", "1 / y"});
        DirectionField field = compute_direction_field_2d(f, -2.0, 2.0, 21, -2.0, 2.0, 21);
        assert(field.has_invalid_samples);

        // extract_isocline should still run without throwing, using only
        // the valid samples.
        std::vector<IsoclinePoint> x_null = extract_isocline(field, 0, 0.0);
        assert(!x_null.empty());
        std::cout << "direction field hardening test passed (invalid grid points flagged, isocline still extracts)\n";
    }

    // --- JSON output: structurally sane and round-trippable by inspection ---
    {
        RK4Integrator rk4;
        ODEFunc f = make_ode_func({"-x"});
        Trajectory traj = solve(f, rk4, State{1.0}, 0.0, 1.0, 0.5);
        std::string j = to_json(traj);
        assert(contains(j, "\"t\":["));
        assert(contains(j, "\"x\":["));
        assert(contains(j, "\"diverged\":false"));
        assert(contains(j, "\"diverged_at_t\":null"));
        assert(contains(j, "\"divergence_reason\":null"));
    }
    {
        // A diverged trajectory should surface the reason as a JSON string,
        // and diverged_at_t as a real number, not null.
        int call_count = 0;
        ODEFunc f = [&call_count](double, const State&) -> State {
            ++call_count;
            if (call_count > 1) return State{std::numeric_limits<double>::quiet_NaN()};
            return State{1.0};
        };
        RK4Integrator rk4;
        Trajectory traj = solve(f, rk4, State{0.0}, 0.0, 5.0, 1.0);
        std::string j = to_json(traj);
        assert(contains(j, "\"diverged\":true"));
        assert(!contains(j, "\"diverged_at_t\":null"));
        assert(contains(j, "\"divergence_reason\":\""));
    }
    {
        ScalarField1D f = make_scalar_field_1d("x - x^3");
        PhaseLineResult result = compute_phase_line(f, -2.0, 2.0, 400);
        std::string j = to_json(result);
        assert(contains(j, "\"fixed_points\":["));
        assert(contains(j, "\"stability\":\"stable\""));
        assert(contains(j, "\"stability\":\"unstable\""));
    }
    {
        ODEFunc f = make_ode_func({"-y", "x"});
        DirectionField field = compute_direction_field_2d(f, -1.0, 1.0, 3, -1.0, 1.0, 3);
        std::string j = to_json(field);
        assert(contains(j, "\"nx\":3"));
        assert(contains(j, "\"samples\":["));
        assert(contains(j, "\"position\":["));
        assert(contains(j, "\"vector\":["));

        std::vector<IsoclinePoint> iso = extract_isocline(field, 0, 0.0);
        std::string iso_json = to_json(iso);
        assert(iso_json.front() == '[');
        assert(iso_json.back() == ']');
    }
    std::cout << "JSON output tests passed\n";

    std::cout << "All hardening/JSON tests passed.\n";
    return 0;
}
