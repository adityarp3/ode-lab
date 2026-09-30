#include "odelab/rk4.hpp"

namespace odelab {

State RK4Integrator::step(const ODEFunc& f, double t, const State& x, double dt) const {
    State k1 = f(t, x);
    State k2 = f(t + dt / 2.0, x + (dt / 2.0) * k1);
    State k3 = f(t + dt / 2.0, x + (dt / 2.0) * k2);
    State k4 = f(t + dt, x + dt * k3);

    State sum = k1 + 2.0 * k2 + 2.0 * k3 + k4;
    return x + (dt / 6.0) * sum;
}

} // namespace odelab
