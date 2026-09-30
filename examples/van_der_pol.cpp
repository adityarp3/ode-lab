// Van der Pol oscillator: dx/dt = y, dy/dt = mu*(1 - x^2)*y - x
//
// The canonical example of a limit cycle: trajectories from very different
// starting points (near the origin, and far outside) both spiral onto the
// *same* closed orbit. We compute the direction field, both nullclines
// (x-nullcline: y=0; y-nullcline: y = x/(mu*(1-x^2))), and two trajectories
// to show the limit cycle being approached from both inside and outside.
#include <iostream>
#include "odelab/expression.hpp"
#include "odelab/direction_field.hpp"
#include "odelab/solver.hpp"
#include "odelab/rk4.hpp"
#include "odelab/io.hpp"

using namespace odelab;

int main() {
    double mu = 1.5;
    ODEFunc f = make_ode_func({"y", "mu*(1 - x^2)*y - x"}, {{"mu", mu}});

    DirectionField field = compute_direction_field_2d(f, -4.0, 4.0, 41, -4.0, 4.0, 41);
    write_csv("output/vanderpol_direction_field.csv", field);

    std::vector<IsoclinePoint> x_nullcline = extract_isocline(field, 0, 0.0);
    std::vector<IsoclinePoint> y_nullcline = extract_isocline(field, 1, 0.0);
    write_csv("output/vanderpol_x_nullcline.csv", x_nullcline);
    write_csv("output/vanderpol_y_nullcline.csv", y_nullcline);

    RK4Integrator rk4;

    // Starts near the origin (inside the limit cycle) — spirals outward onto it.
    Trajectory traj_inside = solve(f, rk4, State{0.1, 0.1}, 0.0, 30.0, 0.01);
    write_csv("output/vanderpol_traj_inside.csv", traj_inside);

    // Starts far outside the limit cycle — spirals inward onto the same orbit.
    Trajectory traj_outside = solve(f, rk4, State{3.5, 3.5}, 0.0, 30.0, 0.01);
    write_csv("output/vanderpol_traj_outside.csv", traj_outside);

    std::cout << "Van der Pol oscillator: dx/dt = y, dy/dt = mu*(1 - x^2)*y - x, mu = " << mu << "\n";
    std::cout << "Wrote direction field, both nullclines, and two trajectories\n";
    std::cout << "(one starting inside the limit cycle, one starting outside) to output/\n";
    return 0;
}
