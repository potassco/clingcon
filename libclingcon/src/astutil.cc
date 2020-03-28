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

#include "clingcon/astutil.hh"

namespace Clingcon {

namespace {

// Unpooling could still be improved. Since this is tricky, the current
// implementation simply checks if there is a pool in a statement and only
// unpools it in this case. To improve the implementation, a term in a tuple
// resulting from a cross product can be moved if all other terms are the last
// from their pool.

struct TermUnpooler {
    using Ret = std::vector<Clingo::AST::Term>;

    [[nodiscard]] static Ret accept(Clingo::AST::Term &term) {
        TermUnpooler v;
        return term.data.accept(v, term);
    }

    [[nodiscard]] static Ret visit(Clingo::Symbol &value, Clingo::AST::Term &node) {
        static_cast<void>(value);
        return {std::move(node)};
    }

    [[nodiscard]] static Ret visit(Clingo::AST::Variable &value, Clingo::AST::Term &node) {
        static_cast<void>(value);
        return {std::move(node)};
    }

    [[nodiscard]] static Ret visit(Clingo::AST::UnaryOperation &value, Clingo::AST::Term &node) {
        Ret ret;
        for (auto &argument : accept(value.argument)) {
            ret.push_back({node.location, Clingo::AST::UnaryOperation{value.unary_operator, std::move(argument)}});
        }
        return ret;
    }

    [[nodiscard]] static Ret visit(Clingo::AST::BinaryOperation &value, Clingo::AST::Term &node) {
        Ret ret;
        auto pool_left = accept(value.left);
        auto pool_right = accept(value.right);
        for (auto &left : pool_left) {
            for (auto &right : pool_right) {
                ret.push_back({node.location, Clingo::AST::BinaryOperation{value.binary_operator, left, right}});
            }
        }
        return ret;
    }

    [[nodiscard]] static Ret visit(Clingo::AST::Interval &value, Clingo::AST::Term &node) {
        Ret ret;
        auto pool_left = accept(value.left);
        auto pool_right = accept(value.right);
        for (auto &left : pool_left) {
            for (auto &right : pool_right) {
                ret.push_back({node.location, Clingo::AST::Interval{left, right}});
            }
        }
        return ret;
    }

    [[nodiscard]] static Ret visit(Clingo::AST::Function &value, Clingo::AST::Term &node) {
        std::vector<Ret> pools;
        for (auto &term : value.arguments) {
            pools.emplace_back(accept(term));
        }
        Ret ret;
        ::Clingcon::cross_product(pools, [&](auto it, auto ie){
            ret.push_back({node.location, Clingo::AST::Function{value.name, {it, ie}, value.external}});
        });
        return ret;
    }

    [[nodiscard]] static Ret visit(Clingo::AST::Pool &value, Clingo::AST::Term &node) {
        static_cast<void>(node);
        Ret ret;
        for (auto &term : value.arguments) {
            auto pool = accept(term);
            std::move(pool.begin(), pool.end(), std::back_inserter(ret));
        }
        return ret;
    }
};

struct LiteralUnpooler {
    using Ret = std::vector<Clingo::AST::Literal>;

    [[nodiscard]] static Ret visit(Clingo::AST::Boolean &value, Clingo::AST::Literal &node) {
        static_cast<void>(value);
        return {std::move(node)};
    }

    [[nodiscard]] static Ret visit(Clingo::AST::Term &value, Clingo::AST::Literal &node) {
        Ret ret;
        for (auto &term : ::Clingcon::unpool(std::move(value))) {
            ret.push_back({node.location, node.sign, std::move(term)});
        }
        return ret;
    }

    [[nodiscard]] static Ret visit(Clingo::AST::Comparison &value, Clingo::AST::Literal &node) {
        Ret ret;
        auto pool_left = ::Clingcon::unpool(std::move(value.left));
        auto pool_right = ::Clingcon::unpool(std::move(value.right));
        for (auto &left : pool_left) {
            for (auto &right : pool_right) {
                ret.push_back({node.location, node.sign, Clingo::AST::Comparison{value.comparison, left, right}});
            }
        }
        return ret;
    }

