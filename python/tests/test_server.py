"""
pytest suite for the FastAPI server layer.

Unlike test_bindings.py (which calls odelab_py directly), this exercises
the actual HTTP surface: status codes, Pydantic request validation, and
the error-translation in server.py's call_cpp() helper. Run from python/:

    pip install pytest httpx
    pytest tests/test_server.py -v

Requires the odelab_py extension to already be built (see README.md) —
this imports the real server, not a mock.
"""
import math
import sys
import pathlib

import pytest
from fastapi.testclient import TestClient

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
from server import app  # noqa: E402

client = TestClient(app)


def test_health():
    resp = client.get("/api/health")
    assert resp.status_code == 200
    assert resp.json() == {"status": "ok"}


# --- /api/solve ---

def test_solve_matches_analytic_solution():
    resp = client.post("/api/solve", json={
        "expressions": ["-x"], "params": {}, "x0": [1.0],
        "t0": 0.0, "t1": 1.0, "dt": 0.01, "method": "rk4",
    })
    assert resp.status_code == 200
    body = resp.json()
    assert body["diverged"] is False
    assert abs(body["x"][-1][0] - math.exp(-1)) < 1e-6


def test_solve_with_euler_method():
    resp = client.post("/api/solve", json={
        "expressions": ["-x"], "x0": [1.0], "t1": 1.0, "dt": 0.01, "method": "euler",
    })
    assert resp.status_code == 200
    # Euler is less accurate than RK4 but should still be in the right ballpark.
    assert abs(resp.json()["x"][-1][0] - math.exp(-1)) < 0.01


def test_solve_missing_required_field_returns_422():
    resp = client.post("/api/solve", json={"expressions": ["-x"], "x0": [1.0]})  # missing t1
    assert resp.status_code == 422


def test_solve_reports_divergence_not_a_500():
    resp = client.post("/api/solve", json={
        "expressions": ["-sqrt(x)"], "x0": [1.0], "t1": 5.0, "dt": 0.6, "method": "euler",
    })
    assert resp.status_code == 200  # divergence is data, not a server error
    body = resp.json()
    assert body["diverged"] is True
    assert "sqrt" in body["divergence_reason"]


# --- /api/phase-line ---

def test_phase_line_finds_fixed_points():
    resp = client.post("/api/phase-line", json={
        "expression": "r*x*(1-x/K)", "params": {"r": 1.0, "K": 10.0},
        "x_min": -2, "x_max": 12, "n_samples": 500,
    })
    assert resp.status_code == 200
    body = resp.json()
    stabilities = {round(fp["x"]): fp["stability"] for fp in body["fixed_points"]}
    assert stabilities.get(0) == "unstable"
    assert stabilities.get(10) == "stable"


def test_phase_line_malformed_expression_returns_400():
    resp = client.post("/api/phase-line", json={
        "expression": "2 + * 3", "x_min": -1, "x_max": 1, "n_samples": 10,
    })
    assert resp.status_code == 400
    assert "detail" in resp.json()


def test_phase_line_particle_flows_to_fixed_point():
    resp = client.post("/api/phase-line/particle", json={
        "expression": "x - x^3", "click_x": 0.6, "t_max": 20.0, "dt": 0.01,
    })
    assert resp.status_code == 200
    assert abs(resp.json()["x"][-1][0] - 1.0) < 1e-3


# --- /api/slope-field ---

def test_slope_field_shape():
    resp = client.post("/api/slope-field", json={
        "expression": "t - x", "t_min": 0, "t_max": 5, "n_t": 6,
        "x_min": -3, "x_max": 6, "n_x": 9,
    })
    assert resp.status_code == 200
    assert len(resp.json()["samples"]) == 6 * 9


def test_slope_field_isocline_through_click():
    resp = client.post("/api/slope-field/isocline", json={
        "expression": "x", "t_min": 0, "t_max": 5, "n_t": 6,
        "x_min": -2, "x_max": 2, "n_x": 9, "click_t": 2.0, "click_x": 1.0,
    })
    assert resp.status_code == 200
    points = resp.json()
    assert len(points) > 0
    assert all(abs(p["axis2"] - 1.0) < 1e-6 for p in points)


# --- /api/direction-field ---

def test_direction_field_shape():
    resp = client.post("/api/direction-field", json={
        "expressions": ["-y", "x"], "x_min": -2, "x_max": 2, "n_x": 21,
        "y_min": -2, "y_max": 2, "n_y": 21,
    })
    assert resp.status_code == 200
    assert len(resp.json()["samples"]) == 21 * 21


def test_direction_field_nullcline():
    resp = client.post("/api/direction-field/isocline", json={
        "expressions": ["-y", "x"], "x_min": -2, "x_max": 2, "n_x": 21,
        "y_min": -2, "y_max": 2, "n_y": 21,
        "vector_component": 0, "click_axis1": 1.0, "click_axis2": 0.0,
    })
    assert resp.status_code == 200
    points = resp.json()
    assert all(abs(p["axis2"]) < 1e-6 for p in points)


def test_direction_field_trajectory():
    resp = client.post("/api/direction-field/trajectory", json={
        "expressions": ["y", "1.5*(1-x^2)*y - x"],
        "click_x": 0.1, "click_y": 0.1, "t_max": 30.0, "dt": 0.01,
    })
    assert resp.status_code == 200
    assert len(resp.json()["x"]) > 100


# --- /api/direction-field/slice (Lorenz) ---

def test_direction_field_slice():
    resp = client.post("/api/direction-field/slice", json={
        "expressions": ["sigma*(y-x)", "x*(rho-z)-y", "x*y-beta*z"],
        "params": {"sigma": 10.0, "rho": 28.0, "beta": 8.0 / 3.0},
        "axis1_min": -20, "axis1_max": 20, "n1": 11,
        "axis2_min": -20, "axis2_max": 20, "n2": 11,
        "fixed_state": [25.0],
    })
    assert resp.status_code == 200
    assert len(resp.json()["samples"]) == 11 * 11


if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-v"]))
