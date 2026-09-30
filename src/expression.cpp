#include "odelab/expression.hpp"
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <functional>

namespace odelab {

namespace {

// Not guaranteed to exist in <cmath> under strict -std=c++17 on all
// toolchains (M_PI/M_E are POSIX/MSVC extensions, not standard C++).
constexpr double kPi = 3.14159265358979323846;
constexpr double kE = 2.71828182845904523536;

// ---------- Tokenizer ----------

enum class TokType { Number, Ident, Plus, Minus, Star, Slash, Caret,
                      LParen, RParen, Comma, End };

struct Token {
    TokType type;
    std::string text;   // for Ident
    double value = 0.0;  // for Number
};

class Tokenizer {
public:
    explicit Tokenizer(const std::string& src) : src_(src), pos_(0) {}

    Token next() {
        skip_whitespace();
        if (pos_ >= src_.size()) return {TokType::End, "", 0.0};

        char c = src_[pos_];

        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            size_t start = pos_;
            bool seen_dot = false;
            while (pos_ < src_.size() &&
                   (std::isdigit(static_cast<unsigned char>(src_[pos_])) ||
                    (src_[pos_] == '.' && !seen_dot))) {
                if (src_[pos_] == '.') seen_dot = true;
                ++pos_;
            }
            // Optional exponent, e.g. 1e-3
            if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
                size_t save = pos_;
                size_t p = pos_ + 1;
                if (p < src_.size() && (src_[p] == '+' || src_[p] == '-')) ++p;
                if (p < src_.size() && std::isdigit(static_cast<unsigned char>(src_[p]))) {
                    pos_ = p;
                    while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
                } else {
                    pos_ = save;
                }
            }
            std::string numtext = src_.substr(start, pos_ - start);
            return {TokType::Number, numtext, std::stod(numtext)};
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            size_t start = pos_;
            while (pos_ < src_.size() &&
                   (std::isalnum(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '_')) {
                ++pos_;
            }
            return {TokType::Ident, src_.substr(start, pos_ - start), 0.0};
        }

        ++pos_;
        switch (c) {
            case '+': return {TokType::Plus, "+", 0.0};
            case '-': return {TokType::Minus, "-", 0.0};
            case '*': return {TokType::Star, "*", 0.0};
            case '/': return {TokType::Slash, "/", 0.0};
            case '^': return {TokType::Caret, "^", 0.0};
            case '(': return {TokType::LParen, "(", 0.0};
            case ')': return {TokType::RParen, ")", 0.0};
            case ',': return {TokType::Comma, ",", 0.0};
            default:
                throw std::runtime_error(std::string("Unexpected character '") + c + "' in expression");
        }
    }

private:
    void skip_whitespace() {
        while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) ++pos_;
    }

    const std::string& src_;
    size_t pos_;
};

// ---------- AST nodes ----------

class NumberNode : public ExprNode {
public:
    explicit NumberNode(double v) : value_(v) {}
    double eval(const Context&) const override { return value_; }
private:
    double value_;
};

class VariableNode : public ExprNode {
public:
    explicit VariableNode(std::string name) : name_(std::move(name)) {}
    double eval(const Context& ctx) const override {
        auto it = ctx.find(name_);
        if (it == ctx.end()) {
            throw std::runtime_error("Unknown variable or parameter: '" + name_ + "'");
        }
        return it->second;
    }
private:
    std::string name_;
};

class BinaryOpNode : public ExprNode {
public:
    BinaryOpNode(char op, ExprPtr lhs, ExprPtr rhs)
        : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    double eval(const Context& ctx) const override {
        double a = lhs_->eval(ctx);
        double b = rhs_->eval(ctx);
        switch (op_) {
            case '+': return a + b;
            case '-': return a - b;
            case '*': return a * b;
            case '/':
                if (b == 0.0) {
                    throw std::runtime_error("Division by zero in expression");
                }
                return a / b;
            case '^': return std::pow(a, b);
            default: throw std::runtime_error("Internal error: unknown binary operator");
        }
    }
private:
    char op_;
    ExprPtr lhs_, rhs_;
};