    [[nodiscard]] static Ret visit(Clingo::AST::CSPLiteral &value, Clingo::AST::Literal &node) {
        static_cast<void>(value);
        static_cast<void>(node);
        throw std::runtime_error("not implemented!!!");
    }
};

void unpool_elements(Clingo::AST::TheoryAtom &atom) {
    std::vector<Clingo::AST::TheoryAtomElement> elements;
    std::swap(elements, atom.elements);
    for (auto &element : elements) {
        std::vector<std::vector<Clingo::AST::Literal>> pools;
        for (auto &lit : element.condition) {
            pools.emplace_back(unpool(std::move(lit)));
        }
        cross_product(pools, [&](auto it, auto ie){
            atom.elements.push_back({element.tuple, {it, ie}});
        });
    }
}

struct HeadBodyUnpooler {
    [[nodiscard]] static std::vector<Clingo::AST::BodyLiteral> visit(Clingo::AST::TheoryAtom &value, Clingo::AST::BodyLiteral &lit) {
        unpool_elements(value);
        std::vector<Clingo::AST::BodyLiteral> ret;
        for (auto &term : unpool(std::move(value.term))) {
            ret.push_back({lit.location, lit.sign, Clingo::AST::TheoryAtom{std::move(term), value.elements, value.guard}});
        }
        return ret;
    }

    [[nodiscard]] static std::vector<Clingo::AST::HeadLiteral> visit(Clingo::AST::TheoryAtom &value, Clingo::AST::HeadLiteral &lit) {
        unpool_elements(value);
        std::vector<Clingo::AST::HeadLiteral> ret;
        for (auto &term : unpool(std::move(value.term))) {
            ret.push_back({lit.location, Clingo::AST::TheoryAtom{std::move(term), value.elements, value.guard}});
        }
        return ret;
    }

    template <class T, class R>
    [[nodiscard]] static std::vector<R> visit(T &value, R &lit) {
        static_cast<void>(value);
        std::vector<R> ret;
        ret.emplace_back(std::move(lit));
        return ret;
    }
};

std::vector<std::vector<Clingo::AST::BodyLiteral>> unpool_body(std::vector<Clingo::AST::BodyLiteral> &body) {
    std::vector<std::vector<Clingo::AST::BodyLiteral>> pools;
    pools.reserve(body.size());
    for (auto &lit : body) {
        pools.emplace_back(unpool(std::move(lit)));
    }
    return pools;
}

struct StatementUnpooler {
    void visit(Clingo::AST::Rule &value, Clingo::AST::Statement &stm) const {
        auto pools = unpool_body(value.body);
        for (auto &lit : unpool(std::move(value.head))) {
            cross_product(pools, [&](auto it, auto ie) {
                callback({stm.location, Clingo::AST::Rule{lit, {it, ie}}});
            });
        }
    }

    void visit(Clingo::AST::Definition &value, Clingo::AST::Statement &stm) const {
        static_cast<void>(value);
        callback(std::move(stm));
    }

    void visit(Clingo::AST::ShowSignature &value, Clingo::AST::Statement &stm) const {
        static_cast<void>(value);
        callback(std::move(stm));
    }

    void visit(Clingo::AST::ShowTerm &value, Clingo::AST::Statement &stm) const {
        cross_product(unpool_body(value.body), [&](auto it, auto ie) {
            callback({stm.location, Clingo::AST::ShowTerm{value.term, {it, ie}, value.csp}});
        });
    }

    void visit(Clingo::AST::Minimize &value, Clingo::AST::Statement &stm) const {
        cross_product(unpool_body(value.body), [&](auto it, auto ie) {
            callback({stm.location, Clingo::AST::Minimize{value.weight, value.priority, value.tuple, {it, ie}}});
        });
    }

    void visit(Clingo::AST::Script &value, Clingo::AST::Statement &stm) const {
        static_cast<void>(value);
        callback(std::move(stm));
    }

    void visit(Clingo::AST::Program &value, Clingo::AST::Statement &stm) const {
        static_cast<void>(value);
        callback(std::move(stm));
    }

