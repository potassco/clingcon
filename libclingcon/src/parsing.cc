// {{{ MIT License
//
// Copyright 2020 Roland Kaminski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
// }}}

#include "clingcon/parsing.hh"
#include "clingcon/base.hh"
#include "clingcon/util.hh"
#include "clingcon/astutil.hh"

#include <unordered_map>
#include <cmath>
#include <numeric>

namespace Clingcon {

namespace {

template <typename T=void>
T throw_syntax_error(char const *message="Invalid Syntax") {
    throw std::runtime_error(message);
}

void check_syntax(bool condition, char const *message="Invalid Syntax") {
    if (!condition) {
        throw_syntax_error(message);
    }
}

char const *negate_relation(char const *op) {
    if (std::strcmp(op, "=") == 0) {
        return "!=";
    }
    if (std::strcmp(op, "!=") == 0) {
        return "=";
    }
    if (std::strcmp(op, "<") == 0) {
        return ">=";
    }
    if (std::strcmp(op, "<=") == 0) {
        return ">";
    }
    if (std::strcmp(op, ">") == 0) {
        return "<=";
    }
    if (std::strcmp(op, ">=") == 0) {
        return "<";
    }
    throw std::runtime_error("unexpected operator");
}

// Checks if the given theory atom is shiftable.
struct SigMatcher {
    template <typename... CStrs>
    [[nodiscard]] static bool visit(Clingo::Symbol const &f, CStrs... strs) {
        return (f.match(strs, 0) || ...);
    }

    template <typename... CStrs>
    [[nodiscard]] static bool visit(Clingo::AST::Function const &f, CStrs... strs) {
        return !f.external && f.arguments.empty() && ((std::strcmp(f.name, strs) == 0) || ...);
    }

    template <class T, typename... CStrs>
    [[nodiscard]] static bool visit(T const &x, CStrs... strs) {
        static_cast<void>(x);
        (static_cast<void>(strs), ...);
        return false;
    }
};

// Shifts constraints into rule heads.
struct TheoryShifter {
    static void visit(Clingo::AST::Rule &rule) {
        if (!rule.head.data.is<Clingo::AST::Literal>()) {
            return;
        }
        auto &head = rule.head.data.get<Clingo::AST::Literal>();
        if (!head.data.is<Clingo::AST::Boolean>() || head.data.get<Clingo::AST::Boolean>().value) {
            return;
        }
        auto it = rule.body.begin();
        auto ie = rule.body.end();
        auto jt = it;
        for (; it != ie; ++it) {
            if (it->data.is<Clingo::AST::TheoryAtom>()) {
                auto &atom = it->data.get<Clingo::AST::TheoryAtom>();
                SigMatcher matcher;
                if (atom.term.data.accept(matcher, "sum", "diff")) {
                    check_syntax(atom.guard.get() != nullptr);
                    if (it->sign != Clingo::AST::Sign::Negation) {
                        auto *guard = atom.guard.get();
                        guard->operator_name = negate_relation(guard->operator_name);
                    }
                    rule.head.location = it->location;
                    rule.head.data = std::move(atom);
                    for (++it; it != ie; ++it, ++jt) {
                        if (it != jt) {
                            std::iter_swap(it, jt);
                        }
                    }
                    break;
                }
            }
            if (it != jt) {
                std::iter_swap(it, jt);
            }
            ++jt;
        }
        rule.body.erase(jt, ie);
    }

    template <class T>
    static void visit(T &value) {
        static_cast<void>(value);
    }
};

struct TermTagger {
    static void visit(Clingo::AST::Function &term, char const *tag) {
        std::string name{"__"};
        name += term.name;
        name += tag;
        term.name = Clingo::add_string(name.c_str());
    }

    static void visit(Clingo::Symbol &term, char const *tag) {
        std::string name{"__"};
        name += term.name();
        name += tag;
        term = Clingo::Function(name.c_str(), {});
    }