class UnaryOpNode : public ExprNode {
public:
    UnaryOpNode(char op, ExprPtr operand) : op_(op), operand_(std::move(operand)) {}
    double eval(const Context& ctx) const override {
        double v = operand_->eval(ctx);
        return op_ == '-' ? -v : v;
    }
private:
    char op_;
    ExprPtr operand_;
};

class FunctionCallNode : public ExprNode {
public:
    FunctionCallNode(std::string name, std::vector<ExprPtr> args)
        : name_(std::move(name)), args_(std::move(args)) {}

    double eval(const Context& ctx) const override {
        auto arg = [&](size_t i) { return args_.at(i)->eval(ctx); };

        if (name_ == "sin")  { check_arity(1); return std::sin(arg(0)); }
        if (name_ == "cos")  { check_arity(1); return std::cos(arg(0)); }
        if (name_ == "tan")  { check_arity(1); return std::tan(arg(0)); }
        if (name_ == "exp")  { check_arity(1); return std::exp(arg(0)); }
        if (name_ == "log")  {
            check_arity(1);
            double v = arg(0);
            if (v <= 0.0) {
                throw std::runtime_error("log of non-positive number: log(" + std::to_string(v) + ")");
            }
            return std::log(v);
        }
        if (name_ == "sqrt") {
            check_arity(1);
            double v = arg(0);
            if (v < 0.0) {
                throw std::runtime_error("sqrt of negative number: sqrt(" + std::to_string(v) + ")");
            }
            return std::sqrt(v);
        }
        if (name_ == "abs")  { check_arity(1); return std::abs(arg(0)); }
        if (name_ == "tanh") { check_arity(1); return std::tanh(arg(0)); }
        if (name_ == "pow")  { check_arity(2); return std::pow(arg(0), arg(1)); }

        throw std::runtime_error("Unknown function: '" + name_ + "'");
    }
private:
    void check_arity(size_t n) const {
        if (args_.size() != n) {
            throw std::runtime_error("Function '" + name_ + "' expects " +
                                      std::to_string(n) + " argument(s), got " +
                                      std::to_string(args_.size()));
        }
    }
    std::string name_;
    std::vector<ExprPtr> args_;
};

// ---------- Recursive descent parser ----------
//
// expr   := term (('+'|'-') term)*
// term   := factor (('*'|'/') factor)*
// factor := ('-'|'+')? power
// power  := primary ('^' factor)?          [right-associative]
// primary:= number | ident | ident '(' expr (',' expr)* ')' | '(' expr ')'

class Parser {
public:
    explicit Parser(const std::string& src) : tok_(src) {
        advance();
    }

    ExprPtr parse() {
        ExprPtr result = parse_expr();
        if (cur_.type != TokType::End) {
            throw std::runtime_error("Unexpected trailing input near '" + cur_.text + "'");
        }
        return result;
    }

private:
    void advance() { cur_ = tok_.next(); }

    ExprPtr parse_expr() {
        ExprPtr node = parse_term();
        while (cur_.type == TokType::Plus || cur_.type == TokType::Minus) {
            char op = cur_.type == TokType::Plus ? '+' : '-';
            advance();
            node = std::make_shared<BinaryOpNode>(op, node, parse_term());
        }
        return node;
    }

    ExprPtr parse_term() {
        ExprPtr node = parse_factor();
        while (cur_.type == TokType::Star || cur_.type == TokType::Slash) {
            char op = cur_.type == TokType::Star ? '*' : '/';
            advance();
            node = std::make_shared<BinaryOpNode>(op, node, parse_factor());
        }
        return node;
    }

