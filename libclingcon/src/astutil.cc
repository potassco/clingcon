#include "clingcon/astutil.hh"

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

} // namespace

bool match(Clingo::AST::TheoryTerm const &term, char const *name, size_t arity) {
    return term.data.accept(MatchVisitor{name, arity});
}

