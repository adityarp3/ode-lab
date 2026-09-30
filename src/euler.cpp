#include "odelab/euler.hpp"

namespace odelab {

State EulerIntegrator::step(const ODEFunc& f, double t, const State& x, double dt) const {
    State k1 = f(t, x);
    return x + dt * k1;
}

} // namespace odelab
