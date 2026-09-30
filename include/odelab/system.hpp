#pragma once
#include <functional>
#include "odelab/state.hpp"

namespace odelab {

// Right-hand side of dx/dt = f(t, x). Works for autonomous systems too
// (just ignore t) — e.g. logistic growth, Van der Pol, Lorenz.
using ODEFunc = std::function<State(double t, const State& x)>;

} // namespace odelab
