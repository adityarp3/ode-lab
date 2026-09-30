#pragma once
#include <vector>
#include "odelab/system.hpp"

namespace odelab {

// A single sampled point of a direction field: a location and the field's
// value (slope, or vector) there. For 1D fields, only the first component
// of `position`/`vector` is meaningful (e.g. position=[x], vector=[xdot],
// or position=[t,x], vector=[1,dx/dt] for a t-x slope field). For 2D phase
// planes, both components are used (position=[x,y], vector=[dx,dy]).
struct FieldSample {
    std::vector<double> position;
    std::vector<double> vector;
};

struct DirectionField {
    std::vector<FieldSample> samples;
    int nx = 0; // grid resolution along the first axis
    int ny = 0; // grid resolution along the second axis (1 for 1D fields)

    // True if f() threw or returned a non-finite value at any grid point.
    // Those samples get a vector filled with NaN (matching the dimension
    // of the last successful evaluation, or size 1 if none succeeded yet)
    // rather than crashing the whole grid computation.
    bool has_invalid_samples = false;
};

// General N-dim sampler: evaluates f at every point of an nx * ny grid over
// [axis1_min, axis1_max] x [axis2_min, axis2_max]. `fixed_state` supplies
// the values for any state components not on the grid (e.g. for a 3D system
// projected onto x-y, fixed_state holds z). `axis1_is_time` controls
// whether the first grid axis feeds into f as t (true, for t-x slope
// fields) or as a state component (false, for phase-plane direction
// fields).
DirectionField compute_direction_field(
    const ODEFunc& f,
    double axis1_min, double axis1_max, int n1,
    double axis2_min, double axis2_max, int n2,
    bool axis1_is_time,
    State fixed_state = {});

// Convenience: t-x slope field for a possibly non-autonomous 1D ODE
// dx/dt = f(t, x). This is the classic 18.03 "slope field" tool.
DirectionField compute_slope_field_1d(
    const ODEFunc& f,
    double t_min, double t_max, int n_t,
    double x_min, double x_max, int n_x);

// Convenience: 2D phase-plane direction field for dx/dt = f(x, y) (autonomous,
// t ignored). This is the Van der Pol / predator-prey / Lorenz-projection tool.
DirectionField compute_direction_field_2d(
    const ODEFunc& f,
    double x_min, double x_max, int n_x,
    double y_min, double y_max, int n_y);

// An isocline is the set of points where a component of the field's vector
// equals a fixed constant `c` — e.g. component 0 = dx/dt for a t-x slope
// field's isoclines, or component 1 = dy/dt (with c=0) for a 2D phase
// plane's y-nullcline. Rather than solving for the curve analytically, we
// extract it from an already-computed DirectionField by finding grid cells
// where that component crosses c and linearly interpolating — this reuses
// the sampled data the direction field itself needed, no duplicate
// evaluation of f.
struct IsoclinePoint {
    double axis1; // first grid axis (t for a slope field, x for a phase plane)
    double axis2; // second grid axis (x for a slope field, y for a phase plane)
};

std::vector<IsoclinePoint> extract_isocline(const DirectionField& field,
                                             int vector_component,
                                             double c);

// Evaluates the field's `vector_component` at an arbitrary (axis1, axis2)
// point that isn't necessarily on any precomputed grid — an exact point
// evaluation, not an interpolation. This is the piece an interactive
// "click to place an isocline" UI needs: the exact slope at the clicked
// pixel, not the nearest sampled grid value.
double evaluate_component(const ODEFunc& f,
                           double axis1, double axis2,
                           bool axis1_is_time,
                           int vector_component,
                           const State& fixed_state = {});

// The whole backend of a "click on the field to drop an isocline, drag to
// move it" tool in one call: derives c from the field's exact value at
// (click_axis1, click_axis2), then extracts the isocline through that
// point from the already-computed `field`. Cheap enough to call on every
// mouse-move event — it doesn't recompute the grid, only rescans the
// existing samples for the new level set. `axis1_is_time`/`fixed_state`
// must match whatever was used to build `field` (e.g. via
// compute_direction_field / compute_slope_field_1d / compute_direction_field_2d).
std::vector<IsoclinePoint> isocline_through_point(
    const ODEFunc& f, const DirectionField& field,
    double click_axis1, double click_axis2,
    bool axis1_is_time, int vector_component,
    const State& fixed_state = {});

} // namespace odelab
