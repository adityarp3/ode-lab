#include "odelab/direction_field.hpp"
#include <cmath>
#include <limits>

namespace odelab {

namespace {

// Builds the (t, state) pair to feed into f for a given grid or click
// point — shared by compute_direction_field() and evaluate_component() so
// the axis1_is_time/fixed_state convention only lives in one place.
std::pair<double, State> build_query(double axis1, double axis2,
                                      bool axis1_is_time,
                                      const State& fixed_state) {
    double t;
    State state;
    if (axis1_is_time) {
        state.reserve(1 + fixed_state.size());
        t = axis1;
        state.push_back(axis2);
    } else {
        state.reserve(2 + fixed_state.size());
        t = 0.0;
        state.push_back(axis1);
        state.push_back(axis2);
    }
    for (double v : fixed_state) state.push_back(v);
    return {t, state};
}

// Evaluates f at a grid point, catching exceptions and non-finite results
// so one bad grid point (e.g. a user expression hitting a domain error at
// that particular x,y) doesn't abort the whole sampling pass. On failure,
// returns a vector of NaN sized to `fallback_dim` (the dimension of the
// last successful evaluation, so downstream code that expects a consistent
// vector size still gets one).
struct EvalResult { bool ok; State vector; };

EvalResult safe_eval(const ODEFunc& f, double t, const State& state, size_t fallback_dim) {
    try {
        State v = f(t, state);
        for (double c : v) {
            if (!std::isfinite(c)) {
                return {false, State(v.empty() ? fallback_dim : v.size(),
                                      std::numeric_limits<double>::quiet_NaN())};
            }
        }
        return {true, v};
    } catch (...) {
        return {false, State(fallback_dim, std::numeric_limits<double>::quiet_NaN())};
    }
}

} // namespace

DirectionField compute_direction_field(
    const ODEFunc& f,
    double axis1_min, double axis1_max, int n1,
    double axis2_min, double axis2_max, int n2,
    bool axis1_is_time,
    State fixed_state) {

    if (n1 < 1) n1 = 1;
    if (n2 < 1) n2 = 1;

    DirectionField field;
    field.nx = n1;
    field.ny = n2;
    field.samples.reserve(static_cast<size_t>(n1) * static_cast<size_t>(n2));

    double step1 = (n1 > 1) ? (axis1_max - axis1_min) / (n1 - 1) : 0.0;
    double step2 = (n2 > 1) ? (axis2_max - axis2_min) / (n2 - 1) : 0.0;

    size_t last_good_dim = 1; // best-guess fallback dimension until we see a success

    // Storage order: axis1 varies slowest, axis2 fastest (row-major with
    // n2 as row width) — extract_isocline() relies on this layout.
    for (int i = 0; i < n1; ++i) {
        double a1 = axis1_min + i * step1;
        for (int j = 0; j < n2; ++j) {
            double a2 = axis2_min + j * step2;

            auto [t, state] = build_query(a1, a2, axis1_is_time, fixed_state);
            EvalResult res = safe_eval(f, t, state, last_good_dim);
            if (res.ok) {
                last_good_dim = res.vector.size();
            } else {
                field.has_invalid_samples = true;
            }

            FieldSample sample;
            sample.position = {a1, a2};
            sample.vector = std::move(res.vector);
            field.samples.push_back(std::move(sample));
        }
    }
    return field;
}

DirectionField compute_slope_field_1d(
    const ODEFunc& f,
    double t_min, double t_max, int n_t,
    double x_min, double x_max, int n_x) {
    return compute_direction_field(f, t_min, t_max, n_t, x_min, x_max, n_x,
                                    /*axis1_is_time=*/true, /*fixed_state=*/{});
}

DirectionField compute_direction_field_2d(
    const ODEFunc& f,
    double x_min, double x_max, int n_x,
    double y_min, double y_max, int n_y) {
    return compute_direction_field(f, x_min, x_max, n_x, y_min, y_max, n_y,
                                    /*axis1_is_time=*/false, /*fixed_state=*/{});
}

std::vector<IsoclinePoint> extract_isocline(const DirectionField& field,
                                             int vector_component,
                                             double c) {
    std::vector<IsoclinePoint> points;
    int n1 = field.nx;
    int n2 = field.ny;
    if (n1 == 0 || n2 == 0) return points;

    auto idx = [&](int i, int j) { return static_cast<size_t>(i) * n2 + j; };
    auto value_at = [&](int i, int j) -> double {
        const auto& vec = field.samples[idx(i, j)].vector;
        if (vector_component < 0 || static_cast<size_t>(vector_component) >= vec.size()) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        double v = vec[static_cast<size_t>(vector_component)];
        if (!std::isfinite(v)) return std::numeric_limits<double>::quiet_NaN();
        return v - c;
    };
    auto axis1_at = [&](int i, int j) { return field.samples[idx(i, j)].position[0]; };
    auto axis2_at = [&](int i, int j) { return field.samples[idx(i, j)].position[1]; };

    // Scan along axis2 for each fixed axis1 row, linearly interpolating
    // where the target component crosses c.
    for (int i = 0; i < n1; ++i) {
        for (int j = 0; j + 1 < n2; ++j) {
            double v0 = value_at(i, j);
            double v1 = value_at(i, j + 1);
            if (std::isnan(v0) || std::isnan(v1)) continue; // can't test this edge

            if (v0 == 0.0) {
                points.push_back({axis1_at(i, j), axis2_at(i, j)});
                continue;
            }
            if ((v0 > 0) != (v1 > 0)) {
                double frac = v0 / (v0 - v1);
                double a1 = axis1_at(i, j) + frac * (axis1_at(i, j + 1) - axis1_at(i, j));
                double a2 = axis2_at(i, j) + frac * (axis2_at(i, j + 1) - axis2_at(i, j));
                points.push_back({a1, a2});
            }
        }
    }

    // Scan along axis1 for each fixed axis2 column too, so the contour is
    // resolved in both directions (matters when the level set runs roughly
    // parallel to one grid axis).
    for (int j = 0; j < n2; ++j) {
        for (int i = 0; i + 1 < n1; ++i) {
            double v0 = value_at(i, j);
            double v1 = value_at(i + 1, j);
            if (std::isnan(v0) || std::isnan(v1)) continue; // can't test this edge
            if (v0 == 0.0) continue; // already captured by the row scan above
            if ((v0 > 0) != (v1 > 0)) {
                double frac = v0 / (v0 - v1);
                double a1 = axis1_at(i, j) + frac * (axis1_at(i + 1, j) - axis1_at(i, j));
                double a2 = axis2_at(i, j) + frac * (axis2_at(i + 1, j) - axis2_at(i, j));
                points.push_back({a1, a2});
            }
        }
    }

    return points;
}

double evaluate_component(const ODEFunc& f,
                           double axis1, double axis2,
                           bool axis1_is_time,
                           int vector_component,
                           const State& fixed_state) {
    // Deliberately does NOT catch exceptions/non-finite results, unlike
    // the grid sampling above: this is meant for one-off calls (e.g. a
    // single click event), where the caller wants to know immediately
    // that the field is undefined there, not silently get back NaN.
    auto [t, state] = build_query(axis1, axis2, axis1_is_time, fixed_state);
    State vec = f(t, state);
    return vec.at(static_cast<size_t>(vector_component));
}

std::vector<IsoclinePoint> isocline_through_point(
    const ODEFunc& f, const DirectionField& field,
    double click_axis1, double click_axis2,
    bool axis1_is_time, int vector_component,
    const State& fixed_state) {
    double c = evaluate_component(f, click_axis1, click_axis2,
                                   axis1_is_time, vector_component, fixed_state);
    return extract_isocline(field, vector_component, c);
}

} // namespace odelab
