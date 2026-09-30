#pragma once
#include "odelab/system.hpp"

namespace odelab {

// Strategy interface: any fixed-step, one-step integration method
// (Euler, RK4, and later RK45/Heun/etc.) implements this.
class Integrator {
public:
    virtual ~Integrator() = default;

    // Advance state x at time t forward by step dt using rhs f.
    virtual State step(const ODEFunc& f, double t, const State& x, double dt) const = 0;

    virtual const char* name() const = 0;
};

} // namespace odelab
