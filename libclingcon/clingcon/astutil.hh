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

#ifndef CLINGCON_ASTUTIL_H
#define CLINGCON_ASTUTIL_H

//! @file clingcon/astutil.hh
//! Utility functions to work with ASTs.
//!
//! @author Roland Kaminski

#include <clingo.hh>
#include <set>

namespace Clingcon {

//! Match the given term if it is a function with signature `name/arity`.
[[nodiscard]] bool match(Clingo::AST::TheoryTerm const &term, char const *name, size_t arity);

template <typename V, typename N>
void transform_ast(V&& v, N &node);
template <typename V, typename N>
void visit_ast(V&& v, N const &node);

template <typename N>
void collect_variables(std::set<char const *> &vars, N const &node);
template <typename N>
std::set<char const *> collect_variables(N const &node);

// {{{ Implementation

namespace Detail {

template <typename T, typename N, typename V, typename = void>
struct has_visit : std::false_type { };

template <typename T, typename N, typename V>
struct has_visit<T, N, V, std::void_t<decltype(std::declval<T>().visit(std::declval<N>(), std::declval<V>()))>> : std::true_type { };

template <typename T, typename N, typename V>
std::enable_if_t<has_visit<T, N, V>::value, bool>
call_visit(T& t, N&& node, V&& value) {
    t.visit(std::forward<N>(node), std::forward<V>(value));
    return false;
}

template <typename T, typename N, typename V>
std::enable_if_t<!has_visit<T, N, V>::value, bool>
call_visit(T& t, N&& node, V&& value) {
    static_cast<void>(t);
    static_cast<void>(node);
    static_cast<void>(value);
    return true;
}

template <bool C, typename T>
using make_const_t = std::conditional_t<C, std::add_const_t<T>, T>;

template <typename V, bool Const>
struct Visitor {

    template <class T>
    void accept(T &value) {
        value.data.accept(*this, value);
    }

    // terms
    void visit(make_const_t<Const, Clingo::Symbol> &value, make_const_t<Const, Clingo::AST::Term> &node) {
        if (call_visit(visitor, node, value)) {
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Variable> &value, make_const_t<Const, Clingo::AST::Term> &node) {
        if (call_visit(visitor, node, value)) {
        }
    }

    void visit(make_const_t<Const, Clingo::AST::UnaryOperation> &value, make_const_t<Const, Clingo::AST::Term> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value.argument);
        }
    }

