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

#include <algorithm>
#include <clingo/ast.hh>
#include <clingo/core.hh>
#include <cmath>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>

namespace Clingcon {

namespace {

template <typename T = void> auto throw_syntax_error(char const *message = "Invalid Syntax") -> T {
    throw std::runtime_error(message);
}

void check_syntax(bool condition, char const *message = "Invalid Syntax") {
    if (!condition) {
        throw_syntax_error(message);
    }
}

//! Negate a relation symbol.
[[maybe_unused]] auto negate_relation(std::string_view op) -> std::string_view {
    if (op == "=") {
        return "!=";
    }
    if (op == "!=") {
        return "=";
    }
    if (op == "<") {
        return ">=";
    }
    if (op == "<=") {
        return ">";
    }
    if (op == ">") {
        return "<=";
    }
    if (op == ">=") {
        return "<";
    }
    return throw_syntax_error<std::string_view>("unexpected operator");
}

using Clingcon::match;

//! Match if the given node represents a constant with the given name.
template <typename... Views> auto match_constant(Clingo::AST::Node const &ast, Views... names) -> bool {
    using namespace Clingo::AST;
    switch (ast.type()) {
        case NodeType::term_symbolic: {
            return (ast.symbol(Attribute::symbol).match(names, 0) || ...);
        }
        case NodeType::term_function: {
            check_syntax(ast.number(Attribute::external) == 0, "theory atom names must not be external");
            if (((ast.string(Attribute::name) != names) && ...)) {
                return false;
            }
            auto pool = ast.nodes(Attribute::pool);
            check_syntax(pool.size() == 1, "theory atom names must not contain pools");
            return pool.front().nodes(Attribute::arguments).empty();
        }
        default: {
            return false;
        }
    }
}

//! Shift difference constraints in integrity constraints to the head.
//!
//! Moves the first difference constraint in the body of a rule into the head of an
//! integrity constraint.
auto shift_rule(Clingo::Library const &lib, Clingo::AST::Node ast) -> Clingo::AST::Node {
    using namespace Clingo::AST;
    if (ast.type() != NodeType::statement_rule) {
        return ast;
    }
    auto head = ast.node(Attribute::head);
    if (head.type() != NodeType::head_simple_literal) {
        return ast;
    }
    auto lit = head.node(Attribute::literal);
    if (lit.type() != NodeType::literal_boolean) {
        return ast;
    }
    auto sign = lit.number(Attribute::sign);
    auto value = lit.number(Attribute::value);
    if ((value == 0 && sign == Sign::single) || (value == 1 && sign != Sign::single)) {
        return ast;
    }

    auto body = ast.nodes(Attribute::body);
    for (auto it = body.begin(), ie = body.end(); it != ie; ++it) {
        auto lit = *it;
        if (lit.type() != NodeType::body_theory_atom) {
            continue;
        }
        auto name = lit.node(Attribute::name);
        if (!match_constant(name, "diff", "sum")) {
            continue;
        }
        auto elems = lit.nodes(Attribute::elements);
        auto guard = lit.optional_node(Attribute::right);
        check_syntax(guard.has_value());
        if (lit.number(Attribute::sign) != Sign::single) {
            guard = guard->update<NodeType::theory_right_guard>(lib, [&]<Attribute Attr>() {
                if constexpr (Attr == Attribute::theory_operator) {
                    return negate_relation(guard->string(Attribute::theory_operator));
                }
            });
        }
        body.erase(it);
        return ast.update<NodeType::statement_rule>(lib, [&]<Attribute Attr>() {
            if constexpr (Attr == Attribute::head) {
                return Node::create<NodeType::head_theory_atom>(lib, lit.location(Attribute::location), name, elems,
                                                                guard);
            }
            if constexpr (Attr == Attribute::body) {
                return std::move(body);
            }
        });
    }
    return ast;
}

//! Tag terms depending on whether they occur in heads or bodies.
[[maybe_unused]] auto tag_terms(Clingo::Library const &lib, Clingo::AST::Node const &ast, std::string_view tag)
    -> Clingo::AST::Node {
    using namespace Clingo::AST;
    auto tag_string = [tag](std::string_view str) {
        auto res = std::string{"__"};
        res.reserve(res.size() + str.size() + tag.size());
        res += str;
        res += tag;
        return res;
    };
    if (ast.type() == NodeType::term_symbolic) {
        auto term = ast.symbol(Attribute::symbol);
        assert(term.match("diff", 0));
        auto sym = Clingo::Function(lib, tag_string(term.name()), {});
        return Node::create<NodeType::term_symbolic>(lib, ast.location(Attribute::location), sym);
    }
    if (ast.type() == NodeType::term_function) {
        return ast.update<NodeType::term_function>(lib, [&]<Attribute Attr>() {
            if constexpr (Attr == Attribute::name) {
                return tag_string(ast.string(Attribute::name));
            }
        });
    }
    return throw_syntax_error<Node>();
}

using VarSet = std::set<std::string_view>;

void collect_variables(VarSet &vars, Clingo::AST::Node const &ast) {
    if (ast.type() == Clingo::AST::NodeType::term_variable ||
        ast.type() == Clingo::AST::NodeType::theory_term_variable) {
        vars.emplace(ast.string(Clingo::AST::Attribute::name));
    } else {
        ast.accept([&vars](auto const &child) { collect_variables(vars, child); });
    }
}

template <typename It> auto collect_variables(It ib, It ie) -> VarSet {
    VarSet vars;
    for (; ib != ie; ++ib) {
        collect_variables(vars, *ib);
    }
    return vars;
}

// NOTE: msvc cannot handle nested templated lambdas
template <Clingo::AST::NodeType N> struct Update {
    template <Clingo::AST::Attribute attr> auto operator()() const {
        using namespace Clingo::AST;
        if constexpr (attr == Attribute::name) {
            return tag_terms(lib, term, N == NodeType::body_theory_atom ? "_b" : "_h");
        }
    }
    Clingo::Library const &lib;    // NOLINT
    Clingo::AST::Node const &term; // NOLINT
};

// NOTE: condidate to put in clingo
class Unpooler {
  public:
    Unpooler(Clingo::Library const &lib) : lib_{&lib} {}

