// Python bindings for the odelab C++ core.
//
// Every function here returns a JSON string built by the C++ side's own
// to_json() (see io.hpp). FastAPI can hand that string straight back as
// the HTTP response body — no re-serialization needed on the Python side.
//
// Build/import smoke_test.cpp FIRST if you haven't already — it isolates
// "is my pybind11/compiler toolchain even working" from "is this specific
// binding code correct", which is a much faster thing to debug.
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "odelab/expression.hpp"
#include "odelab/solver.hpp"
#include "odelab/euler.hpp"
#include "odelab/rk4.hpp"
#include "odelab/phase_line.hpp"
#include "odelab/direction_field.hpp"
#include "odelab/io.hpp"

namespace py = pybind11;
using namespace odelab;

namespace {

// --- t vs x(t) ---
//
// One or more expressions (one per state dimension), e.g. ["-x"] for 1D,
// or ["sigma*(y-x)", "x*(rho-z)-y", "x*y-beta*z"] for Lorenz.
std::string solve_ode(const std::vector<std::string>& expressions,
                       const Context& params,
                       const std::vector<double>& x0,
                       double t0, double t1, double dt,
                       const std::string& method) {
    ODEFunc f = make_ode_func(expressions, params);
    State x0_state(x0.begin(), x0.end());

    Trajectory traj;
    if (method == "euler") {
        EulerIntegrator integ;
        traj = solve(f, integ, x0_state, t0, t1, dt);
    } else {
        RK4Integrator integ;
        traj = solve(f, integ, x0_state, t0, t1, dt);
    }
    return to_json(traj);
}

// --- xdot vs x (phase line) ---
std::string phase_line(const std::string& expression,
                        const Context& params,
                        double x_min, double x_max, int n_samples) {
    ScalarField1D f = make_scalar_field_1d(expression, params);
    PhaseLineResult result = compute_phase_line(f, x_min, x_max, n_samples);
    return to_json(result);
}

// Click-to-drop-a-particle on a phase line: solves the same ScalarField1D
// (adapted via to_ode_func) starting from the clicked x, so the frontend
// can animate where it flows.
std::string phase_line_particle(const std::string& expression,
                                 const Context& params,
                                 double click_x, double t_max, double dt) {
    ScalarField1D f = make_scalar_field_1d(expression, params);
    ODEFunc as_ode = to_ode_func(f);
    RK4Integrator rk4;
    Trajectory traj = solve(as_ode, rk4, State{click_x}, 0.0, t_max, dt);
    return to_json(traj);
}

// --- t-x slope field (non-autonomous 1D) ---
std::string slope_field(const std::string& expression,
                         const Context& params,
                         double t_min, double t_max, int n_t,
                         double x_min, double x_max, int n_x) {
    ODEFunc f = make_ode_func({expression}, params);
    DirectionField field = compute_slope_field_1d(f, t_min, t_max, n_t, x_min, x_max, n_x);
    return to_json(field);
}

// Click-to-place / drag isocline on a slope field. Recomputes the (cheap)
// grid each call — see the note in the chat about why stateless is fine
// here — then derives the exact isocline through the clicked point.
std::string slope_field_isocline(const std::string& expression,
                                  const Context& params,
                                  double t_min, double t_max, int n_t,
                                  double x_min, double x_max, int n_x,
                                  double click_t, double click_x) {
    ODEFunc f = make_ode_func({expression}, params);
    DirectionField field = compute_slope_field_1d(f, t_min, t_max, n_t, x_min, x_max, n_x);
    std::vector<IsoclinePoint> points = isocline_through_point(
        f, field, click_t, click_x, /*axis1_is_time=*/true, /*vector_component=*/0);
    return to_json(points);
}

// --- 2D phase-plane direction field (Van der Pol, predator-prey, ...) ---
std::string direction_field_2d(const std::vector<std::string>& expressions,
                                const Context& params,
                                double x_min, double x_max, int n_x,
                                double y_min, double y_max, int n_y) {
    ODEFunc f = make_ode_func(expressions, params);
    DirectionField field = compute_direction_field_2d(f, x_min, x_max, n_x, y_min, y_max, n_y);
    return to_json(field);
}

// Click-to-place / drag isocline (nullcline) on a 2D field.
// vector_component: 0 for the x-nullcline (dx/dt), 1 for the y-nullcline (dy/dt).
std::string direction_field_2d_isocline(const std::vector<std::string>& expressions,
                                         const Context& params,
                                         double x_min, double x_max, int n_x,
                                         double y_min, double y_max, int n_y,
                                         int vector_component,
                                         double click_axis1, double click_axis2) {
    ODEFunc f = make_ode_func(expressions, params);
    DirectionField field = compute_direction_field_2d(f, x_min, x_max, n_x, y_min, y_max, n_y);
    std::vector<IsoclinePoint> points = isocline_through_point(
        f, field, click_axis1, click_axis2, /*axis1_is_time=*/false, vector_component);
    return to_json(points);
}

// Click-to-drop-a-trajectory on a 2D direction field (Van der Pol limit
// cycle, predator-prey orbit, etc.) — just solve_ode with 2 expressions.
std::string direction_field_2d_trajectory(const std::vector<std::string>& expressions,
                                           const Context& params,
                                           double click_x, double click_y,
                                           double t_max, double dt) {
    ODEFunc f = make_ode_func(expressions, params);
    RK4Integrator rk4;
    Trajectory traj = solve(f, rk4, State{click_x, click_y}, 0.0, t_max, dt);
    return to_json(traj);
}

// --- 3D slice (Lorenz-style) ---
// A 2D direction-field grid through an N-dim system with the remaining
// components held fixed via fixed_state, e.g. an x-y slice of Lorenz at a
// fixed z.
std::string direction_field_slice(const std::vector<std::string>& expressions,
                                   const Context& params,
                                   double axis1_min, double axis1_max, int n1,
                                   double axis2_min, double axis2_max, int n2,
                                   const std::vector<double>& fixed_state) {
    ODEFunc f = make_ode_func(expressions, params);
    State fixed(fixed_state.begin(), fixed_state.end());
    DirectionField field = compute_direction_field(
        f, axis1_min, axis1_max, n1, axis2_min, axis2_max, n2,
        /*axis1_is_time=*/false, fixed);
    return to_json(field);
}

} // namespace

