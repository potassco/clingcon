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

#ifndef CLINGCON_ASTUTIL_IMPL_H
#define CLINGCON_ASTUTIL_IMPL_H

#include "clingcon/astutil.hh" // for completion

namespace Clingcon {

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

template <typename It, typename V, typename F>
inline void cross_product(It begin, It end, V &accu, F f) {
    if (begin == end) {
        f(accu.begin(), accu.end());
    }
    else {
        for (auto &x : *begin++) {
            accu.emplace_back(x);
            // Note: recursion is avoidable but pools are usually small...
            cross_product(begin, end, accu, f);
            accu.pop_back();
        }
    }
}

struct TermUnpooler {
    using Ret = std::vector<Clingo::AST::Term>;

    [[nodiscard]] Ret accept(Clingo::AST::Term const &term) const {
        return term.data.accept(*this, term);
    }

    [[nodiscard]] static Ret visit(Clingo::Symbol const &value, Clingo::AST::Term const &node) {
        static_cast<void>(value);
        return {node};
    }

    [[nodiscard]] static Ret visit(Clingo::AST::Variable const &value, Clingo::AST::Term const &node) {
        static_cast<void>(value);
        return {node};
    }

    [[nodiscard]] Ret visit(Clingo::AST::UnaryOperation const &value, Clingo::AST::Term const &node) const {
        Ret ret;
        for (auto &argument : value.argument.data.accept(*this, value.argument)) {
            ret.push_back({node.location, Clingo::AST::UnaryOperation{value.unary_operator, argument}});
        }
        return ret;
    }

    [[nodiscard]] Ret visit(Clingo::AST::BinaryOperation const &value, Clingo::AST::Term const &node) const {
        Ret ret;
        for (auto &left : accept(value.left)) {
            for (auto &right : accept(value.right)) {
                ret.push_back({node.location, Clingo::AST::BinaryOperation{value.binary_operator, left, right}});
            }
        }
        return ret;
    }

    [[nodiscard]] Ret visit(Clingo::AST::Interval const &value, Clingo::AST::Term const &node) const {
        Ret ret;
        for (auto &left : accept(value.left)) {
            for (auto &right : accept(value.right)) {
                ret.push_back({node.location, Clingo::AST::Interval{left, right}});
            }
        }
        return ret;
    }

    [[nodiscard]] Ret visit(Clingo::AST::Function const &value, Clingo::AST::Term const &node) const {
        std::vector<Ret> pools;
        for (auto const &term : value.arguments) {
            pools.emplace_back(accept(term));
        }
        Ret ret;
        ::Clingcon::cross_product(pools, [&](auto it, auto ie){
            ret.push_back({node.location, Clingo::AST::Function{value.name, {it, ie}, value.external}});
        });
        return ret;
    }

    [[nodiscard]] Ret visit(Clingo::AST::Pool const &value, Clingo::AST::Term const &node) const {
        static_cast<void>(node);
        Ret ret;
        for (auto const &term : value.arguments) {
            auto pool = accept(term);
            ret.insert(ret.end(), pool.begin(), pool.end());
        }
        return ret;
    }
};

struct LiteralUnpooler {
    using Ret = std::vector<Clingo::AST::Literal>;

    [[nodiscard]] static Ret visit(Clingo::AST::Boolean const &value, Clingo::AST::Literal const &node) {
        static_cast<void>(value);
        return {node};
    }

    [[nodiscard]] static Ret visit(Clingo::AST::Term const &value, Clingo::AST::Literal const &node) {
        Ret ret;
        for (auto &term : ::Clingcon::unpool(value)) {
            ret.push_back({node.location, node.sign, term});
        }
        return ret;
    }

    [[nodiscard]] static Ret visit(Clingo::AST::Comparison const &value, Clingo::AST::Literal const &node) {
        Ret ret;
        for (auto &left : ::Clingcon::unpool(value.left)) {
            for (auto &right : ::Clingcon::unpool(value.right)) {
                ret.push_back({node.location, node.sign, Clingo::AST::Comparison{value.comparison, left, right}});
            }
        }
        return ret;
    }

    [[nodiscard]] static Ret visit(Clingo::AST::CSPLiteral const &value, Clingo::AST::Literal const &node) {
        static_cast<void>(value);
        static_cast<void>(node);
        throw std::runtime_error("not implemented!!!");
    }
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

template <typename Seq, typename F>
inline void cross_product(Seq seq, F f) {
    std::vector<std::reference_wrapper<std::remove_reference_t<decltype(*begin(*begin(seq)))>>> accu;
    Detail::cross_product(begin(seq), end(seq), accu, f);
}

std::vector<Clingo::AST::Term> unpool(Clingo::AST::Term const &term) {
    Detail::TermUnpooler v;
    return v.accept(term);
}

std::vector<Clingo::AST::Literal> unpool(Clingo::AST::Literal const &lit) {
    Detail::LiteralUnpooler v;
    return lit.data.accept(v, lit);
}

void unpool(Clingo::AST::TheoryAtom &atom) {
    std::vector<Clingo::AST::TheoryAtomElement> elements;
    std::swap(elements, atom.elements);
    for (auto const &element : elements) {
        std::vector<std::vector<Clingo::AST::Literal>> pools;
        for (auto const &lit : element.condition) {
            pools.emplace_back(unpool(lit));
        }
        cross_product(pools, [&](auto it, auto ie){
            atom.elements.push_back({element.tuple, {it, ie}});
        });
    }
}

// }}}

} // namespace Clingcon

#endif