    //! Unpools vectors of simple literals and terms.
    auto unpool(std::vector<Clingo::AST::Node> const &nodes) -> std::vector<std::vector<Clingo::AST::Node>> {
        using namespace Clingo::AST;
        std::vector<std::vector<Node>> unpooled_nodes;
        unpooled_nodes.emplace_back();
        for (const auto &node : nodes) {
            auto unpooled_node = unpool(node);
            std::vector<std::vector<Node>> temp;
            for (const auto &partial : unpooled_nodes) {
                for (const auto &arg_tuple : unpooled_node) {
                    temp.emplace_back(partial);
                    temp.back().push_back(arg_tuple);
                }
            }
            unpooled_nodes = std::move(temp);
        }
        return unpooled_nodes;
    }

    //! Unpools simple literals and terms.
    auto unpool(Clingo::AST::Node const &node) -> std::vector<Clingo::AST::Node> {
        using namespace Clingo::AST;
        switch (node.type()) {
            case NodeType::argument_tuple: {
                std::vector<Node> result;
                for (auto const &args : unpool(node.nodes(Attribute::arguments))) {
                    result.emplace_back(Node::create<NodeType::argument_tuple>(*lib_, args));
                }
                return result;
            }
            case NodeType::right_guard: {
                std::vector<Node> result;
                for (auto const &arg : unpool(node.node(Attribute::term))) {
                    result.emplace_back(node.update<NodeType::right_guard>(*lib_, [&]<Attribute Attr>() {
                        if constexpr (Attr == Attribute::term) {
                            return arg;
                        }
                    }));
                }
                return result;
            }
            case NodeType::term_function: {
                std::vector<Node> result;
                for (auto const &pool : node.nodes(Attribute::pool)) {
                    for (auto const &arg : unpool(pool)) {
                        result.emplace_back(node.update<NodeType::term_function>(*lib_, [&]<Attribute Attr>() {
                            if constexpr (Attr == Attribute::pool) {
                                return std::vector{arg};
                            }
                        }));
                    }
                }
                return result;
            }
            case Clingo::AST::NodeType::term_absolute: {
                std::vector<Node> result;
                for (auto const &term : node.nodes(Attribute::pool)) {
                    for (auto const &unpooled : unpool(term)) {
                        result.emplace_back(node.update<NodeType::term_absolute>(*lib_, [&]<Attribute Attr>() {
                            if constexpr (Attr == Attribute::pool) {
                                return std::vector{unpooled};
                            }
                        }));
                    }
                }
                return result;
            }
            case Clingo::AST::NodeType::term_binary_operation: {
                std::vector<Node> result;
                for (auto const &lhs : unpool(node.node(Attribute::left))) {
                    for (auto const &rhs : unpool(node.node(Attribute::right))) {
                        result.emplace_back(node.update<NodeType::term_binary_operation>(*lib_, [&]<Attribute Attr>() {
                            if constexpr (Attr == Attribute::left) {
                                return lhs;
                            }
                            if constexpr (Attr == Attribute::right) {
                                return rhs;
                            }
                        }));
                    }
                }
                return result;
            }
            case Clingo::AST::NodeType::term_tuple: {
                std::vector<Node> result;
                for (auto const &pool : node.nodes(Attribute::pool)) {
                    for (auto const &arg : unpool(pool)) {
                        result.emplace_back(node.update<NodeType::term_tuple>(*lib_, [&]<Attribute Attr>() {
                            if constexpr (Attr == Attribute::pool) {
                                return std::vector{arg};
                            }
                        }));
                    }
                }
                return result;
            }
            case Clingo::AST::NodeType::term_unary_operation: {
                std::vector<Node> result;
                for (auto const &rhs : unpool(node.node(Attribute::right))) {
                    result.emplace_back(node.update<NodeType::term_unary_operation>(*lib_, [&]<Attribute Attr>() {
                        if constexpr (Attr == Attribute::right) {
                            return rhs;
                        }
                    }));
                }
                return result;
            }
            case Clingo::AST::NodeType::literal_comparison: {
                std::vector<Node> result;
                for (auto const &lhs : unpool(node.node(Attribute::left))) {
                    for (auto const &rhs : unpool(node.nodes(Attribute::right))) {
                        result.emplace_back(node.update<NodeType::literal_comparison>(*lib_, [&]<Attribute Attr>() {
                            if constexpr (Attr == Attribute::left) {
                                return lhs;
                            }
                            if constexpr (Attr == Attribute::right) {
                                return rhs;
                            }
                        }));
                    }
                }
                return result;
            }
            case Clingo::AST::NodeType::literal_symbolic: {
                std::vector<Node> result;
                for (auto const &atom : unpool(node.node(Attribute::atom))) {
                    result.emplace_back(node.update<NodeType::literal_symbolic>(*lib_, [&]<Attribute Attr>() {
                        if constexpr (Attr == Attribute::atom) {
                            return atom;
                        }
                    }));
                }
                return result;
            }
            case Clingo::AST::NodeType::literal_boolean:
            case Clingo::AST::NodeType::term_symbolic:
            case Clingo::AST::NodeType::term_variable: {
                return {node};
            }
            default: {
                return throw_syntax_error<std::vector<Clingo::AST::Node>>("unexpected element");
            }
        }
    }

