#pragma once
#include <vector>
#include <cstddef>

namespace odelab {

// A point in R^N — works for a 1D scalar ODE (State{x}) all the way
// up to Lorenz-style 3D systems (State{x, y, z}).
using State = std::vector<double>;

State operator+(const State& a, const State& b);
State operator-(const State& a, const State& b);
State operator*(double scalar, const State& a);
State operator*(const State& a, double scalar);
State& operator+=(State& a, const State& b);

} // namespace odelab
