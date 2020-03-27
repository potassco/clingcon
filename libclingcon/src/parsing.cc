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

namespace Clingcon {

namespace {

void check_syntax(bool condition=false, char const *message="Invalid Syntax") {
    if (!condition) {
        throw std::runtime_error(message);
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
        return ">=";
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
        if (rule.head.data.is<Clingo::AST::Literal>()) {
            auto &head = rule.head.data.get<Clingo::AST::Literal>();
            if (head.data.is<Clingo::AST::Boolean>() && !head.data.get<Clingo::AST::Boolean>().value) {
                auto it = rule.body.begin();
                auto jt = it;
                auto ie = rule.body.end();
                for (; it != ie; ++it) {
                    if (it->data.is<Clingo::AST::TheoryAtom>()) {
                        auto &atom = it->data.get<Clingo::AST::TheoryAtom>();
                        SigMatcher matcher;
                        if (atom.term.data.accept(matcher, "sum", "diff")) {
                            if (it->sign != Clingo::AST::Sign::Negation) {
                                auto *guard = atom.guard.get();
                                guard->operator_name = negate_relation(guard->operator_name);
                            }
                            rule.head.location = it->location;
                            rule.head.data = std::move(atom);
                            break;
                        }
                    }
                    if (it != jt) {
                        std::iter_swap(it, jt);
                    }
                    ++jt;
                }
                for (; it != ie; ++it, ++jt) {
                    if (it != jt) {
                        std::iter_swap(it, jt);
                    }
                }
                rule.body.erase(jt, ie);
            }
        }
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
        check_syntax();
    }
};


// Tags head and body atoms and ensures multiset semantics.
struct TheoryRewriter {
    // Add variables to tuple to ensure multiset semantics.
    static void rewrite_tuple(Clingo::AST::TheoryAtomElement &element, int number) {
        check_syntax(element.tuple.size() != 1);
        auto vars_condition = collect_variables(element.condition.begin(), element.condition.end());
        for (auto const &name : collect_variables(element.tuple.begin(), element.tuple.end())) {
            vars_condition.erase(name);
        }
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

/*
void parse_theory(AbstractConstraintBuilder &builder, Clingo::TheoryAtoms &theory_atoms);
def parse_theory(builder, theory_atoms):
    """
    Parse the atoms in the given theory and pass them to the builder.
    """
    for atom in theory_atoms:
        is_sum = match(atom.term, "sum", 1)
        is_diff = match(atom.term, "diff", 1)
        if is_sum or is_diff:
            body = match(atom.term.arguments[0], "body", 0)
            _parse_constraint(builder, atom, is_sum, body)
        elif match(atom.term, "distinct", 0):
            _parse_distinct(builder, atom)
        elif match(atom.term, "show", 0):
            _parse_show(builder, atom)
        elif match(atom.term, "dom", 0):
            _parse_dom(builder, atom)
        elif match(atom.term, "minimize", 0):
            _parse_objective(builder, atom, 1)
        elif match(atom.term, "maximize", 0):
            _parse_objective(builder, atom, -1)


def _parse_objective(builder, atom, factor):
    """
    Parses minimize and maximize directives.
    """
    assert factor in (1, -1)
    for co, var in _parse_constraint_elems(builder, atom.elements, None, True):
        builder.add_minimize(factor * co, var)


def _parse_show(builder, atom):
    builder.add_show()

    for elem in atom.elements:
        if len(elem.terms) == 1 and not elem.condition:
            _parse_show_elem(builder, elem.terms[0])
        else:
            raise RuntimeError("Invalid Syntax")


def _parse_show_elem(builder, term):
    if match(term, "/", 2):
        a = _evaluate_term(term.arguments[0])
        if a.type != clingo.SymbolType.Function or a.arguments:
            raise RuntimeError("Invalid Syntax")

        b = _evaluate_term(term.arguments[1])
        if b.type != clingo.SymbolType.Number:
            raise RuntimeError("Invalid Syntax")

        builder.show_signature(a.name, b.number)
    else:
        a = _evaluate_term(term)
        if a.type == clingo.SymbolType.Number:
            raise RuntimeError("Invalid Syntax")

        builder.show_variable(a)


def _parse_dom(builder, atom):
    elements = []
    for elem in atom.elements:
        if len(elem.terms) == 1 and not elem.condition:
            elements.append(_parse_dom_elem(elem.terms[0]))
        else:
            raise RuntimeError("Invalid Syntax")

    var = _evaluate_term(atom.guard[1])
    if var.type == clingo.SymbolType.Number:
        raise RuntimeError("Invalid Syntax")

    builder.add_dom(builder.cc.solver_literal(atom.literal), builder.add_variable(var), elements)


def _parse_dom_elem(term):
    if match(term, "..", 2):
        a = _evaluate_term(term.arguments[0])
        if a.type != clingo.SymbolType.Number:
            raise RuntimeError("Invalid Syntax")

        b = _evaluate_term(term.arguments[1])
        if b.type != clingo.SymbolType.Number:
            raise RuntimeError("Invalid Syntax")

        return (a.number, b.number+1)

    a = _evaluate_term(term)
    if a.type != clingo.SymbolType.Number:
        raise RuntimeError("Invalid Syntax")

    return (a.number, a.number+1)


def _parse_distinct(builder, atom):
    """
    Currently only distinct constraints in the head are supported. Supporting
    them in the body would also be possible where they should be strict.
    """

    elements = []
    for elem in atom.elements:
        if elem.terms and not elem.condition:
            elements.append(simplify(_parse_constraint_elem(builder, elem.terms[0], True), False))
        else:
            raise RuntimeError("Invalid Syntax")

    builder.add_distinct(builder.cc.solver_literal(atom.literal), elements)


def _parse_constraint(builder, atom, is_sum, strict):
    """
    Adds constraints from the given theory atom to the builder.

    If `is_sum` is true parses a sum constraint. Otherwise, it parses a
    difference constraint as supported by clingo-dl.

    Contraints are represented as a triple of a literal, its elements, and an
    upper bound.
    """

    elements = []
    rhs = 0

    # map literal
    literal = builder.cc.solver_literal(atom.literal)

    # combine coefficients
    rhs, elements = simplify(_parse_constraint_elems(builder, atom.elements, atom.guard[1], is_sum), True)

    # divide by gcd
    d = reduce(lambda a, b: gcd(a, b[0]), elements, rhs)
    if d > 1:
        elements = [(co//d, var) for co, var in elements]
        rhs //= d

    _normalize_constraint(builder, literal, elements, atom.guard[0], rhs, strict)


def _normalize_constraint(builder, literal, elements, op, rhs, strict):
    # rerwrite > and < guard
    if op == ">":
        op = ">="
        rhs = rhs + 1
    elif op == "<":
        op = "<="
        rhs -= 1

    # rewrite >= guard
    if op == ">=":
        op = "<="
        rhs = -rhs
        elements = [(-co, var) for co, var in elements]

    if op == "<=":
        if strict and len(elements) == 1:
            builder.add_constraint(literal, elements, rhs, True)
            return
        builder.add_constraint(literal, elements, rhs, False)

    elif op == "=":
        if strict:
            if builder.cc.assignment.is_true(literal):
                a = b = 1
            else:
                a = builder.cc.add_literal()
                b = builder.cc.add_literal()

            # Note: this cannot fail because constraint normalization does not propagate
            builder.cc.add_clause([-literal, a])
            builder.cc.add_clause([-literal, b])
            builder.cc.add_clause([-a, -b, literal])
        else:
            a = b = literal

        _normalize_constraint(builder, a, elements, "<=", rhs, strict)
        _normalize_constraint(builder, b, elements, ">=", rhs, strict)

        if strict:
            return

    elif op == "!=":
        if strict:
            _normalize_constraint(builder, -literal, elements, "=", rhs, True)
            return

        a = builder.cc.add_literal()
        b = builder.cc.add_literal()

        # Note: this cannot fail
        builder.cc.add_clause([a, b, -literal])
        builder.cc.add_clause([-a, -b])

        _normalize_constraint(builder, a, elements, "<", rhs, False)
        _normalize_constraint(builder, b, elements, ">", rhs, False)

    if strict:
        if op == "<=":
            op = ">"
        elif op == "!=":
            op = "="

        _normalize_constraint(builder, -literal, elements, op, rhs, False)


def _parse_constraint_elems(builder, elems, rhs, is_sum):
    if not is_sum and len(elems) != 1:
        raise RuntimeError("Invalid Syntax")

    for elem in elems:
        if elem.terms and not elem.condition:
            for co, var in _parse_constraint_elem(builder, elem.terms[0], is_sum):
                yield co, var
        else:
            raise RuntimeError("Invalid Syntax")

    if is_sum:
        if rhs is not None:
            for co, var in _parse_constraint_elem(builder, rhs, is_sum):
                yield -co, var
    else:
        term = _evaluate_term(rhs)
        if term.type == clingo.SymbolType.Number:
            yield -term.number, None
        else:
            raise RuntimeError("Invalid Syntax")


def _parse_constraint_elem(builder, term, is_sum):
    assert term is not None
    if not is_sum:
        if match(term, "-", 2):
            a = _evaluate_term(term.arguments[0])
            if a.type == clingo.SymbolType.Number:
                yield a.number, None
            else:
                yield 1, builder.add_variable(a)

            b = _evaluate_term(term.arguments[1])
            if b.type == clingo.SymbolType.Number:
                yield -b.number, None
            else:
                yield -1, builder.add_variable(b)

        else:
            raise RuntimeError("Invalid Syntax for difference constraint")

    else:
        if match(term, "+", 2):
            for co, var in _parse_constraint_elem(builder, term.arguments[0], True):
                yield co, var
            for co, var in _parse_constraint_elem(builder, term.arguments[1], True):
                yield co, var

        elif match(term, "-", 2):
            for co, var in _parse_constraint_elem(builder, term.arguments[0], True):
                yield co, var
            for co, var in _parse_constraint_elem(builder, term.arguments[1], True):
                yield -co, var

        elif match(term, "*", 2):
            lhs = list(_parse_constraint_elem(builder, term.arguments[0], True))
            for co_prime, var_prime in _parse_constraint_elem(builder, term.arguments[1], True):
                for co, var in lhs:
                    if var is None:
                        yield co*co_prime, var_prime
                    elif var_prime is None:
                        yield co*co_prime, var
                    else:
                        raise RuntimeError("Invalid Syntax, only linear constraints allowed")

        elif match(term, "-", 1):
            for co, var in _parse_constraint_elem(builder, term.arguments[0], True):
                yield -co, var

        elif match(term, "+", 1):
            for co, var in _parse_constraint_elem(builder, term.arguments[0], True):
                yield co, var

        elif term.type == clingo.TheoryTermType.Number:
            yield term.number, None

        elif term.type in (clingo.TheoryTermType.Symbol, clingo.TheoryTermType.Function, clingo.TheoryTermType.Tuple):
            yield 1, builder.add_variable(_evaluate_term(term))

        else:
            raise RuntimeError("Invalid Syntax for linear constraint")


_BOP = {"+": lambda a, b: a+b,
        "-": lambda a, b: a-b,
        "*": lambda a, b: a*b,
        "**": lambda a, b: a**b,
        "\\": lambda a, b: a%b,
        "/": lambda a, b: a//b}


def _evaluate_term(term):
    # pylint: disable=too-many-return-statements

    # tuples
    if term.type == clingo.TheoryTermType.Tuple:
        return clingo.Tuple(_evaluate_term(x) for x in term.arguments)

    # functions and arithmetic operations
    if term.type == clingo.TheoryTermType.Function:
        # binary operations
        if term.name in _BOP and len(term.arguments) == 2:
            a = _evaluate_term(term.arguments[0])
            b = _evaluate_term(term.arguments[1])

            if a.type != clingo.SymbolType.Number or b.type != clingo.SymbolType.Number:
                raise RuntimeError("Invalid Binary Operation")

            if term.name in ("/", "\\") and b.number == 0:
                raise RuntimeError("Division by Zero")

            return clingo.Number(_BOP[term.name](a.number, b.number))

        # unary operations
        if term.name == "-" and len(term.arguments) == 1:
            a = _evaluate_term(term.arguments[0])

            if a.type == clingo.SymbolType.Number:
                return clingo.Number(-a.number)

            if a.type == clingo.SymbolType.Function and a.name:
                return clingo.Function(a.name, a.arguments, not a.positive)

            raise RuntimeError("Invalid Unary Operation")

        # invalid operators
        if term.name == ".." and len(term.arguments) == 2:
            raise RuntimeError("Invalid Interval")

        # functions
        return clingo.Function(term.name, (_evaluate_term(x) for x in term.arguments))

    # constants
    if term.type == clingo.TheoryTermType.Symbol:
        return clingo.Function(term.name)

    # numbers
    if term.type == clingo.TheoryTermType.Number:
        return clingo.Number(term.number)

    raise RuntimeError("Invalid Syntax")

*/

} // namespace Clingcon
