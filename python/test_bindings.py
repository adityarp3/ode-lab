"""
Quick sanity check for the odelab_py bindings — run this AFTER building
(python setup.py build_ext --inplace), from inside the python/ directory
so odelab_py.so / odelab_py.pyd is importable.

This is not a substitute for the C++ test suite (tests/*.cpp) — it only
confirms the Python<->C++ boundary itself (JSON round-trips, argument
passing, error propagation) works.
"""
import json
import math
import odelab_py


def check(name, condition):
    status = "OK" if condition else "FAIL"
    print(f"[{status}] {name}")
    if not condition:
        raise SystemExit(1)


# --- solve_ode: dx/dt = -x, x0=1 -> x(1) ~= e^-1 ---
result = json.loads(odelab_py.solve_ode(["-x"], {}, [1.0], 0.0, 1.0, 0.01, "rk4"))
check("solve_ode reaches analytic exp(-1)", abs(result["x"][-1][0] - math.exp(-1)) < 1e-6)
check("solve_ode not diverged", result["diverged"] is False)

# --- phase_line: logistic growth -> fixed points at 0 (unstable) and K (stable) ---
result = json.loads(odelab_py.phase_line("r*x*(1-x/K)", {"r": 1.0, "K": 10.0}, -2.0, 12.0, 800))
fps = {round(fp["x"]): fp["stability"] for fp in result["fixed_points"]}
check("phase_line finds 2 fixed points", len(result["fixed_points"]) == 2)
check("phase_line x=0 unstable", fps.get(0) == "unstable")
check("phase_line x=10 stable", fps.get(10) == "stable")

# --- phase_line_particle: bistable system, click near x=1 should flow to x=1 ---
result = json.loads(odelab_py.phase_line_particle("x - x^3", {}, 0.6, 20.0, 0.01))
check("phase_line_particle flows to x=1", abs(result["x"][-1][0] - 1.0) < 1e-3)

# --- slope_field: dx/dt = x, t-x plane ---
result = json.loads(odelab_py.slope_field("x", {}, 0.0, 5.0, 6, -2.0, 2.0, 9))
check("slope_field has nx*ny samples", len(result["samples"]) == 6 * 9)

# --- slope_field_isocline: dx/dt = x, click at (t=2, x=1) -> isocline is x=1 line ---
result = json.loads(odelab_py.slope_field_isocline("x", {}, 0.0, 5.0, 6, -2.0, 2.0, 9, 2.0, 1.0))
check("slope_field_isocline returns points", len(result) > 0)
check("slope_field_isocline points all at x=1", all(abs(p["axis2"] - 1.0) < 1e-6 for p in result))

# --- direction_field_2d + nullcline: rigid rotation dx/dt=-y, dy/dt=x ---
result = json.loads(odelab_py.direction_field_2d(["-y", "x"], {}, -2.0, 2.0, 21, -2.0, 2.0, 21))
check("direction_field_2d has nx*ny samples", len(result["samples"]) == 21 * 21)

result = json.loads(odelab_py.direction_field_2d_isocline(
    ["-y", "x"], {}, -2.0, 2.0, 21, -2.0, 2.0, 21, 0, 1.0, 0.0))
check("x-nullcline through (1,0) is the line y=0",
      all(abs(p["axis2"]) < 1e-6 for p in result))

# --- direction_field_2d_trajectory: click-to-drop on Van der Pol ---
result = json.loads(odelab_py.direction_field_2d_trajectory(
    ["y", "1.5*(1 - x^2)*y - x"], {}, 0.1, 0.1, 30.0, 0.01))
check("direction_field_2d_trajectory produces a trajectory", len(result["x"]) > 100)

# --- direction_field_slice: Lorenz x-y slice at fixed z ---
result = json.loads(odelab_py.direction_field_slice(
    ["sigma*(y-x)", "x*(rho-z)-y", "x*y-beta*z"],
    {"sigma": 10.0, "rho": 28.0, "beta": 8.0 / 3.0},
    -20.0, 20.0, 11, -20.0, 20.0, 11, [25.0]))
check("direction_field_slice has n1*n2 samples", len(result["samples"]) == 11 * 11)

# --- Hardening: malformed expression syntax raises immediately ---
# Parsing happens eagerly in make_scalar_field_1d/make_ode_func, before any
# of the internal safe-call protection kicks in, so this should propagate
# as a real Python RuntimeError, not silently return garbage.
try:
    odelab_py.phase_line("2 + * 3", {}, -1.0, 1.0, 10)
    check("malformed expression raises RuntimeError", False)
except RuntimeError:
    check("malformed expression raises RuntimeError", True)

# --- Hardening: a domain error DURING sampling is caught internally ---
# (not raised) — phase_line wraps every f(x) call, so a pole at x=0
# should show up as has_invalid_samples, not an exception.
result = json.loads(odelab_py.phase_line("1/x", {}, -0.5, 0.5, 401))
check("phase_line flags invalid samples for a pole (1/x)", result["has_invalid_samples"] is True)

# --- Hardening: solve_ode catches mid-integration domain errors too ---
result = json.loads(odelab_py.solve_ode(["-sqrt(x)"], {}, [1.0], 0.0, 5.0, 0.6, "euler"))
check("solve_ode reports diverged instead of crashing", result["diverged"] is True)
check("solve_ode divergence_reason mentions sqrt", "sqrt" in result["divergence_reason"])

# --- Hardening: evaluate_component/isocline click is deliberately NOT caught ---
# Clicking exactly where the field is undefined should raise, so the caller
# (a click handler) gets immediate, specific feedback.
try:
    odelab_py.direction_field_2d_isocline(
        ["1/x", "y"], {}, -2.0, 2.0, 21, -2.0, 2.0, 21, 0, 0.0, 1.0)
    check("clicking an undefined point raises RuntimeError", False)
except RuntimeError as e:
    check("clicking an undefined point raises RuntimeError", "Division by zero" in str(e))

print("\nAll binding sanity checks passed.")
