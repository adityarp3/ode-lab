#include <cassert>
#include <cmath>
#include <iostream>
#include "odelab/direction_field.hpp"

using namespace odelab;

namespace {
bool approx(double a, double b, double tol = 1e-9) { return std::abs(a - b) < tol; }
}

int main() {
    // --- 1D slope field: dx/dt = x (independent of t) ---
    // At every grid point, the sampled slope should equal x exactly.
    {
        ODEFunc f = [](double /*t*/, const State& x) -> State { return State{x[0]}; };
        DirectionField field = compute_slope_field_1d(f, /*t*/0.0, 5.0, 6, /*x*/-2.0, 2.0, 9);

        assert(field.samples.size() == 6 * 9);
        for (const auto& s : field.samples) {
            double t = s.position[0];
            double x = s.position[1];
            (void)t;
            assert(s.vector.size() == 1);
            assert(approx(s.vector[0], x, 1e-9));
        }
        std::cout << "slope field sampling test passed (dx/dt = x)\n";

        // Isocline dx/dt = c=1.0 should be the horizontal line x=1, for all t.
        std::vector<IsoclinePoint> iso = extract_isocline(field, 0, 1.0);
        assert(!iso.empty());
        for (const auto& p : iso) {
            assert(approx(p.axis2, 1.0, 1e-9)); // x should be exactly 1 (exact grid hit or interpolated)
        }
        std::cout << "isocline extraction test passed (dx/dt = x, c=1 -> line x=1)\n";
    }

    // --- 2D phase plane: dx/dt = -y, dy/dt = x (rigid rotation) ---
    {
        ODEFunc f = [](double /*t*/, const State& s) -> State {
            return State{-s[1], s[0]};
        };
        DirectionField field = compute_direction_field_2d(f, -2.0, 2.0, 41, -2.0, 2.0, 41);

        // Spot check: at (x=1, y=0), dx/dt=-0=0, dy/dt=1.
        bool found = false;
        for (const auto& s : field.samples) {
            if (approx(s.position[0], 1.0, 1e-9) && approx(s.position[1], 0.0, 1e-9)) {
                assert(approx(s.vector[0], 0.0, 1e-9));
                assert(approx(s.vector[1], 1.0, 1e-9));
                found = true;
            }
        }
        assert(found);
        std::cout << "2D direction field spot check passed (rigid rotation)\n";

        // x-nullcline (component 0, dx/dt=0) should be the line y=0.
        std::vector<IsoclinePoint> x_null = extract_isocline(field, 0, 0.0);
        assert(!x_null.empty());
        for (const auto& p : x_null) {
            assert(approx(p.axis2, 0.0, 1e-9));
        }
        // y-nullcline (component 1, dy/dt=0) should be the line x=0.
        std::vector<IsoclinePoint> y_null = extract_isocline(field, 1, 0.0);
        assert(!y_null.empty());
        for (const auto& p : y_null) {
            assert(approx(p.axis1, 0.0, 1e-9));
        }
        std::cout << "nullcline extraction test passed (x-nullcline y=0, y-nullcline x=0)\n";
    }

    std::cout << "All direction field tests passed.\n";
    return 0;
}