#include "odelab/state.hpp"
#include <stdexcept>

namespace odelab {

State operator+(const State& a, const State& b) {
    if (a.size() != b.size()) throw std::invalid_argument("State size mismatch in +");
    State result(a.size());
    for (size_t i = 0; i < a.size(); ++i) result[i] = a[i] + b[i];
    return result;
}

State operator-(const State& a, const State& b) {
    if (a.size() != b.size()) throw std::invalid_argument("State size mismatch in -");
    State result(a.size());
    for (size_t i = 0; i < a.size(); ++i) result[i] = a[i] - b[i];
    return result;
}

State operator*(double scalar, const State& a) {
    State result(a.size());
    for (size_t i = 0; i < a.size(); ++i) result[i] = scalar * a[i];
    return result;
}

State operator*(const State& a, double scalar) {
    return scalar * a;
}

State& operator+=(State& a, const State& b) {
    if (a.size() != b.size()) throw std::invalid_argument("State size mismatch in +=");
    for (size_t i = 0; i < a.size(); ++i) a[i] += b[i];
    return a;
}

} // namespace odelab
