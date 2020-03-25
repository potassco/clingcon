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

struct MatchVisitor {
    [[nodiscard]] bool visit(Clingo::Symbol const &f) const {
        return f.match(name, arity);
    }

    [[nodiscard]] bool visit(Clingo::AST::TheoryFunction const &f) const {
        return std::strcmp(f.name, name) == 0 && f.arguments.size() == arity;
    }

    template <class T>
    [[nodiscard]] bool visit(T const &) const { // NOLINT
        return false;
    }

    char const *name;
    size_t arity;
};

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

} // namespace

bool match(Clingo::AST::TheoryTerm const &term, char const *name, size_t arity) {
    return term.data.accept(MatchVisitor{name, arity});
}

std::vector<Clingo::AST::Term> unpool(Clingo::AST::Term const &term) {
    TermUnpooler v;
    return v.accept(term);
}

std::vector<Clingo::AST::Literal> unpool(Clingo::AST::Literal const &lit) {
    LiteralUnpooler v;
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

} // namespace Clingcon
