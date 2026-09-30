#pragma once
#include <string>
#include "odelab/solver.hpp"
#include "odelab/phase_line.hpp"
#include "odelab/direction_field.hpp"

namespace odelab {

// Writes columns t,x0,x1,...,xN-1 — one row per time step.
// This is the plain-data handoff format for a Python/Java backend
// to read until we wire up a proper binding (pybind11 / JNI).
void write_csv(const std::string& path, const Trajectory& traj);

// Writes columns x,xdot — the sampled curve for the phase-line plot.
void write_csv(const std::string& path, const PhaseLineResult& pl);

// Writes columns x,stability — one row per fixed point found.
void write_fixed_points_csv(const std::string& path,
                             const std::vector<FixedPoint>& fixed_points);

// Writes columns axis1,axis2,v0,v1,... — one row per grid sample.
// For a slope field (compute_slope_field_1d): axis1=t, axis2=x, v0=dx/dt.
// For a phase-plane field (compute_direction_field_2d): axis1=x, axis2=y,
// v0=dx/dt, v1=dy/dt.
void write_csv(const std::string& path, const DirectionField& field);

// Writes columns axis1,axis2 — one row per point on the extracted isocline
// or nullcline curve.
void write_csv(const std::string& path, const std::vector<IsoclinePoint>& points);

// --- JSON ---
//
// Builds the exact string a web API would return, and a file writer for
// local testing. Non-finite doubles (from a diverged trajectory or an
// invalid field sample) are encoded as JSON `null` rather than the
// non-standard NaN/Infinity tokens, since most JSON parsers reject those.

std::string to_json(const Trajectory& traj);
std::string to_json(const PhaseLineResult& pl);
std::string to_json(const std::vector<FixedPoint>& fixed_points);
std::string to_json(const DirectionField& field);
std::string to_json(const std::vector<IsoclinePoint>& points);

void write_json(const std::string& path, const Trajectory& traj);
void write_json(const std::string& path, const PhaseLineResult& pl);
void write_json(const std::string& path, const std::vector<FixedPoint>& fixed_points);
void write_json(const std::string& path, const DirectionField& field);
void write_json(const std::string& path, const std::vector<IsoclinePoint>& points);

} // namespace odelab
