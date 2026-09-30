#pragma once
#include <string>
#include <map>
#include <memory>
#include <vector>
#include "odelab/system.hpp"
#include "odelab/phase_line.hpp"

namespace odelab {

// Named variable/parameter values available while evaluating an expression,
// e.g. {"x": 2.0, "r": 1.0, "K": 10.0}.
using Context = std::map<std::string, double>;

// A parsed expression tree. Build once via parse_expression(), then call
// eval() many times with different variable values (cheap — no re-parsing).
class ExprNode {
public:
    virtual ~ExprNode() = default;
    virtual double eval(const Context& ctx) const = 0;
};
using ExprPtr = std::shared_ptr<ExprNode>;

// Parses a math expression typed by a user, e.g. "r*x*(1 - x/K)" or
// "sin(x) + cos(t)". Supports + - * / ^, parentheses, unary minus,
// functions (sin, cos, tan, exp, log, sqrt, abs), and constants (pi, e).
// Any other identifier (x, t, r, K, ...) is treated as a variable to be
// looked up in the Context passed to eval(). Throws std::runtime_error
// with a human-readable message on a malformed expression.
ExprPtr parse_expression(const std::string& text);

// --- Convenience bridges into the existing tools ---
//
// These are what let a user's typed-in text flow directly into solve(),
// compute_phase_line(), and (later) the direction field tool without
// those tools needing to know anything about parsing.

// Builds a ScalarField1D (dx/dt = f(x)) from a user expression in "x",
// with fixed parameter values (e.g. {"r": 1.0, "K": 10.0}) baked in.
ScalarField1D make_scalar_field_1d(const std::string& expr_text,
                                    const Context& params = {});

// Builds an N-dimensional ODEFunc from one expression per state component.
// Each expression can reference "t" and the state variables. For dim <= 3,
// both "x0"/"x1"/"x2" and the aliases "x"/"y"/"z" are bound, so users can
// write natural expressions like "sigma*(y - x)" for Lorenz's dx/dt.
ODEFunc make_ode_func(const std::vector<std::string>& expr_texts,
                       const Context& params = {});

} // namespace odelab