    ExprPtr parse_factor() {
        if (cur_.type == TokType::Minus) {
            advance();
            return std::make_shared<UnaryOpNode>('-', parse_power());
        }
        if (cur_.type == TokType::Plus) {
            advance();
            return parse_power();
        }
        return parse_power();
    }

    ExprPtr parse_power() {
        ExprPtr base = parse_primary();
        if (cur_.type == TokType::Caret) {
            advance();
            ExprPtr exponent = parse_factor(); // right-assoc, allows x^-2
            return std::make_shared<BinaryOpNode>('^', base, exponent);
        }
        return base;
    }

    ExprPtr parse_primary() {
        if (cur_.type == TokType::Number) {
            double v = cur_.value;
            advance();
            return std::make_shared<NumberNode>(v);
        }
        if (cur_.type == TokType::Ident) {
            std::string name = cur_.text;
            advance();
            if (cur_.type == TokType::LParen) {
                advance();
                std::vector<ExprPtr> args;
                if (cur_.type != TokType::RParen) {
                    args.push_back(parse_expr());
                    while (cur_.type == TokType::Comma) {
                        advance();
                        args.push_back(parse_expr());
                    }
                }
                expect(TokType::RParen, ")");
                return std::make_shared<FunctionCallNode>(name, std::move(args));
            }
            if (name == "pi") return std::make_shared<NumberNode>(kPi);
            if (name == "e") return std::make_shared<NumberNode>(kE);
            return std::make_shared<VariableNode>(name);
        }
        if (cur_.type == TokType::LParen) {
            advance();
            ExprPtr inner = parse_expr();
            expect(TokType::RParen, ")");
            return inner;
        }
        throw std::runtime_error("Unexpected token '" + cur_.text + "' in expression");
    }

    void expect(TokType t, const std::string& what) {
        if (cur_.type != t) {
            throw std::runtime_error("Expected '" + what + "' in expression, got '" + cur_.text + "'");
        }
        advance();
    }

    Tokenizer tok_;
    Token cur_;
};

} // namespace

ExprPtr parse_expression(const std::string& text) {
    Parser p(text);
    return p.parse();
}

ScalarField1D make_scalar_field_1d(const std::string& expr_text, const Context& params) {
    ExprPtr expr = parse_expression(expr_text);
    return [expr, params](double x) -> double {
        Context ctx = params;
        ctx["x"] = x;
        double result = expr->eval(ctx);
        if (!std::isfinite(result)) {
            throw std::runtime_error(
                "Expression evaluated to a non-finite value at x=" + std::to_string(x) +
                " (check for division by zero, sqrt of a negative number, log of a "
                "non-positive number, or a fractional power of a negative number)");
        }
        return result;
    };
}

ODEFunc make_ode_func(const std::vector<std::string>& expr_texts, const Context& params) {
    std::vector<ExprPtr> exprs;
    exprs.reserve(expr_texts.size());
    for (const auto& text : expr_texts) exprs.push_back(parse_expression(text));

    return [exprs, params](double t, const State& x) -> State {
        Context ctx = params;
        ctx["t"] = t;
        for (size_t i = 0; i < x.size(); ++i) {
            ctx["x" + std::to_string(i)] = x[i];
        }
        // Natural aliases for low-dimensional systems.
        if (x.size() >= 1) ctx["x"] = x[0];
        if (x.size() >= 2) ctx["y"] = x[1];
        if (x.size() >= 3) ctx["z"] = x[2];

        State result(exprs.size());
        for (size_t i = 0; i < exprs.size(); ++i) {
            result[i] = exprs[i]->eval(ctx);
            if (!std::isfinite(result[i])) {
                throw std::runtime_error(
                    "Expression evaluated to a non-finite value at t=" + std::to_string(t) +
                    " (check for division by zero, sqrt of a negative number, log of a "
                    "non-positive number, or a fractional power of a negative number)");
            }
        }
        return result;
    };
}

} // namespace odelab
