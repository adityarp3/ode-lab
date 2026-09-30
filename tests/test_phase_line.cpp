#include <cassert>
#include <cmath>
#include <iostream>
#include "odelab/phase_line.hpp"
#include "odelab/solver.hpp"
#include "odelab/rk4.hpp"

using namespace odelab;

namespace {

const FixedPoint* find_near(const std::vector<FixedPoint>& fps, double x, double tol = 1e-3) {
    for (const auto& fp : fps) {
        if (std::abs(fp.x - x) < tol) return &fp;
    }
    return nullptr;
}

} // namespace

int main() {
    // Strogatz example: dx/dt = x^2 - 1
    // Fixed points at x=-1 (stable) and x=1 (unstable).
    ScalarField1D f1 = [](double x) { return x * x - 1.0; };
    PhaseLineResult r1 = compute_phase_line(f1, -3.0, 3.0, 1000);

    assert(r1.fixed_points.size() == 2);
    const FixedPoint* stable = find_near(r1.fixed_points, -1.0);
    const FixedPoint* unstable = find_near(r1.fixed_points, 1.0);
    assert(stable != nullptr && stable->stability == Stability::Stable);
    assert(unstable != nullptr && unstable->stability == Stability::Unstable);
    std::cout << "x^2 - 1 test passed: x=-1 stable, x=1 unstable\n";

    // Logistic growth: dx/dt = r*x*(1 - x/K), r=1, K=10
    // Fixed points at x=0 (unstable) and x=K (stable).
    double r = 1.0, K = 10.0;
    ScalarField1D f2 = [r, K](double x) { return r * x * (1.0 - x / K); };
    PhaseLineResult r2 = compute_phase_line(f2, -2.0, 12.0, 1000);

    assert(r2.fixed_points.size() == 2);
    const FixedPoint* zero = find_near(r2.fixed_points, 0.0);
    const FixedPoint* capacity = find_near(r2.fixed_points, K);
    assert(zero != nullptr && zero->stability == Stability::Unstable);
    assert(capacity != nullptr && capacity->stability == Stability::Stable);
    std::cout << "logistic test passed: x=0 unstable, x=K stable\n";

    // Semi-stable example: dx/dt = x^2 (Strogatz's canonical semi-stable case)
    ScalarField1D f3 = [](double x) { return x * x; };
    PhaseLineResult r3 = compute_phase_line(f3, -2.0, 2.0, 1001); // odd count hits x=0 exactly
    assert(r3.fixed_points.size() == 1);
    assert(r3.fixed_points[0].stability == Stability::SemiStable);
    std::cout << "semi-stable test passed: x=0 semi-stable for x^2\n";

    // --- Interactive click-to-drop-a-particle ---
    // Bistable system dx/dt = x - x^3. A user clicking anywhere near the
    // right-hand stable state (x=1, but not exactly on it) should see the
    // particle flow to x=1 when solved forward — this is the whole backend
    // of "click the phase line, watch where it goes."
    {
        ScalarField1D bistable = [](double x) { return x - x * x * x; };
        ODEFunc as_ode = to_ode_func(bistable);

        RK4Integrator rk4;
        Trajectory traj = solve(as_ode, rk4, State{0.6}, 0.0, 20.0, 0.01);
        assert(std::abs(traj.x.back()[0] - 1.0) < 1e-3);

        // Click near the OTHER stable state — should flow to -1 instead.
        Trajectory traj2 = solve(as_ode, rk4, State{-0.3}, 0.0, 20.0, 0.01);
        assert(std::abs(traj2.x.back()[0] - (-1.0)) < 1e-3);
    }
    std::cout << "click-to-drop-particle test passed (to_ode_func + solve reaches correct fixed point)\n";

    std::cout << "All phase line tests passed.\n";
    return 0;
}