  private:
    Clingo::Library const *lib_;
};

// Lift non-singular pools into conditions.
auto lift(Clingo::Library const &lib, Clingo::AST::Node const &ast) -> std::optional<Clingo::AST::Node> {
    // NOTE: condidate to put in clingo
    using namespace Clingo::AST;
    using T = NodeType;
    using A = Attribute;
    std::vector<Node> lifted;
    int number = 0;
    std::string prefix = "__CLINGCON_";
    bool anonymous = false;
    Transformer trans = [&](Clingo::AST::Node ast) -> std::optional<Node> {
        auto sub = ast.accept(lib, trans);
        if (sub) {
            ast = *sub;
        }
        auto type = ast.type();
        if (type == T::literal_symbolic || type == T::literal_boolean || type == T::literal_comparison) {
            anonymous = type == T::literal_symbolic && ast.number(Attribute::sign) == Sign::no_sign;
        }
        // TODO: dots is missing in action
        static constexpr auto dots = 9;
        if ((type == T::term_binary_operation && ast.number(A::operator_type) == dots) ||
            (type == T::term_function && ast.number(A::external) == 1)) {
            auto loc = ast.location(A::location);
            auto lhs = Node::create<T::term_variable>(lib, loc, prefix + std::to_string(number++), false);
            auto rhs = std::vector{Node::create<T::right_guard>(lib, Relation::equal, ast)};
            lifted.emplace_back(Node::create<T::literal_comparison>(lib, loc, Sign::no_sign, lhs, rhs));
            return lhs;
        }
        // NOTE: we only replace anonymous variables in positive constexts
        // (in other contexts they are either unsafe or handled specially)
        if (anonymous && type == T::term_variable && ast.number(A::anonymous) == 1) {
            auto loc = ast.location(A::location);
            return Node::create<T::term_variable>(lib, loc, prefix + std::to_string(number++), false);
        }
        return sub;
    };
    if (auto x = trans(ast)) {
        return x->update<NodeType::theory_atom_element>(lib, [&]<A Attr>() {
            if constexpr (Attr == A::condition) {
                auto cond = x->nodes(A::condition);
                cond.insert(cond.end(), lifted.begin(), lifted.end());
                return cond;
            }
        });
    }
    return trans(ast);
}

//! Unpool conditions of elements and lift pools with (potentially) more than one element into conditions.
template <Clingo::AST::NodeType T>
auto rewrite_elements(Clingo::Library const &lib, Clingo::AST::Node const &ast) -> Clingo::AST::Node {
    using namespace Clingo::AST;
    auto result = std::vector<Node>{};
    Unpooler upl{lib};
    for (auto const &elem : ast.nodes(Attribute::elements)) {
        for (auto const &cond : upl.unpool(elem.nodes(Attribute::condition))) {
            auto x = elem.update<NodeType::theory_atom_element>(lib, [&]<Attribute A>() {
                if constexpr (A == Attribute::condition) {
                    return cond;
                }
            });
            if (auto y = lift(lib, x)) {
                x = *std::move(y);
            }
            result.emplace_back(std::move(x));
        }
    }
    return ast.update<T>(lib, [&]<Attribute A>() {
        if constexpr (A == Attribute::elements) {
            return result;
        }
    });
}

// Ensures multiset semantics.
auto rewrite_tuple(Clingo::Library const &lib, Clingo::AST::Node const &ast) -> std::optional<Clingo::AST::Node> {
    using namespace Clingo::AST;
    using T = NodeType;
    using A = Attribute;
    int number = 0;
    bool enumerate = ast.nodes(A::elements).size() > 1;
    Transformer trans = [&](Clingo::AST::Node const &ast) -> std::optional<Node> {
        if (ast.type() == T::theory_atom_element) {
            auto tuple = ast.nodes(A::tuple);
            check_syntax(tuple.size() == 1);
            auto condition = ast.nodes(A::condition);
            auto vars_condition = collect_variables(condition.begin(), condition.end());
            for (auto const &name : collect_variables(tuple.begin(), tuple.end())) {
                vars_condition.erase(name);
            }
            auto loc = tuple.front().location(A::location);
            if (enumerate) {
                auto num = Node::create<T::theory_term_symbolic>(lib, loc, Clingo::Number(number++));
                tuple.emplace_back(std::move(num));
            }
            for (auto const &name : vars_condition) {
                auto num = Node::create<T::theory_term_variable>(lib, loc, name, false);
                tuple.emplace_back(std::move(num));
            }
            return ast.update<T::theory_atom_element>(lib, [&]<A Attr>() {
                if constexpr (Attr == A::tuple) {
                    return tuple;
                }
            });
        }
        return ast.accept(lib, trans);
    };
    return trans(ast);
}

// Tags head and body atoms and ensures multiset semantics.
auto rewrite_theory(Clingo::Library const &lib, Clingo::AST::Node const &ast) -> std::optional<Clingo::AST::Node> {
    using namespace Clingo::AST;
    using T = NodeType;
    using A = Attribute;
    auto transform = [&]<T N>(Clingo::AST::Node const &ast) -> std::optional<Node> {
        auto term = ast.node(A::name);
        if (match_constant(term, "sum", "nsum", "diff", "distinct", "disjoint", "minimize", "maximize")) {
            Clingo::AST::Node res = rewrite_elements<N>(lib, ast);
            if (auto x = rewrite_tuple(lib, res)) {
                res = *x;
            }
            if (match_constant(term, "sum", "nsum", "diff")) {
                res = res.update<N>(lib, Update<N>{lib, term});
            }
            return res;
        }
        return std::nullopt;
    };
    Transformer trans = [&](Clingo::AST::Node const &ast) -> std::optional<Node> {
        if (ast.type() == T::body_theory_atom) {
            return transform.template operator()<T::body_theory_atom>(ast);
        }
        if (ast.type() == T::head_theory_atom) {
            return transform.template operator()<T::head_theory_atom>(ast);
        }
        return ast.accept(lib, trans);
    };
    return trans(ast);
}

[[nodiscard]] auto evaluate(Clingo::Library const &lib, Clingo::TheoryTerm const &term) -> Clingo::Symbol;

template <class F>
[[nodiscard]] auto evaluate(Clingo::Library const &lib, Clingo::TheoryTerm const &a, Clingo::TheoryTerm const &b, F f)
    -> Clingo::Symbol {
    auto ea = evaluate(lib, a);
    check_syntax(ea.type() == Clingo::SymbolType::number);
    auto eb = evaluate(lib, b);
    check_syntax(eb.type() == Clingo::SymbolType::number);
    return Clingo::Number(f(ea.number(), eb.number()));
}

auto safe_pow(val_t a, val_t b) -> val_t {
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

inline auto unquote(std::string_view str) -> std::string {
    std::string res;
    bool slash = false;
    if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
        str = str.substr(1, str.size() - 2);
    }
    for (char c : str) {
        if (slash) {
            switch (c) {
                case 'n': {
                    res.push_back('\n');
                    break;
                }
                case '\\': {
                    res.push_back('\\');
                    break;
                }
                case '"': {
                    res.push_back('"');
                    break;
                }
                default: {
                    assert(false);
                    break;
                }
            }
            slash = false;
        } else if (c == '\\') {
            slash = true;
        } else {
            res.push_back(c);
        }
    }
    return res;
}

auto evaluate(Clingo::Library const &lib, Clingo::TheoryTerm const &term) -> Clingo::Symbol {
    if (term.type() == Clingo::TheoryTermType::symbol) {
        auto name = term.name();
        if (!name.empty() && name.front() == '"' && name.back() == '"') {
            return Clingo::String(lib, unquote(name));
        }
        return Clingo::Function(lib, name, {});
    }

    if (term.type() == Clingo::TheoryTermType::number) {
        return Clingo::Number(term.number());
    }

    if (match(term, "+", 2)) {
        return evaluate(lib, term.arguments().front(), term.arguments().back(), safe_add<val_t>);
    }
    if (match(term, "-", 2)) {
        return evaluate(lib, term.arguments().front(), term.arguments().back(), safe_sub<val_t>);
    }
    if (match(term, "*", 2)) {
        return evaluate(lib, term.arguments().front(), term.arguments().back(), safe_mul<val_t>);
    }
    if (match(term, "/", 2)) {
        return evaluate(lib, term.arguments().front(), term.arguments().back(), safe_div<val_t>);
    }
    if (match(term, "\\", 2)) {
        return evaluate(lib, term.arguments().front(), term.arguments().back(), safe_mod<val_t>);
    }
    if (match(term, "**", 2)) {
        return evaluate(lib, term.arguments().front(), term.arguments().back(), safe_pow);
    }

    if (match(term, "-", 1)) {
        auto ea = evaluate(lib, term.arguments().front());
        if (ea.type() == Clingo::SymbolType::number) {
            return Clingo::Number(safe_inv(ea.number()));
        }
        if (ea.type() == Clingo::SymbolType::function && !ea.name().empty()) {
            return Clingo::Function(lib, ea.name(), ea.arguments(), !ea.is_positive());
        }
        return throw_syntax_error<Clingo::Symbol>();
    }

    check_syntax(!match(term, "..", 2));

    if (term.type() == Clingo::TheoryTermType::tuple || term.type() == Clingo::TheoryTermType::function) {
        std::vector<Clingo::Symbol> args;
        args.reserve(term.arguments().size());
        for (auto const &arg : term.arguments()) {
            args.emplace_back(evaluate(lib, arg));
        }
        return term.type() == Clingo::TheoryTermType::tuple ? Clingo::Tuple(lib, args)
                                                            : Clingo::Function(lib, term.name(), args);
    }

    return throw_syntax_error<Clingo::Symbol>();
}

using VarVec = std::vector<var_t>;
using NonlinearTerm = std::pair<val_t, VarVec>;
using NonlinearTermVec = std::vector<NonlinearTerm>;

// NOTE: no need for a good hash function
struct VectorHash {
    template <class T> auto operator()(std::vector<T> const &vec) const -> std::size_t {
        std::size_t ret = 0;
        for (auto &i : vec) {
            ret += std::hash<T>()(i);
        }
        return ret;
    }
};

void push_co(val_t co, CoVarVec &res) {
    res.emplace_back(co, INVALID_VAR);
}

void push_co(val_t co, NonlinearTermVec &res) {
    res.emplace_back(co, VarVec{});
}

void push_co_var(val_t co, var_t var, CoVarVec &res) {
    res.emplace_back(co, var);
}

void push_co_var(val_t co, var_t var, NonlinearTermVec &res) {
    res.emplace_back(co, VarVec{var});
}

void push_co_vars(val_t co, var_t l_var, var_t r_var, CoVarVec &res) {
    if (!is_valid_var(l_var)) {
        res.emplace_back(co, r_var);
    } else if (!is_valid_var(r_var)) {
        res.emplace_back(co, l_var);
    } else {
        throw_syntax_error("Invalid Syntax: only linear sum constraints are supported");
    }
}

void push_co_vars(val_t co, VarVec const &l_vars, VarVec const &r_vars, NonlinearTermVec &res) {
    auto vars = l_vars;
    vars.insert(vars.end(), r_vars.begin(), r_vars.end());
    res.emplace_back(co, std::move(vars));
}

void push_value(Clingo::Symbol const &sym, CoVarVec &res) {
    check_syntax(sym.type() == Clingo::SymbolType::number);
    push_co(sym.number(), res);
}

void push_value(Clingo::Symbol const &sym, NonlinearTermVec &res) {
    check_syntax(sym.type() == Clingo::SymbolType::number);
    push_co(sym.number(), res);
}
using Clingcon::simplify;

auto simplify(NonlinearTermVec &vec, bool drop_zero) -> val_t {
    static thread_local std::unordered_map<VarVec, NonlinearTermVec::iterator, VectorHash> seen;
    val_t rhs = 0;

    seen.clear();

    auto jt = vec.begin();
    for (auto it = jt, ie = vec.end(); it != ie; ++it) {
        auto &[co, vars] = *it;
        std::ranges::sort(vars);
        if (drop_zero && co == 0) {
            continue;
        }
        if (vars.empty()) {
            rhs = safe_sub(rhs, co);
        } else if (auto [kt, ins] = seen.try_emplace(vars, jt); !ins) {
            kt->second->first = safe_add(kt->second->first, co);
        } else {
            if (it != jt) {
                *jt = *it;
            }
            ++jt;
        }
    }

    if (drop_zero) {
        jt = std::remove_if(vec.begin(), jt, [](auto &co_var) { return co_var.first == 0; });
    }

    vec.erase(jt, vec.end());

    // overflow checking (maybe put in separate function)
    check_valid_value(rhs);
    nsum_t min = rhs;
    nsum_t max = rhs;
    for (auto const &[co, vars] : vec) {
        check_valid_value(co);
        // Note: we only consider the linear part here. The coefficient of the
        // non-linear part will be handled specially during propagation to
        // prevent overflows.
        if (vars.size() == 1) {
            min = safe_add<nsum_t>(min, safe_mul<nsum_t>(co, co > 0 ? MIN_VAL : MAX_VAL));
            max = safe_add<nsum_t>(max, safe_mul<nsum_t>(co, co > 0 ? MAX_VAL : MIN_VAL));
        }
    }
    safe_inv(min);
    safe_inv(max);

    return rhs;
}

[[nodiscard]] auto add_constraint(AbstractConstraintBuilder &builder, lit_t literal, CoVarVec const &elements,
                                  val_t rhs, bool strict) -> bool {
    return builder.add_constraint(literal, elements, rhs, strict);
}

[[nodiscard]] auto add_constraint(AbstractConstraintBuilder &builder, lit_t literal, NonlinearTermVec const &elements,
                                  val_t rhs, bool strict) -> bool {
    var_t var_a{INVALID_VAR};
    var_t var_b{INVALID_VAR};
    val_t co_ab{0};
    var_t var_c{INVALID_VAR};
    val_t co_c{0};
    for (auto const &[co, vars] : elements) {
        check_syntax(vars.size() <= 2, "nonlinear terms with more than 2 variables are not supported");
        if (vars.size() == 1) {
            check_syntax(co_c == 0, "nonlinear sums can have at most one linear term");
            co_c = co;
            var_c = vars.front();
        }
        if (vars.size() == 2) {
            check_syntax(co_c == 0, "nonlinear sums can have at most one nonlinear term");
            co_ab = co;
            var_a = vars.front();
            var_b = vars.back();
        }
    }
    return builder.add_nonlinear(literal, co_ab, var_a, var_b, co_c, var_c, rhs, strict);
}

template <class TermVec, bool is_sum = true>
void parse_constraint_elem(Clingo::Library const &lib, AbstractConstraintBuilder &builder,
                           Clingo::TheoryTerm const &term, TermVec &res) {
    if constexpr (!is_sum) {
        if (match(term, "-", 2)) {
            auto args = term.arguments();

            auto a = evaluate(lib, args.front());
            if (a.type() == Clingo::SymbolType::number) {
                push_co(a.number(), res);
            } else {
                push_co_var(1, builder.add_variable(a), res);
            }

            auto b = evaluate(lib, args.back());
            if (b.type() == Clingo::SymbolType::number) {
                push_co(safe_inv(b.number()), res);
            } else {
                push_co_var(-1, builder.add_variable(b), res);
            }
        } else {
            throw_syntax_error("Invalid Syntax: invalid difference constraint");
        }
    } else if (term.type() == Clingo::TheoryTermType::number) {
        push_co(term.number(), res);
    } else if (match(term, "+", 2)) {
        auto args = term.arguments();
        parse_constraint_elem<TermVec>(lib, builder, args.front(), res);
        parse_constraint_elem<TermVec>(lib, builder, args.back(), res);
    } else if (match(term, "-", 2)) {
        auto args = term.arguments();
        parse_constraint_elem<TermVec>(lib, builder, args.front(), res);
        auto pos = res.size();
        parse_constraint_elem<TermVec>(lib, builder, args.back(), res);
        for (auto it = res.begin() + pos, ie = res.end(); it != ie; ++it) {
            it->first = safe_inv(it->first);
        }
    } else if (match(term, "-", 1)) {
        auto pos = res.size();
        parse_constraint_elem<TermVec>(lib, builder, term.arguments().front(), res);
        for (auto it = res.begin() + pos, ie = res.end(); it != ie; ++it) {
            it->first = safe_inv(it->first);
        }
    } else if (match(term, "+", 1)) {
        parse_constraint_elem<TermVec>(lib, builder, term.arguments().front(), res);
    } else if (match(term, "*", 2)) {
        auto args = term.arguments();
        TermVec lhs, rhs; // NOLINT
        parse_constraint_elem<TermVec>(lib, builder, args.front(), lhs);
        parse_constraint_elem<TermVec>(lib, builder, args.back(), rhs);
        for (auto &[l_co, l_vars] : lhs) {
            for (auto &[r_co, r_vars] : rhs) {
                push_co_vars(safe_mul(l_co, r_co), l_vars, r_vars, res);
            }
        }
    } else if (match(term, "**", 2) || match(term, "/", 2) || match(term, "\\", 2)) {
        push_value(evaluate(lib, term), res);
    } else if (term.type() == Clingo::TheoryTermType::symbol || term.type() == Clingo::TheoryTermType::function ||
               term.type() == Clingo::TheoryTermType::tuple) {
        push_co_var(1, builder.add_variable(evaluate(lib, term)), res);
    } else {
        throw_syntax_error("Invalid Syntax: invalid sum constraint");
    }
}

template <class TermVec, bool is_sum = true>
void parse_constraint_elems(Clingo::Library const &lib, AbstractConstraintBuilder &builder,
                            std::span<Clingo::TheoryElement const> elements, Clingo::TheoryTerm const *rhs,
                            TermVec &res) {
    check_syntax(is_sum || elements.size() == 1, "Invalid Syntax: invalid difference constraint");

    for (auto const &element : elements) {
        auto tuple = element.tuple();
        check_syntax(!tuple.empty() && element.condition().empty(), "Invalid Syntax: invalid sum constraint");
        parse_constraint_elem<TermVec, is_sum>(lib, builder, element.tuple().front(), res);
    }

    if (rhs != nullptr) {
        if constexpr (is_sum) {
            auto pos = res.size();
            parse_constraint_elem<TermVec, is_sum>(lib, builder, *rhs, res);
            for (auto it = res.begin() + pos, ie = res.end(); it != ie; ++it) {
                it->first = safe_inv(it->first);
            }
        } else {
            auto term = evaluate(lib, *rhs);
            check_syntax(term.type() == Clingo::SymbolType::number, "Invalid Syntax: invalid difference constraint");
            push_co(safe_inv(term.number()), res);
        }
    }
}

template <class TermVec>
[[nodiscard]] auto normalize_constraint(AbstractConstraintBuilder &builder, lit_t literal, TermVec const &elements,
                                        std::string_view op, val_t rhs, bool strict) -> bool {
    TermVec copy;
    TermVec const *elems = &elements;

    // rewrite '>', '<', and '>=' into '<='
    if (op == ">") {
        op = ">=";
        rhs = safe_add(rhs, 1);
    } else if (op == "<") {
        op = "<=";
        rhs = safe_sub(rhs, 1);
    }
    if (op == ">=") {
        op = "<=";
        rhs = safe_inv(rhs);
        copy.reserve(elements.size());
        for (auto const &[co, var] : elements) {
            copy.emplace_back(safe_inv(co), var);
        }
        elems = &copy;
    }

    // handle remaining '<=', '=', and '!='
    if (op == "<=") {
        if (strict && elems->size() == 1) {
            return add_constraint(builder, literal, *elems, rhs, true);
        }
        if (!builder.is_true(-literal) && !add_constraint(builder, literal, *elems, rhs, false)) {
            return false;
        }
    } else if (op == "=") {
        lit_t a, b; // NOLINT
        if (strict) {
            if (builder.is_true(literal)) {
                a = b = TRUE_LIT;
            } else {
                a = builder.add_literal();
                b = builder.add_literal();
            }

            // Note: this cannot fail because constraint normalization does not propagate
            if (!builder.add_clause(std::to_array({-literal, a}))) {
                return false;
            }
            if (!builder.add_clause(std::to_array({-literal, b}))) {
                return false;
            }
            if (!builder.add_clause(std::to_array({-a, -b, literal}))) {
                return false;
            }
        } else {
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
    } else if (op == "!=") {
        if (strict) {
            return normalize_constraint(builder, -literal, *elems, "=", rhs, true);
        }

        auto a = builder.add_literal();
        auto b = builder.add_literal();

        if (!builder.add_clause(std::to_array({a, b, -literal}))) {
            return false;
        }
        if (!builder.add_clause(std::to_array({-a, -b}))) {
            return false;
        }

        if (!builder.add_clause(std::to_array({literal, -a}))) {
            return false;
        }
        if (!builder.add_clause(std::to_array({literal, -b}))) {
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
        assert(op != "=");

        if (op == "<=") {
            op = ">";
        } else if (op == "!=") {
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
// Constraints are represented as a triple of a literal, its elements, and an
// upper bound.
template <class TermVec, bool is_sum = true>
[[nodiscard]] auto parse_constraint(Clingo::Library const &lib, AbstractConstraintBuilder &builder,
                                    Clingo::TheoryAtom const &atom, bool strict) -> bool {
    auto guard = atom.guard();
    check_syntax(guard.has_value());

    TermVec elements;
    val_t rhs{0};
    auto literal = builder.solver_literal(atom.literal());

    // combine coefficients
    parse_constraint_elems<TermVec, is_sum>(lib, builder, atom.elements(), &guard->second, elements);
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

    return normalize_constraint(builder, literal, elements, guard->first, rhs, strict);
}

// Parses minimize and maximize directives.
void parse_objective(Clingo::Library const &lib, AbstractConstraintBuilder &builder, Clingo::TheoryAtom const &atom,
                     int factor) {
    CoVarVec elems;
    parse_constraint_elems<CoVarVec>(lib, builder, atom.elements(), nullptr, elems);
    for (auto &[co, var] : elems) {
        builder.add_minimize(safe_mul(factor, co), var);
    }
}

void parse_show_elem(Clingo::Library const &lib, AbstractConstraintBuilder &builder, Clingo::TheoryTerm const &term) {
    if (match(term, "/", 2)) {
        auto args = term.arguments();

        auto a = evaluate(lib, args.front());
        check_syntax(a.type() == Clingo::SymbolType::function && a.arguments().empty(),
                     "Invalid Syntax: invalid show statement");

        auto b = evaluate(lib, args.back());
        check_syntax(b.type() == Clingo::SymbolType::number, "Invalid Syntax: invalid show statement");

        builder.show_signature(a.name(), b.number());
    } else {
        auto a = evaluate(lib, term);
        check_syntax(a.type() != Clingo::SymbolType::number, "Invalid Syntax: invalid show statement");

        builder.show_variable(builder.add_variable(a));
    }
}
void parse_show(Clingo::Library const &lib, AbstractConstraintBuilder &builder, Clingo::TheoryAtom const &atom) {
    builder.add_show();

    for (auto elem : atom.elements()) {
        check_syntax(elem.tuple().size() == 1 && elem.condition().empty(), "Invalid Syntax: invalid show statement");
        parse_show_elem(lib, builder, elem.tuple().front());
    }
}

[[nodiscard]] auto parse_dom_elem(Clingo::Library const &lib, Clingo::TheoryTerm const &term)
    -> std::pair<val_t, val_t> {
    if (match(term, "..", 2)) {
        auto args = term.arguments();

        auto a = evaluate(lib, args.front());
        check_syntax(a.type() == Clingo::SymbolType::number, "Invalid Syntax: invalid dom statement");

        auto b = evaluate(lib, args.back());
        check_syntax(b.type() == Clingo::SymbolType::number, "Invalid Syntax: invalid dom statement");

        return {a.number(), safe_add(b.number(), 1)};
    }

    auto a = evaluate(lib, term);
    check_syntax(a.type() == Clingo::SymbolType::number, "Invalid Syntax: invalid dom statement");

    return {a.number(), safe_add(a.number(), 1)};
}

[[nodiscard]] auto parse_dom(Clingo::Library const &lib, AbstractConstraintBuilder &builder,
                             Clingo::TheoryAtom const &atom) -> bool {
    IntervalSet<val_t> elements;
    for (auto elem : atom.elements()) {
        auto tuple = elem.tuple();
        check_syntax(tuple.size() == 1 && elem.condition().empty(), "Invalid Syntax: invalid dom statement");
        auto [l, r] = parse_dom_elem(lib, tuple.front());
        if (l < r) {
            check_valid_value(l);
            check_valid_value(safe_sub(r, 1));
            elements.add(l, r);
        }
    }

    auto guard = atom.guard();
    check_syntax(guard.has_value(), "Invalid Syntax: invalid dom statement");
    auto var = evaluate(lib, guard->second);
    check_syntax(var.type() != Clingo::SymbolType::number, "Invalid Syntax: invalid dom statement");

    return builder.add_dom(builder.solver_literal(atom.literal()), builder.add_variable(var), elements);
}

// Currently only distinct constraints in the head are supported. Supporting
// them in the body would also be possible where they should be strict.
[[nodiscard]] auto parse_distinct(Clingo::Library const &lib, AbstractConstraintBuilder &builder,
                                  Clingo::TheoryAtom const &atom) -> bool {
    std::vector<std::pair<CoVarVec, val_t>> elements;

    for (auto elem : atom.elements()) {
        auto tuple = elem.tuple();
        check_syntax(!tuple.empty() && elem.condition().empty(), "Invalid Syntax: invalid distinct statement");
        elements.emplace_back();
        auto &term = elements.back();
        parse_constraint_elem<CoVarVec>(lib, builder, tuple.front(), term.first);
        term.second = safe_inv(simplify(term.first));
    }

    return builder.add_distinct(builder.solver_literal(atom.literal()), elements);
}

[[nodiscard]] auto parse_disjoint(Clingo::Library const &lib, AbstractConstraintBuilder &builder,
                                  Clingo::TheoryAtom const &atom) -> bool {
    CoVarVec elements;

    for (auto elem : atom.elements()) {
        auto tuple = elem.tuple();
        check_syntax(!tuple.empty() && elem.condition().empty(), "Invalid Syntax: invalid disjoint statement");
        check_syntax(match(tuple.front(), "@", 2), "Invalid Syntax: invalid disjoint statement");
        auto args = tuple.front().arguments();
        auto var = evaluate(lib, args.front());
        auto val = evaluate(lib, args.back());
        check_syntax(var.type() != Clingo::SymbolType::number, "Invalid Syntax: invalid disjoint statement");
        check_syntax(val.type() == Clingo::SymbolType::number, "Invalid Syntax: invalid disjoint statement");
        if (val.number() > 0) {
            elements.emplace_back(check_valid_value(val.number()), builder.add_variable(var));
        }
    }

    return builder.add_disjoint(builder.solver_literal(atom.literal()), elements);
}

} // namespace

auto simplify(CoVarVec &vec, bool drop_zero) -> val_t {
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
        } else if (auto [kt, ins] = seen.try_emplace(var, jt); !ins) {
            kt->second->first = safe_add(kt->second->first, co);
        } else {
            if (it != jt) {
                *jt = *it;
            }
            ++jt;
        }
    }

    if (drop_zero) {
        jt = std::remove_if(vec.begin(), jt, [](auto &co_var) { return co_var.first == 0; });
    }

    vec.erase(jt, vec.end());

    // overflow checking (maybe put in separate function)
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

void transform(Clingo::Library const &lib, Clingo::AST::Node const &ast, NodeCallback const &cb, bool shift) {
    std::optional<Clingo::AST::Node> res;
    if (shift) {
        res = shift_rule(lib, ast);
    }
    if (auto x = rewrite_theory(lib, res ? *res : ast)) {
        res = x;
    }
    cb(res ? *std::move(res) : ast);
}

auto parse(Clingo::Library const &lib, AbstractConstraintBuilder &builder, Clingo::TheoryBase theory_atoms) -> bool {
    for (auto const &atom : theory_atoms) {
        bool is_sum_b = match(atom.name(), "__sum_b", 0);
        bool is_sum_h = match(atom.name(), "__sum_h", 0);
        bool is_diff_b = match(atom.name(), "__diff_b", 0);
        bool is_diff_h = match(atom.name(), "__diff_h", 0);
        bool is_nsum_h = match(atom.name(), "__nsum_h", 0);
        bool is_nsum_b = match(atom.name(), "__nsum_b", 0);
        if (is_sum_b || is_sum_h) {
            if (!parse_constraint<CoVarVec>(lib, builder, atom, is_sum_b)) {
                return false;
            }
        }
        if (is_diff_b || is_diff_h) {
            if (!parse_constraint<CoVarVec, false>(lib, builder, atom, is_diff_b)) {
                return false;
            }
        } else if (is_nsum_b || is_nsum_h) {
            // could be done more cleverly by merging into sum constraint
            if (!parse_constraint<NonlinearTermVec>(lib, builder, atom, is_nsum_b)) {
                return false;
            }
        } else if (match(atom.name(), "distinct", 0)) {
            if (!parse_distinct(lib, builder, atom)) {
                return false;
            }
        } else if (match(atom.name(), "disjoint", 0)) {
            if (!parse_disjoint(lib, builder, atom)) {
                return false;
            }
        } else if (match(atom.name(), "show", 0)) {
            parse_show(lib, builder, atom);
        } else if (match(atom.name(), "dom", 0)) {
            if (!parse_dom(lib, builder, atom)) {
                return false;
            }
        } else if (match(atom.name(), "minimize", 0)) {
            parse_objective(lib, builder, atom, 1);
        } else if (match(atom.name(), "maximize", 0)) {
            parse_objective(lib, builder, atom, -1);
        }
    }
    return true;
}

[[nodiscard]] auto match(Clingo::TheoryTerm const &term, std::string_view name, size_t arity) -> bool {
    return (term.type() == Clingo::TheoryTermType::symbol && term.name() == name && arity == 0) ||
           (term.type() == Clingo::TheoryTermType::function && term.name() == name && term.arguments().size() == arity);
}

} // namespace Clingcon
