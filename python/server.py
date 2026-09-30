"""
FastAPI server wrapping the odelab_py pybind11 module.

Run:
    uvicorn server:app --reload --port 8000

Every endpoint returns the JSON string built on the C++ side (see
odelab::to_json in io.hpp) unmodified — Python doesn't re-parse/re-serialize
it, just passes it through as the response body.
"""
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import Response
from pydantic import BaseModel, Field
from typing import List, Dict, Callable, Any

import odelab_py

app = FastAPI(
    title="odelab API",
    description="Euler/RK4 solver, phase line, direction fields, and isoclines "
                "for 18.03 / nonlinear-dynamics tools, backed by C++.",
)

# Loosen this to your actual frontend origin before deploying publicly.
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


def json_response(json_str: str) -> Response:
    return Response(content=json_str, media_type="application/json")


def call_cpp(fn: Callable, *args: Any, **kwargs: Any) -> Response:
    """Runs a C++ binding and turns any exception (bad user expression,
    domain error, etc.) into a 400 with the C++ side's own error message,
    rather than a raw 500."""
    try:
        return json_response(fn(*args, **kwargs))
    except RuntimeError as e:
        raise HTTPException(status_code=400, detail=str(e))


# --- t vs x(t) ---

class SolveRequest(BaseModel):
    expressions: List[str] = Field(
        ..., description="One expression per state dimension, e.g. ['-x'] "
                          "or ['sigma*(y-x)', 'x*(rho-z)-y', 'x*y-beta*z']")
    params: Dict[str, float] = {}
    x0: List[float]
    t0: float = 0.0
    t1: float
    dt: float = 0.01
    method: str = "rk4"  # "rk4" or "euler"


@app.post("/api/solve")
def solve_endpoint(req: SolveRequest):
    return call_cpp(odelab_py.solve_ode, req.expressions, req.params,
                     req.x0, req.t0, req.t1, req.dt, req.method)


# --- xdot vs x (phase line) ---

class PhaseLineRequest(BaseModel):
    expression: str
    params: Dict[str, float] = {}
    x_min: float
    x_max: float
    n_samples: int = 500


@app.post("/api/phase-line")
def phase_line_endpoint(req: PhaseLineRequest):
    return call_cpp(odelab_py.phase_line, req.expression, req.params,
                     req.x_min, req.x_max, req.n_samples)


class PhaseLineParticleRequest(BaseModel):
    expression: str
    params: Dict[str, float] = {}
    click_x: float
    t_max: float = 20.0
    dt: float = 0.01


@app.post("/api/phase-line/particle")
def phase_line_particle_endpoint(req: PhaseLineParticleRequest):
    return call_cpp(odelab_py.phase_line_particle, req.expression, req.params,
                     req.click_x, req.t_max, req.dt)


# --- t-x slope field (non-autonomous 1D) ---

class SlopeFieldRequest(BaseModel):
    expression: str
    params: Dict[str, float] = {}
    t_min: float
    t_max: float
    n_t: int
    x_min: float
    x_max: float
    n_x: int


@app.post("/api/slope-field")
def slope_field_endpoint(req: SlopeFieldRequest):
    return call_cpp(odelab_py.slope_field, req.expression, req.params,
                     req.t_min, req.t_max, req.n_t, req.x_min, req.x_max, req.n_x)


class SlopeFieldIsoclineRequest(SlopeFieldRequest):
    click_t: float
    click_x: float


@app.post("/api/slope-field/isocline")
def slope_field_isocline_endpoint(req: SlopeFieldIsoclineRequest):
    return call_cpp(odelab_py.slope_field_isocline, req.expression, req.params,
                     req.t_min, req.t_max, req.n_t, req.x_min, req.x_max, req.n_x,
                     req.click_t, req.click_x)


# --- 2D phase-plane direction field ---

class DirectionField2DRequest(BaseModel):
    expressions: List[str] = Field(..., description="Exactly 2: [dx/dt, dy/dt]")
    params: Dict[str, float] = {}
    x_min: float
    x_max: float
    n_x: int
    y_min: float
    y_max: float
    n_y: int


@app.post("/api/direction-field")
def direction_field_endpoint(req: DirectionField2DRequest):
    return call_cpp(odelab_py.direction_field_2d, req.expressions, req.params,
                     req.x_min, req.x_max, req.n_x, req.y_min, req.y_max, req.n_y)


class DirectionFieldIsoclineRequest(DirectionField2DRequest):
    vector_component: int  # 0 = x-nullcline (dx/dt=c), 1 = y-nullcline (dy/dt=c)
    click_axis1: float
    click_axis2: float


@app.post("/api/direction-field/isocline")
def direction_field_isocline_endpoint(req: DirectionFieldIsoclineRequest):
    return call_cpp(
        odelab_py.direction_field_2d_isocline,
        req.expressions, req.params, req.x_min, req.x_max, req.n_x,
        req.y_min, req.y_max, req.n_y,
        req.vector_component, req.click_axis1, req.click_axis2,
    )


class DirectionFieldTrajectoryRequest(BaseModel):
    expressions: List[str] = Field(..., description="Exactly 2: [dx/dt, dy/dt]")
    params: Dict[str, float] = {}
    click_x: float
    click_y: float
    t_max: float = 20.0
    dt: float = 0.01


@app.post("/api/direction-field/trajectory")
def direction_field_trajectory_endpoint(req: DirectionFieldTrajectoryRequest):
    return call_cpp(
        odelab_py.direction_field_2d_trajectory,
        req.expressions, req.params, req.click_x, req.click_y, req.t_max, req.dt,
    )


# --- N-dim slice (Lorenz-style) ---

class DirectionFieldSliceRequest(BaseModel):
    expressions: List[str] = Field(..., description="N expressions, one per state dimension")
    params: Dict[str, float] = {}
    axis1_min: float
    axis1_max: float
    n1: int
    axis2_min: float
    axis2_max: float
    n2: int
    fixed_state: List[float] = Field(
        ..., description="Values for state components beyond the first two "
                          "grid axes, e.g. [z] for an x-y slice of Lorenz")


@app.post("/api/direction-field/slice")
def direction_field_slice_endpoint(req: DirectionFieldSliceRequest):
    return call_cpp(
        odelab_py.direction_field_slice,
        req.expressions, req.params, req.axis1_min, req.axis1_max, req.n1,
        req.axis2_min, req.axis2_max, req.n2, req.fixed_state,
    )


@app.get("/api/health")
def health():
    return {"status": "ok"}
