#pragma once
#include "odelab/integrator.hpp"

namespace odelab {

// Classic 4th-order Runge-Kutta.
class RK4Integrator : public Integrator {
public:
    State step(const ODEFunc& f, double t, const State& x, double dt) const override;
    const char* name() const override { return "RK4"; }
};

} // namespace odelab
