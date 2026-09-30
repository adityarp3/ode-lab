// Bistable system: dx/dt = x - x^3
//
// Classic pitchfork normal form (Strogatz 3.1-3.4 territory). Two stable
// fixed points at x=+-1 and an unstable one at x=0 — a 1D model of any
// system with two attracting states (a switch, a bent beam, an activated
// gene). Built from a user-typed expression to double as a demo of the
// expression parser feeding straight into the phase-line tool.
#include <iostream>
#include "odelab/expression.hpp"
#include "odelab/phase_line.hpp"
#include "odelab/io.hpp"

using namespace odelab;

int main() {
    ScalarField1D f = make_scalar_field_1d("x - x^3");

    PhaseLineResult result = compute_phase_line(f, -2.0, 2.0, 800);

    std::cout << "Bistable system: dx/dt = x - x^3\n\n";
    std::cout << "Fixed points:\n";
    for (const auto& fp : result.fixed_points) {
        std::cout << "  x = " << fp.x << "  (" << to_string(fp.stability) << ")\n";
    }

    write_csv("output/bistable_phase_line.csv", result);
    write_fixed_points_csv("output/bistable_fixed_points.csv", result.fixed_points);
    std::cout << "\nWrote output/bistable_phase_line.csv and output/bistable_fixed_points.csv\n";
    return 0;
}
