#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include "odelab/expression.hpp"
#include "odelab/phase_line.hpp"
#include "odelab/solver.hpp"
#include "odelab/rk4.hpp"

using namespace odelab;

namespace {

bool approx(double a, double b, double tol = 1e-9) {
    return std::abs(a - b) < tol;
}

}

int main() {
    // Basic arithmetic and precedence
    {
        ExprPtr e = parse_expression("2 + 3 * 4");
        assert(approx(e->eval({}), 14.0));
    }
    {
        ExprPtr e = parse_expression("(2 + 3) * 4");
        assert(approx(e->eval({}), 20.0));
    }
    // Right-associative power + unary minus precedence: -2^2 == -(2^2) == -4
    {
        ExprPtr e = parse_expression("-2^2");
        assert(approx(e->eval({}), -4.0));
    }
    // x^-2 (unary inside exponent)
    {
        ExprPtr e = parse_expression("2^-1");
        assert(approx(e->eval({}), 0.5));
    }
    std::cout << "arithmetic/precedence tests passed\n";

    // Variables
    {
        ExprPtr e = parse_expression("x^2 - 1");
        assert(approx(e->eval({{"x", 2.0}}), 3.0));
        assert(approx(e->eval({{"x", -1.0}}), 0.0));
    }
    std::cout << "variable tests passed\n";

    // Functions and constants
    {
        ExprPtr e = parse_expression("sin(x) + cos(x)");
        assert(approx(e->eval({{"x", 0.0}}), 1.0));
    }
    {
        ExprPtr e = parse_expression("sqrt(4) + abs(-3)");
        assert(approx(e->eval({}), 5.0));
    }
    {
        ExprPtr e = parse_expression("pi");
        assert(approx(e->eval({}), M_PI));
    }
    std::cout << "function/constant tests passed\n";

    // Parameters merged with variable
    {
        // dx/dt = r*x*(1 - x/K), r=1, K=10, x=5 -> 1*5*(1-0.5) = 2.5
        ExprPtr e = parse_expression("r*x*(1 - x/K)");
        Context ctx{{"r", 1.0}, {"K", 10.0}, {"x", 5.0}};
        assert(approx(e->eval(ctx), 2.5));
    }
    std::cout << "parameter tests passed\n";

    // Error handling: unknown variable
    {
        bool threw = false;
        try {
            ExprPtr e = parse_expression("x + y");
            e->eval({{"x", 1.0}}); // y missing
        } catch (const std::runtime_error&) {
            threw = true;
        }
        assert(threw);
    }
    // Error handling: malformed expression
    {
        bool threw = false;
        try {
            parse_expression("2 + * 3");
        } catch (const std::runtime_error&) {
            threw = true;
        }
        assert(threw);
    }
    std::cout << "error handling tests passed\n";

    // user-typed expression -> ScalarField1D -> phase line
    // Same logistic example as test_phase_line.cpp but now sourced from a
    // string the way a website form field would provide it
    {
        ScalarField1D f = make_scalar_field_1d("r*x*(1 - x/K)", {{"r", 1.0}, {"K", 10.0}});
        PhaseLineResult result = compute_phase_line(f, -2.0, 12.0, 1000);
        assert(result.fixed_points.size() == 2);

        bool found_zero_unstable = false;
        bool found_K_stable = false;
        for (const auto& fp : result.fixed_points) {
            if (approx(fp.x, 0.0, 1e-3) && fp.stability == Stability::Unstable) found_zero_unstable = true;
            if (approx(fp.x, 10.0, 1e-3) && fp.stability == Stability::Stable) found_K_stable = true;
        }
        assert(found_zero_unstable);
        assert(found_K_stable);
    }
    std::cout << "phase-line pipeline test passed (user expression -> ScalarField1D -> compute_phase_line)\n";

    // n-dim system -> ODEFunc -> solve() 
    // dx/dt = -x via a string, same check as test_core.cpp's analytic comparison
    {
        ODEFunc f = make_ode_func({"-x"});
        RK4Integrator rk4;
        Trajectory traj = solve(f, rk4, State{1.0}, 0.0, 1.0, 0.01);
        double analytic = std::exp(-1.0);
        assert(approx(traj.x.back()[0], analytic, 1e-6));
    }
    // 2D system using x/y aliases, e.g. a simple linear rotation: dx/dt=-y, dy/dt=x
    {
        ODEFunc f = make_ode_func({"-y", "x"});
        RK4Integrator rk4;
        Trajectory traj = solve(f, rk4, State{1.0, 0.0}, 0.0, 0.0, 0.01);
        assert(traj.x.back().size() == 2);
    }
    std::cout << "solver pipeline test passed (user expressions -> ODEFunc -> solve)\n";

    std::cout << "All expression tests passed.\n";
    return 0;
}