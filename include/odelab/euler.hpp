#pragma once
#include "odelab/integrator.hpp"

namespace odelab {

// Forward Euler: x_{n+1} = x_n + dt * f(t_n, x_n)
class EulerIntegrator : public Integrator {
public:
    State step(const ODEFunc& f, double t, const State& x, double dt) const override;
    const char* name() const override { return "Euler"; }
};

} // namespace odelab
