// Lotka-Volterra predator-prey: dx/dt = a*x - b*x*y, dy/dt = -c*y + d*x*y
//
// x = prey population, y = predator population. Unlike Van der Pol, this
// system has a *center* (not a limit cycle) — every trajectory not at the
// fixed point traces a closed orbit whose size depends on initial
// conditions, giving the classic boom-bust predator-prey cycles. We show
// the direction field, both nullclines (which cross at the nontrivial fixed
// point (c/d, a/b)), and one closed-orbit trajectory.
#include <iostream>
#include "odelab/expression.hpp"
#include "odelab/direction_field.hpp"
#include "odelab/solver.hpp"
#include "odelab/rk4.hpp"
#include "odelab/io.hpp"

using namespace odelab;

int main() {
    double a = 1.0, b = 0.1, c = 1.5, d = 0.075;
    Context params{{"a", a}, {"b", b}, {"c", c}, {"d", d}};
    ODEFunc f = make_ode_func({"a*x - b*x*y", "-c*y + d*x*y"}, params);

    DirectionField field = compute_direction_field_2d(f, 0.0, 30.0, 41, 0.0, 20.0, 41);
    write_csv("output/predator_prey_direction_field.csv", field);

    std::vector<IsoclinePoint> x_nullcline = extract_isocline(field, 0, 0.0);
    std::vector<IsoclinePoint> y_nullcline = extract_isocline(field, 1, 0.0);
    write_csv("output/predator_prey_x_nullcline.csv", x_nullcline);
    write_csv("output/predator_prey_y_nullcline.csv", y_nullcline);

    // Nontrivial fixed point is at (c/d, a/b).
    std::cout << "Lotka-Volterra predator-prey:\n";
    std::cout << "  dx/dt = a*x - b*x*y   (prey)\n";
    std::cout << "  dy/dt = -c*y + d*x*y  (predator)\n";
    std::cout << "  a=" << a << " b=" << b << " c=" << c << " d=" << d << "\n";
    std::cout << "  Nontrivial fixed point (center): (" << c / d << ", " << a / b << ")\n";

    RK4Integrator rk4;
    Trajectory traj = solve(f, rk4, State{10.0, 5.0}, 0.0, 40.0, 0.01);
    write_csv("output/predator_prey_traj.csv", traj);

    std::cout << "Wrote direction field, both nullclines, and one closed-orbit trajectory to output/\n";
    return 0;
}