    template <typename T>
    static void visit(T &value, char const *tag) {
        static_cast<void>(value);
        static_cast<void>(tag);
        throw_syntax_error();
    }
};


// Tags head and body atoms and ensures multiset semantics.
struct TheoryRewriter {
    // Add variables to tuple to ensure multiset semantics.
    static void rewrite_tuple(Clingo::AST::TheoryAtomElement &element, int number) {
        check_syntax(element.tuple.size() == 1);
        auto vars_condition = collect_variables(element.condition.begin(), element.condition.end());
        for (auto const &name : collect_variables(element.tuple.begin(), element.tuple.end())) {
            vars_condition.erase(name);
        }
        vars_condition.erase("_");
        if (number >= 0) {
            element.tuple.push_back({element.tuple.front().location, Clingo::Number(number)});
        }
        for (auto const &name : vars_condition) {
            element.tuple.push_back({element.tuple.front().location, Clingo::AST::Variable{name}});
        }
    }

    // Add variables to tuples of elements to ensure multiset semantics.
    static void rewrite_tuples(Clingo::AST::TheoryAtom &atom) {
        int number = atom.elements.size() > 1 ? 0 : -1;
        for (auto &element : atom.elements) {
            rewrite_tuple(element, number++);
        }
    }

    static char const *tag(Clingo::AST::HeadLiteral const &lit) {
        static_cast<void>(lit);
        return "_h";
    }

    static char const *tag(Clingo::AST::BodyLiteral const &lit) {
        static_cast<void>(lit);
        return "_b";
    }

