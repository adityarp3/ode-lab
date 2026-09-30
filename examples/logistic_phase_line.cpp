// Logistic growth: dx/dt = r*x*(1 - x/K)
//
// The simplest possible phase-line story: one unstable fixed point (x=0,
// the "no population" state nobody stays at) and one stable fixed point
// (x=K, the carrying capacity everything converges to). Paired with
// bistable_phase_line.cpp (two stable states) to show the contrast between
// monostable and bistable 1D dynamics.
#include <iostream>
#include "odelab/expression.hpp"
#include "odelab/phase_line.hpp"
#include "odelab/io.hpp"

using namespace odelab;

int main() {
    double r = 1.0, K = 10.0;
    ScalarField1D f = make_scalar_field_1d("r*x*(1 - x/K)", {{"r", r}, {"K", K}});

    PhaseLineResult result = compute_phase_line(f, -2.0, 12.0, 800);

    std::cout << "Logistic growth: dx/dt = r*x*(1 - x/K), r=" << r << ", K=" << K << "\n\n";
    std::cout << "Fixed points:\n";
    for (const auto& fp : result.fixed_points) {
        std::cout << "  x = " << fp.x << "  (" << to_string(fp.stability) << ")\n";
    }

    write_csv("output/logistic_phase_line.csv", result);
    write_fixed_points_csv("output/logistic_fixed_points.csv", result.fixed_points);
    std::cout << "\nWrote output/logistic_phase_line.csv and output/logistic_fixed_points.csv\n";
    return 0;
}