PYBIND11_MODULE(odelab_py, m) {
    m.doc() = "Python bindings for the odelab C++ ODE toolkit. "
              "Every function returns a JSON string ready to hand back "
              "from a web API.";

    m.def("solve_ode", &solve_ode,
          py::arg("expressions"), py::arg("params"), py::arg("x0"),
          py::arg("t0"), py::arg("t1"), py::arg("dt"),
          py::arg("method") = "rk4",
          "Solve dx/dt = f(t,x) from user expressions (one per dimension). "
          "Returns Trajectory JSON: {t, x, diverged, diverged_at_t, divergence_reason}.");

    m.def("phase_line", &phase_line,
          py::arg("expression"), py::arg("params"),
          py::arg("x_min"), py::arg("x_max"), py::arg("n_samples") = 500,
          "1D autonomous phase line (xdot vs x) with fixed points. Returns JSON.");

    m.def("phase_line_particle", &phase_line_particle,
          py::arg("expression"), py::arg("params"),
          py::arg("click_x"), py::arg("t_max") = 20.0, py::arg("dt") = 0.01,
          "Click-to-drop-a-particle on a phase line: solves forward from "
          "click_x and returns the Trajectory JSON of where it flows.");

    m.def("slope_field", &slope_field,
          py::arg("expression"), py::arg("params"),
          py::arg("t_min"), py::arg("t_max"), py::arg("n_t"),
          py::arg("x_min"), py::arg("x_max"), py::arg("n_x"),
          "t-x slope field for a (possibly non-autonomous) 1D ODE. Returns JSON.");

    m.def("slope_field_isocline", &slope_field_isocline,
          py::arg("expression"), py::arg("params"),
          py::arg("t_min"), py::arg("t_max"), py::arg("n_t"),
          py::arg("x_min"), py::arg("x_max"), py::arg("n_x"),
          py::arg("click_t"), py::arg("click_x"),
          "Isocline through a clicked (t,x) point on a slope field. Returns JSON point list.");

    m.def("direction_field_2d", &direction_field_2d,
          py::arg("expressions"), py::arg("params"),
          py::arg("x_min"), py::arg("x_max"), py::arg("n_x"),
          py::arg("y_min"), py::arg("y_max"), py::arg("n_y"),
          "2D phase-plane direction field for an autonomous system. Returns JSON.");

    m.def("direction_field_2d_isocline", &direction_field_2d_isocline,
          py::arg("expressions"), py::arg("params"),
          py::arg("x_min"), py::arg("x_max"), py::arg("n_x"),
          py::arg("y_min"), py::arg("y_max"), py::arg("n_y"),
          py::arg("vector_component"),
          py::arg("click_axis1"), py::arg("click_axis2"),
          "Nullcline (component=0 -> x-nullcline, component=1 -> y-nullcline) "
          "through a clicked (x,y) point. Returns JSON point list.");

    m.def("direction_field_2d_trajectory", &direction_field_2d_trajectory,
          py::arg("expressions"), py::arg("params"),
          py::arg("click_x"), py::arg("click_y"),
          py::arg("t_max") = 20.0, py::arg("dt") = 0.01,
          "Click-to-drop-a-trajectory on a 2D direction field. Returns Trajectory JSON.");

    m.def("direction_field_slice", &direction_field_slice,
          py::arg("expressions"), py::arg("params"),
          py::arg("axis1_min"), py::arg("axis1_max"), py::arg("n1"),
          py::arg("axis2_min"), py::arg("axis2_max"), py::arg("n2"),
          py::arg("fixed_state"),
          "2D grid slice through an N-dim system (e.g. x-y slice of Lorenz "
          "at a fixed z, passed via fixed_state). Returns JSON.");
}