    // Mark head/body literals and ensure multiset semantics for theory atoms.
    template <class Lit>
    static void visit(Lit &node, Clingo::AST::TheoryAtom &atom) {
        SigMatcher matcher;
        if (atom.term.data.accept(matcher, "sum", "diff", "distinct", "minimize", "maximize")) {
            int number = atom.elements.size() > 1 ? 0 : -1;
            for (auto &element : atom.elements) {
                rewrite_tuple(element, number++);
            }
        }

        if (atom.term.data.accept(matcher, "sum", "diff")) {
            TermTagger tagger;
            atom.term.data.accept(tagger, tag(node));
        }
    }
};

[[nodiscard]] bool match(Clingo::TheoryTerm const &term, char const *name, size_t arity) {
    return (term.type() == Clingo::TheoryTermType::Symbol &&
        std::strcmp(term.name(), name) == 0 &&
        arity == 0) ||
        (term.type() == Clingo::TheoryTermType::Function &&
        std::strcmp(term.name(), name) == 0 &&
        term.arguments().size() == arity);
}

[[nodiscard]] Clingo::Symbol evaluate(Clingo::TheoryTerm const &term);

template <class F>
[[nodiscard]] Clingo::Symbol evaluate(Clingo::TheoryTerm const &a, Clingo::TheoryTerm const &b, F f) {
    auto ea = evaluate(a);
    check_syntax(ea.type() == Clingo::SymbolType::Number);
    auto eb = evaluate(b);
    check_syntax(eb.type() == Clingo::SymbolType::Number);
    return Clingo::Number(f(ea.number(), eb.number()));
}

val_t safe_pow(val_t a, val_t b) {
    if (a == 0) {
        throw std::overflow_error("integer overflow");
    }
    auto ret = std::pow(static_cast<double>(a), b);
    if (ret > std::numeric_limits<val_t>::max()) {
        throw std::overflow_error("integer overflow");
    }
    if (ret < std::numeric_limits<val_t>::min()) {
        throw std::underflow_error("integer underflow");
    }
    return static_cast<val_t>(ret);
}

Clingo::Symbol evaluate(Clingo::TheoryTerm const &term) {
    if (term.type() == Clingo::TheoryTermType::Symbol) {
        return Clingo::Function(term.name(), {});
    }

    if (term.type() == Clingo::TheoryTermType::Number) {
        return Clingo::Number(term.number());
    }

    if (match(term, "+", 2)) {
        return evaluate(term.arguments().front(), term.arguments().back(), safe_add<val_t>);
    }
    if (match(term, "-", 2)) {
        return evaluate(term.arguments().front(), term.arguments().back(), safe_sub<val_t>);
    }
    if (match(term, "*", 2)) {
        return evaluate(term.arguments().front(), term.arguments().back(), safe_mul<val_t>);
    }
    if (match(term, "/", 2)) {
        return evaluate(term.arguments().front(), term.arguments().back(), safe_div<val_t>);
    }
    if (match(term, "\\", 2)) {
        return evaluate(term.arguments().front(), term.arguments().back(), safe_mod<val_t>);
    }
    if (match(term, "**", 2)) {
        return evaluate(term.arguments().front(), term.arguments().back(), safe_pow);
    }

    if (match(term, "-", 1)) {
        auto ea = evaluate(term.arguments().front());
        if (ea.type() == Clingo::SymbolType::Number) {
            return Clingo::Number(safe_inv(ea.number()));
        }
        if (ea.type() == Clingo::SymbolType::Function && std::strlen(ea.name()) > 0) {
            return Clingo::Function(ea.name(), ea.arguments(), !ea.is_positive());
        }
        return throw_syntax_error<Clingo::Symbol>();
    }

    check_syntax(!match(term, "..", 2));

    if (term.type() == Clingo::TheoryTermType::Tuple || term.type() == Clingo::TheoryTermType::Function) {
        std::vector<Clingo::Symbol> args;
        args.reserve(term.arguments().size());
        for (auto const &arg : term.arguments()) {
            args.emplace_back(evaluate(arg));
        }
        return Clingo::Function(term.type() == Clingo::TheoryTermType::Function ? term.name() : "", args);
    }

    return throw_syntax_error<Clingo::Symbol>();
}

void parse_constraint_elem(AbstractConstraintBuilder &builder, Clingo::TheoryTerm const &term, bool is_sum, CoVarVec &res) {
    if (!is_sum) {
        if (match(term, "-", 2)) {
            auto args = term.arguments();

            auto a = evaluate(args.front());
            if (a.type() == Clingo::SymbolType::Number) {
                res.emplace_back(a.number(), INVALID_VAR);
            }
            else {
                res.emplace_back(1, builder.add_variable(a));
            }

            auto b = evaluate(args.back());
            if (b.type() == Clingo::SymbolType::Number) {
                res.emplace_back(-b.number(), INVALID_VAR);
            }
            else {
                res.emplace_back(-1, builder.add_variable(b));
            }
        }
        else {
            throw_syntax_error("Invalid Syntax: invalid difference constraint");
        }
    }
    else if (term.type() == Clingo::TheoryTermType::Number) {
        res.emplace_back(term.number(), INVALID_VAR);
    }
    else if (match(term, "+", 2)) {
        auto args = term.arguments();
        parse_constraint_elem(builder, args.front(), true, res);
        parse_constraint_elem(builder, args.back(), true, res);
    }
    else if (match(term, "-", 2)) {
        auto args = term.arguments();
        parse_constraint_elem(builder, args.front(), true, res);
        auto pos = res.size();
        parse_constraint_elem(builder, args.back(), true, res);
        for (auto it = res.begin() + pos, ie = res.end(); it != ie; ++it) {
            it->first = safe_inv(it->first);
        }
    }
    else if (match(term, "-", 1)) {
        auto pos = res.size();
        parse_constraint_elem(builder, term.arguments().front(), true, res);
        for (auto it = res.begin() + pos, ie = res.end(); it != ie; ++it) {
            it->first = safe_inv(it->first);
        }
    }
    else if (match(term, "+", 1)) {
        parse_constraint_elem(builder, term.arguments().front(), true, res);
    }
    else if (match(term, "*", 2)) {
        auto args = term.arguments();
        CoVarVec lhs, rhs; // NOLINT
        parse_constraint_elem(builder, args.front(), true, lhs);
        parse_constraint_elem(builder, args.back(), true, rhs);
        for (auto &l : lhs) {
            for (auto &r : rhs) {
                if (!is_valid_var(l.second)) {
                    res.emplace_back(safe_mul(l.first, r.first), r.second);
                }
                else if (!is_valid_var(l.second)) {
                    res.emplace_back(safe_mul(l.first, r.first), l.second);
                }
                else {
                    throw_syntax_error("Invalid Syntax: only linear sum constraints are supported");
                }
            }
        }
    }
    else if (term.type() == Clingo::TheoryTermType::Symbol || term.type() == Clingo::TheoryTermType::Function || term.type() == Clingo::TheoryTermType::Tuple) {
        res.emplace_back(1, builder.add_variable(evaluate(term)));
    }
    else {
        throw_syntax_error("Invalid Syntax: invalid sum constraint");
    }
}


void parse_constraint_elems(AbstractConstraintBuilder &builder, Clingo::TheoryElementSpan elements, Clingo::TheoryTerm const *rhs, bool is_sum, CoVarVec &res) {
    check_syntax(is_sum || elements.size() == 1, "Invalid Syntax: invalid difference constraint");

    for (auto const &element : elements) {
        auto tuple = element.tuple();
        check_syntax(!tuple.empty() && element.condition().empty(), "Invalid Syntax: invalid sum constraint");
        parse_constraint_elem(builder, element.tuple().front(), is_sum, res);
    }

    if (rhs != nullptr) {
        if (is_sum) {
            auto pos = res.size();
            parse_constraint_elem(builder, *rhs, is_sum, res);
            for (auto it = res.begin() + pos, ie = res.end(); it != ie; ++it) {
                it->first = safe_inv(it->first);
            }
        }
        else {
            auto term = evaluate(*rhs);
            check_syntax(term.type() == Clingo::SymbolType::Number, "Invalid Syntax: invalid difference constraint");
            res.emplace_back(safe_inv(term.number()), INVALID_VAR);
        }
    }
}

[[nodiscard]] bool normalize_constraint(AbstractConstraintBuilder &builder, lit_t literal, CoVarVec const &elements, char const *op, val_t rhs, bool strict) {
    CoVarVec copy;
    CoVarVec const *elems = &elements;

    // rewrite '>', '<', and '>=' into '<='
    if (std::strcmp(op, ">") == 0) {
        op = ">=";
        rhs = safe_add(rhs, 1);
    }
    else if (std::strcmp(op, "<") == 0) {
        op = "<=";
        rhs = safe_sub(rhs, 1);
    }
    if (std::strcmp(op, ">=") == 0) {
        op = "<=";
        rhs = safe_inv(rhs);
        copy.reserve(elements.size());
        for (auto const &[co, var] : elements) {
            copy.emplace_back(safe_inv(co), var);
        }
        elems = &copy;
    }

    // hanle remainig '<=', '=', and '!='
    if (std::strcmp(op, "<=") == 0) {
        if (strict && elems->size() == 1) {
            builder.add_constraint(literal, *elems, rhs, true);
            return true;
        }
        if (!builder.is_true(-literal)) {
            builder.add_constraint(literal, *elems, rhs, false);
        }
    }
    else if (std::strcmp(op, "=") == 0) {
        lit_t a, b; // NOLINT
        if (strict) {
            if (builder.is_true(literal)) {
                a = b = TRUE_LIT;
            }
            else {
                a = builder.add_literal();
                b = builder.add_literal();
            }

            // Note: this cannot fail because constraint normalization does not propagate
            if (!builder.add_clause({-literal, a})) {
                return false;
            }
            if (!builder.add_clause({-literal, b})) {
                return false;
            }
            if (!builder.add_clause({-a, -b, literal})) {
                return false;
            }
        }
        else {
            a = b = literal;
        }

        if (!normalize_constraint(builder, a, *elems, "<=", rhs, strict)) {
            return false;
        }
        if (!normalize_constraint(builder, b, *elems, ">=", rhs, strict)) {
            return false;
        }

        if (strict) {
            return true;
        }
    }
    else if (std::strcmp(op, "!=") == 0) {
        if (strict) {
            return normalize_constraint(builder, -literal, *elems, "=", rhs, true);
        }

        auto a = builder.add_literal();
        auto b = builder.add_literal();

        if (!builder.add_clause({a, b, -literal})) {
            return false;
        }
        if (!builder.add_clause({-a, -b})) {
            return false;
        }

        if (!normalize_constraint(builder, a, *elems, "<", rhs, false)) {
            return false;
        }
        if (!normalize_constraint(builder, b, *elems, ">", rhs, false)) {
            return false;
        }
    }

    if (strict) {
        assert(std::strcmp(op, "=") != 0);

        if (std::strcmp(op, "<=") == 0) {
            op = ">";
        }
        else if (std::strcmp(op, "!=") == 0) {
            op = "=";
        }

        if (!normalize_constraint(builder, -literal, *elems, op, rhs, false)) {
            return false;
        }
    }

    return true;
}

// Adds constraints from the given theory atom to the builder.
//
// If `is_sum` is true parses a sum constraint. Otherwise, it parses a
// difference constraint as supported by clingo-dl.
//
// Contraints are represented as a triple of a literal, its elements, and an
// upper bound.
[[nodiscard]] bool parse_constraint(AbstractConstraintBuilder &builder, Clingo::TheoryAtom const &atom, bool is_sum, bool strict) {
    check_syntax(atom.has_guard());

    CoVarVec elements;
    val_t rhs{0};
    auto guard{atom.guard()};
    auto literal = builder.solver_literal(atom.literal());

    // combine coefficients
    parse_constraint_elems(builder, atom.elements(), &guard.second, is_sum, elements);
    rhs = simplify(elements, true);

    // divide by gcd
    auto d = rhs;
    for (auto const &element : elements) {
        d = std::gcd(d, element.first);
    }
    if (d > 1) {
        for (auto &element : elements) {
            element.first /= d;
        }
        rhs /= d;
    }

    return normalize_constraint(builder, literal, elements, guard.first, rhs, strict);
}

// Parses minimize and maximize directives.
void parse_objective(AbstractConstraintBuilder &builder, Clingo::TheoryAtom const &atom, int factor) {
    CoVarVec elems;
    parse_constraint_elems(builder, atom.elements(), nullptr, true, elems);
    for (auto &[co, var] : elems) {
        builder.add_minimize(safe_mul(factor, co), var);
    }
}

void parse_show_elem(AbstractConstraintBuilder &builder, Clingo::TheoryTerm const &term) {
    if (match(term, "/", 2)) {
        auto args = term.arguments();

        auto a = evaluate(args.front());
        check_syntax(a.type() == Clingo::SymbolType::Function && a.arguments().empty(), "Invalid Syntax: invalid show statement");

        auto b = evaluate(args.back());
        check_syntax(b.type() == Clingo::SymbolType::Number, "Invalid Syntax: invalid show statement");

        builder.show_signature(a.name(), b.number());
    }
    else {
        auto a = evaluate(term);
        check_syntax(a.type() != Clingo::SymbolType::Number, "Invalid Syntax: invalid show statement");

        builder.show_variable(builder.add_variable(a));
    }
}
void parse_show(AbstractConstraintBuilder &builder, Clingo::TheoryAtom const &atom) {
    builder.add_show();

    for (auto elem : atom.elements()) {
        check_syntax(elem.tuple().size() == 1 && elem.condition().empty(), "Invalid Syntax: invalid show statement");
        parse_show_elem(builder, elem.tuple().front());
    }
}

[[nodiscard]] std::pair<val_t, val_t> parse_dom_elem(Clingo::TheoryTerm const &term) {
    if (match(term, "..", 2)) {
        auto args = term.arguments();

        auto a = evaluate(args.front());
        check_syntax(a.type() == Clingo::SymbolType::Number, "Invalid Syntax: invalid dom statement");

        auto b = evaluate(args.back());
        check_syntax(b.type() == Clingo::SymbolType::Number, "Invalid Syntax: invalid dom statement");

        return {a.number(), safe_add(b.number(), 1)};
    }

    auto a = evaluate(term);
    check_syntax(a.type() == Clingo::SymbolType::Number, "Invalid Syntax: invalid dom statement");

    return {a.number(), safe_add(a.number(), 1)};
}

void parse_dom(AbstractConstraintBuilder &builder, Clingo::TheoryAtom const &atom) {
    IntervalSet<val_t> elements;
    for (auto elem : atom.elements()) {
        auto tuple = elem.tuple();
        check_syntax(tuple.size() == 1 && elem.condition().empty(), "Invalid Syntax: invalid dom statement");
        auto [l, r] = parse_dom_elem(tuple.front());
        if (l < r) {
            check_valid_value(l);
            check_valid_value(safe_sub(r, 1));
            elements.add(l, r);
        }
    }

    check_syntax(atom.has_guard(), "Invalid Syntax: invalid dom statement");
    auto var = evaluate(atom.guard().second);

    builder.add_dom(builder.solver_literal(atom.literal()), builder.add_variable(var), elements);
}

// Currently only distinct constraints in the head are supported. Supporting
// them in the body would also be possible where they should be strict.
void parse_distinct(AbstractConstraintBuilder &builder, Clingo::TheoryAtom const &atom) {
    std::vector<std::pair<CoVarVec,val_t>> elements;

    for (auto elem : atom.elements()) {
        auto tuple = elem.tuple();
        check_syntax(!tuple.empty() && elem.condition().empty(), "Invalid Syntax: invalid distinct statement");
        elements.emplace_back();
        auto &term = elements.back();
        parse_constraint_elem(builder, tuple.front(), true, term.first);
        term.second = safe_inv(simplify(term.first));
    }

    builder.add_distinct(builder.solver_literal(atom.literal()), elements);
}

} // namespace

val_t simplify(CoVarVec &vec, bool drop_zero) {
    static thread_local std::unordered_map<var_t, CoVarVec::iterator> seen;
    val_t rhs = 0;

    seen.clear();

    auto jt = vec.begin();
    for (auto it = jt, ie = vec.end(); it != ie; ++it) {
        auto &[co, var] = *it;
        if (drop_zero && co == 0) {
            continue;
        }
        if (!is_valid_var(var)) {
            rhs = safe_sub(rhs, co);
        }
        else if (auto [kt, ins] = seen.try_emplace(var, jt); !ins) {
            kt->second->first = safe_add(kt->second->first, co);
        }
        else {
            if (it != jt) {
                *jt = *it;
            }
            ++jt;
        }
    }

    if (drop_zero) {
        jt = std::remove_if(vec.begin(), jt, [](auto &co_var) { return co_var.first == 0; } );
    }

    vec.erase(jt, vec.end());

    // overflow checking (maybe put in seperate function)
    check_valid_value(rhs);
    sum_t min = rhs;
    sum_t max = rhs;
    for (auto co_var : vec) {
        check_valid_value(co_var.first);
        min = safe_add<sum_t>(min, safe_mul<sum_t>(co_var.first, co_var.first > 0 ? MIN_VAL : MAX_VAL));
        max = safe_add<sum_t>(max, safe_mul<sum_t>(co_var.first, co_var.first > 0 ? MAX_VAL : MIN_VAL));
    }

    return rhs;
}

void transform(Clingo::AST::Statement &&stm, Clingo::StatementCallback const &cb, bool shift) {
    unpool(std::move(stm), [&](Clingo::AST::Statement &&unpooled) {
        if (shift) {
            TheoryShifter shifter;
            unpooled.data.accept(shifter);
        }
        TheoryRewriter tagger;
        transform_ast(tagger, unpooled);
        cb(std::move(unpooled));
    });
}

bool parse(AbstractConstraintBuilder &builder, Clingo::TheoryAtoms theory_atoms) {
    for (auto const &atom : theory_atoms) {
        bool is_sum_b = match(atom.term(), "__sum_b", 0);
        bool is_sum_h = match(atom.term(), "__sum_h", 0);
        bool is_diff_b = match(atom.term(), "__diff_b", 0);
        bool is_diff_h = match(atom.term(), "__diff_h", 0);
        if (is_sum_b || is_diff_b || is_sum_h || is_diff_h) {
            if (!parse_constraint(builder, atom, is_sum_b || is_sum_h, is_sum_b || is_diff_b)) {
                return false;
            }
        }
        else if(match(atom.term(), "distinct", 0)) {
            parse_distinct(builder, atom);
        }
        else if(match(atom.term(), "show", 0)) {
            parse_show(builder, atom);
        }
        else if(match(atom.term(), "dom", 0)) {
            parse_dom(builder, atom);
        }
        else if(match(atom.term(), "minimize", 0)) {
            parse_objective(builder, atom, 1);
        }
        else if(match(atom.term(), "maximize", 0)) {
            parse_objective(builder, atom, -1);
        }
    }
    return true;
}

} // namespace Clingcon