    void visit(Clingo::AST::External &value, Clingo::AST::Statement &stm) const {
        cross_product(unpool_body(value.body), [&](auto it, auto ie) {
            callback({stm.location, Clingo::AST::External{value.atom, {it, ie}, value.type}});
        });
    }

    void visit(Clingo::AST::Edge &value, Clingo::AST::Statement &stm) const {
        cross_product(unpool_body(value.body), [&](auto it, auto ie) {
            callback({stm.location, Clingo::AST::Edge{value.u, value.v, {it, ie}}});
        });
    }

    void visit(Clingo::AST::Heuristic &value, Clingo::AST::Statement &stm) const {
        cross_product(unpool_body(value.body), [&](auto it, auto ie) {
            callback({stm.location, Clingo::AST::Heuristic{value.atom, {it, ie}, value.bias, value.priority, value.modifier}});
        });
    }

    void visit(Clingo::AST::ProjectAtom &value, Clingo::AST::Statement &stm) const {
        cross_product(unpool_body(value.body), [&](auto it, auto ie) {
            callback({stm.location, Clingo::AST::ProjectAtom{value.atom, {it, ie}}});
        });
    }

    void visit(Clingo::AST::ProjectSignature &value, Clingo::AST::Statement &stm) const {
        static_cast<void>(value);
        callback(std::move(stm));
    }

    void visit(Clingo::AST::TheoryDefinition &value, Clingo::AST::Statement &stm) const {
        static_cast<void>(value);
        callback(std::move(stm));
    }

    void visit(Clingo::AST::Defined &value, Clingo::AST::Statement &stm) const {
        static_cast<void>(value);
        callback(std::move(stm));
    }

    Clingo::StatementCallback const &callback;
};

struct HasPool {
    void visit(Clingo::AST::Term const &term, Clingo::AST::Pool const &pool) {
        static_cast<void>(term);
        static_cast<void>(pool);
        found_pool = true;
    }
    bool found_pool{false};
};

template <class N>
bool has_pool(N const &node) {
    HasPool v;
    visit_ast(v, node);
    return v.found_pool;
}

struct TheoryHasPool {
    void visit(Clingo::AST::BodyLiteral const &lit, Clingo::AST::TheoryAtom const &atom) {
        static_cast<void>(atom);
        found_pool = found_pool || has_pool(lit);
    }
    void visit(Clingo::AST::HeadLiteral const &lit, Clingo::AST::TheoryAtom const &atom) {
        static_cast<void>(atom);
        found_pool = found_pool || has_pool(lit);
    }
    bool found_pool{false};
};

template <class N>
bool theory_has_pool(N const &node) {
    TheoryHasPool v;
    visit_ast(v, node);
    return v.found_pool;
}

} // namespace

std::vector<Clingo::AST::Term> unpool(Clingo::AST::Term &&term) {
    if (has_pool(term)) {
        return TermUnpooler::accept(term);
    }
    return {std::move(term)};
}

std::vector<Clingo::AST::Literal> unpool(Clingo::AST::Literal &&lit) {
    if (has_pool(lit)) {
        LiteralUnpooler v;
        return lit.data.accept(v, lit);
    }
    return {std::move(lit)};
}

std::vector<Clingo::AST::HeadLiteral> unpool(Clingo::AST::HeadLiteral &&lit) {
    if (theory_has_pool(lit)) {
        HeadBodyUnpooler v;
        return lit.data.accept(v, lit);
    }
    return {std::move(lit)};
}

std::vector<Clingo::AST::BodyLiteral> unpool(Clingo::AST::BodyLiteral &&lit) {
    if (theory_has_pool(lit)) {
        HeadBodyUnpooler v;
        return lit.data.accept(v, lit);
    }
    return {std::move(lit)};
}

void unpool(Clingo::AST::Statement &&stm, Clingo::StatementCallback const &cb) {
    if (theory_has_pool(stm)) {
        StatementUnpooler v{cb};
        stm.data.accept(v, stm);
    }
    else {
        cb(std::move(stm));
    }
}

} // namespace Clingcon