    void visit(make_const_t<Const, Clingo::AST::BinaryOperation> &value, make_const_t<Const, Clingo::AST::Term> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value.left);
            accept(value.right);
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Interval> &value, make_const_t<Const, Clingo::AST::Term> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value.left);
            accept(value.right);
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Function> &value, make_const_t<Const, Clingo::AST::Term> &node) {
        if (call_visit(visitor, node, value)) {
            for (auto &argument : value.arguments) {
                accept(argument);
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Pool> &value, make_const_t<Const, Clingo::AST::Term> &node) {
        if (call_visit(visitor, node, value)) {
            for (auto &argument : value.arguments) {
                accept(argument);
            }
        }
    }

    // literals
    void visit(make_const_t<Const, Clingo::AST::Boolean> &value, make_const_t<Const, Clingo::AST::Literal> &node) {
        if (call_visit(visitor, node, value)) {
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Term> &value, make_const_t<Const, Clingo::AST::Literal> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value);
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Comparison> &value, make_const_t<Const, Clingo::AST::Literal> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value.left);
            accept(value.right);
        }
    }

    void visit(make_const_t<Const, Clingo::AST::CSPLiteral> &value, make_const_t<Const, Clingo::AST::Literal> &node) {
        if (call_visit(visitor, node, value)) {
            for (auto &term : value.term.terms) {
                accept(term.coefficient);
                if (auto *variable = term.variable.get()) {
                    accept(*variable);
                }
            }
            for (auto &guard : value.guards) {
                for (auto &term : guard.term.terms) {
                    accept(term.coefficient);
                    if (auto *variable = term.variable.get()) {
                        accept(*variable);
                    }
                }
            }
        }
    }

    // theory terms
    void visit(make_const_t<Const, Clingo::Symbol> &value, make_const_t<Const, Clingo::AST::TheoryTerm> &node) {
        if (call_visit(visitor, node, value)) {
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Variable> &value, make_const_t<Const, Clingo::AST::TheoryTerm> &node) {
        if (call_visit(visitor, node, value)) {
        }
    }

    void visit(make_const_t<Const, Clingo::AST::TheoryTermSequence> &value, make_const_t<Const, Clingo::AST::TheoryTerm> &node) {
        if (call_visit(visitor, node, value)) {
            for (auto &term : value.terms) {
                accept(term);
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::TheoryFunction> &value, make_const_t<Const, Clingo::AST::TheoryTerm> &node) {
        if (call_visit(visitor, node, value)) {
            for (auto &term : value.arguments) {
                accept(term);
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::TheoryUnparsedTerm> &value, make_const_t<Const, Clingo::AST::TheoryTerm> &node) {
        if (call_visit(visitor, node, value)) {
            for (auto &element : value.elements) {
                accept(element.term);
            }
        }
    }

    // head literals
    void visit(make_const_t<Const, Clingo::AST::Literal> &value, make_const_t<Const, Clingo::AST::HeadLiteral> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value);
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Disjunction> &value, make_const_t<Const, Clingo::AST::HeadLiteral> &node) {
        if (call_visit(visitor, node, value)) {
            for (auto &condlit : value.elements) {
                accept(condlit.literal);
                for (auto &literal : condlit.condition) {
                    accept(literal);
                }
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Aggregate> &value, make_const_t<Const, Clingo::AST::HeadLiteral> &node) {
        if (call_visit(visitor, node, value)) {
            if (auto *guard = value.left_guard.get()) {
                accept(guard->term);
            }
            if (auto *guard = value.right_guard.get()) {
                accept(guard->term);
            }
            for (auto &element : value.elements) {
                accept(element.literal);
                for (auto &literal : element.condition) {
                    accept(literal);
                }
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::HeadAggregate> &value, make_const_t<Const, Clingo::AST::HeadLiteral> &node) {
        if (call_visit(visitor, node, value)) {
            if (auto *guard = value.left_guard.get()) {
                accept(guard->term);
            }
            if (auto *guard = value.right_guard.get()) {
                accept(guard->term);
            }
            for (auto &element : value.elements) {
                for (auto &term : element.tuple) {
                    accept(term);
                }
                accept(element.condition.literal);
                for (auto &literal : element.condition.condition) {
                    accept(literal);
                }
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::TheoryAtom> &value, make_const_t<Const, Clingo::AST::HeadLiteral> &node) {
        if (call_visit(visitor, node, value)) {
            if (auto *guard = value.guard.get()) {
                accept(guard->term);
            }
            for (auto &element : value.elements) {
                for (auto &term : element.tuple) {
                    accept(term);
                }
                for (auto &literal : element.condition) {
                    accept(literal);
                }
            }
        }
    }

    // body literals
    void visit(make_const_t<Const, Clingo::AST::Literal> &value, make_const_t<Const, Clingo::AST::BodyLiteral> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value);
        }
    }

    void visit(make_const_t<Const, Clingo::AST::ConditionalLiteral> &value, make_const_t<Const, Clingo::AST::BodyLiteral> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value.literal);
            for (auto &literal : value.condition) {
                accept(literal);
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Aggregate> &value, make_const_t<Const, Clingo::AST::BodyLiteral> &node) {
        if (call_visit(visitor, node, value)) {
            if (auto *guard = value.left_guard.get()) {
                accept(guard->term);
            }
            if (auto *guard = value.right_guard.get()) {
                accept(guard->term);
            }
            for (auto &element : value.elements) {
                accept(element.literal);
                for (auto &literal : element.condition) {
                    accept(literal);
                }
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::BodyAggregate> &value, make_const_t<Const, Clingo::AST::BodyLiteral> &node) {
        if (call_visit(visitor, node, value)) {
            if (auto *guard = value.left_guard.get()) {
                accept(guard->term);
            }
            if (auto *guard = value.right_guard.get()) {
                accept(guard->term);
            }
            for (auto &element : value.elements) {
                for (auto &term : element.tuple) {
                    accept(term);
                }
                for (auto &literal : element.condition) {
                    accept(literal);
                }
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::TheoryAtom> &value, make_const_t<Const, Clingo::AST::BodyLiteral> &node) {
        if (call_visit(visitor, node, value)) {
            if (auto *guard = value.guard.get()) {
                accept(guard->term);
            }
            for (auto &element : value.elements) {
                for (auto &term : element.tuple) {
                    accept(term);
                }
                for (auto &literal : element.condition) {
                    accept(literal);
                }
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Disjoint> &value, make_const_t<Const, Clingo::AST::BodyLiteral> &node) {
        if (call_visit(visitor, node, value)) {
            for (auto &element : value.elements) {
                for (auto &term : element.term.terms) {
                    accept(term.coefficient);
                    if (auto *variable = term.variable.get()) {
                        accept(*variable);
                    }
                }
                for (auto &term : element.tuple) {
                    accept(term);
                }
                for (auto &literal : element.condition) {
                    accept(literal);
                }
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Rule> &value, make_const_t<Const, Clingo::AST::Statement> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value.head);
            for (auto &literal : value.body) {
                accept(literal);
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Definition> &value, make_const_t<Const, Clingo::AST::Statement> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value.value);
        }
    }

    void visit(make_const_t<Const, Clingo::AST::ShowSignature> &value, make_const_t<Const, Clingo::AST::Statement> &node) {
        if (call_visit(visitor, node, value)) {
        }
    }

    void visit(make_const_t<Const, Clingo::AST::ShowTerm> &value, make_const_t<Const, Clingo::AST::Statement> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value.term);
            for (auto &literal : value.body) {
                accept(literal);
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Minimize> &value, make_const_t<Const, Clingo::AST::Statement> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value.weight);
            accept(value.priority);
            for (auto &term : value.tuple) {
                accept(term);
            }
            for (auto &literal : value.body) {
                accept(literal);
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Script> &value, make_const_t<Const, Clingo::AST::Statement> &node) {
        if (call_visit(visitor, node, value)) {
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Program> &value, make_const_t<Const, Clingo::AST::Statement> &node) {
        if (call_visit(visitor, node, value)) {
        }
    }

    void visit(make_const_t<Const, Clingo::AST::External> &value, make_const_t<Const, Clingo::AST::Statement> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value.atom);
            for (auto &literal : value.body) {
                accept(literal);
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Edge> &value, make_const_t<Const, Clingo::AST::Statement> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value.u);
            accept(value.v);
            for (auto &literal : value.body) {
                accept(literal);
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Heuristic> &value, make_const_t<Const, Clingo::AST::Statement> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value.atom);
            accept(value.bias);
            accept(value.modifier);
            accept(value.priority);
            for (auto &literal : value.body) {
                accept(literal);
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::ProjectAtom> &value, make_const_t<Const, Clingo::AST::Statement> &node) {
        if (call_visit(visitor, node, value)) {
            accept(value.atom);
            for (auto &literal : value.body) {
                accept(literal);
            }
        }
    }

    void visit(make_const_t<Const, Clingo::AST::ProjectSignature> &value, make_const_t<Const, Clingo::AST::Statement> &node) {
        if (call_visit(visitor, node, value)) {
        }
    }

    void visit(make_const_t<Const, Clingo::AST::TheoryDefinition> &value, make_const_t<Const, Clingo::AST::Statement> &node) {
        if (call_visit(visitor, node, value)) {
        }
    }

    void visit(make_const_t<Const, Clingo::AST::Defined> &value, make_const_t<Const, Clingo::AST::Statement> &node) {
        if (call_visit(visitor, node, value)) {
        }
    }

    V &visitor;
};

struct VarCollector {
    template <class T>
    void visit(T const &node, Clingo::AST::Variable const &value) {
        static_cast<void>(node);
        vars.emplace(value.name);
    }
    std::set<char const *> &vars;
};

} // namespace Detail

template <typename V, typename N>
void transform_ast(V&& v, N &node) {
    Detail::Visitor<V, false> vv{v};
    node.data.accept(vv, node);
}
template <typename V, typename N>
void visit_ast(V&& v, N const &node) {
    Detail::Visitor<V, true> vv{v};
    node.data.accept(vv, node);
}

template <typename N>
void collect_variables(std::set<char const *> &vars, N const &node) {
    Detail::VarCollector v{vars};
    visit_ast(v, node);
}

template <typename N>
std::set<char const *> collect_variables(N const &node) {
    std::set<char const *> vars;
    collect_variables(vars, node);
    return vars;
}

/*
def unpool_list_with(f, x):
    """
    Cross product of sequence x where each element of x is unpooled with f.
    """
    return product(*(f(y) for y in x))


class _TermUnpooler(Visitor):
    """
    Helper to unpool terms.
    """
    # pylint: disable=invalid-name

    def visit_Symbol(self, x):
        """
        Symbol(location: Location, symbol: clingo.Symbol)
        """
        return [x]

    def visit_Variable(self, x):
        """
        Variable(location: Location, name: str)
        """
        return [x]

    def visit_UnaryOperation(self, x):
        """
        UnaryOperation(location: Location, operator: UnaryOperator, argument: term)
        """
        return [ast.UnaryOperation(x.location, x.operator, y) for y in self.visit(x.term)]

    def visit_BinaryOperation(self, x):
        """
        BinaryOperation(location: Location, operator: BinaryOperator, left: term, right: term)
        """
        return [ast.BinaryOperation(x.location, x.operator, l, r) for l, r in product(self.visit(x.left), self.visit(x.right))]

    def visit_Interval(self, x):
        """
        Interval(location: Location, left: term, right: term)
        """
        return [ast.Interval(x.location, l, r) for l, r in product(self.visit(x.left), self.visit(x.right))]

    def visit_Function(self, x):
        """
        Function(location: Location, name: str, arguments: term*, external: bool)
        """
        return [ast.Function(x.location, x.name, args, x.external)
                for args in unpool_list_with(self.visit, x.arguments)]

    def visit_Pool(self, x):
        """
        Pool(location: Location, arguments: term*)
        """
        return list(chain.from_iterable(self.visit(y) for y in x.arguments))


def unpool_term(x):
    """
    Remove pools from the given term returning a list of terms.

    Care has to be taken when inplace modifying terms afterward because some
    terms might be shared if there are crossproducts. An implementation that
    always return shallow copies of modified objects, like the Transformer,
    will work without problems.
    """
    return _TermUnpooler().visit(x)


class _AtomUnpooler(Visitor):
    """
    Helper to unpool (simple) atoms.
    """
    # pylint: disable=invalid-name

    def visit_Comparison(self, x):
        """
        Unpool comparisons of form `l op r`.
        """
        return [ast.Comparison(x.comparison, l, r)
                for l, r in product(unpool_term(x.left), unpool_term(x.right))]

    def visit_BooleanConstant(self, x):
        """
        Unpool #true or #false.
        """
        return [x]

    def visit_SymbolicAtom(self, x):
        """
        Unpool atoms of form p(X).
        """
        return [ast.SymbolicAtom(y) for y in unpool_term(x.term)]


def unpool_literal(x):
    """
    Unpool a (simple) literal.

    See note in unpool_term.
    """
    if x.type != ast.ASTType.Literal:
        raise ValueError("literal expected")

    return [ast.Literal(x.location, x.sign, y) for y in _AtomUnpooler().visit(x.atom)]


def unpool_theory_atom(x):
    """
    Removes all pools from the conditions of theory atoms.

    See note in unpool_term.
    """
    x.elements = list(chain.from_iterable(
        (ast.TheoryAtomElement(element.tuple, condition)
         for condition in unpool_list_with(unpool_literal, element.condition))
        for element in x.elements))

    return ast.TheoryAtom(x.location, x.term, x.elements, x.guard)
*/

// }}}

} // namespace Clingcon

#endif // CLINGCON_ASTUTIL_H
