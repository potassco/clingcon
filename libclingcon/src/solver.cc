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

#include "clingcon/solver.hh"
#include "clingcon/util.hh"

#include <clingo.hh>
#include <unordered_set>

namespace Clingcon {

//! Class that helps to maintain per decision level state.
class Solver::Level {
  public:
    Level(Solver &solver, level_t level)
        : level_{level}, undo_lower_offset_{solver.undo_lower_.size()}, undo_upper_offset_{solver.undo_upper_.size()},
          inactive_offset_{solver.inactive_.size()}, removed_var_watches_offset_{solver.removed_var_watches_.size()} {}

    [[nodiscard]] auto level() const -> level_t { return level_; }

    //! Update the lower bound of a var state.
    void update_lower(Solver &solver, VarState &vs, val_t value) const {
        val_t diff = value + 1 - vs.lower_bound();
        if (level_ > 0 && !vs.pushed_lower(level_)) {
            vs.push_lower(level_);
            solver.undo_lower_.emplace_back(vs.var());
        }
        vs.lower_bound(value + 1);

        if (solver.ldiff_[vs.var()] == 0) {
            solver.in_ldiff_.emplace_back(vs.var());
        }
        solver.ldiff_[vs.var()] += diff;
    }

    //! Update the upper bound of a var state.
    void update_upper(Solver &solver, VarState &vs, val_t value) const {
        val_t diff = value - vs.upper_bound();

        if (level_ > 0 && !vs.pushed_upper(level_)) {
            vs.push_upper(level_);
            solver.undo_upper_.emplace_back(vs.var());
        }
        vs.upper_bound(value);

        if (solver.udiff_[vs.var()] == 0) {
            solver.in_udiff_.emplace_back(vs.var());
        }
        solver.udiff_[vs.var()] += diff;
    }

    //! Update watches and enque constraints.
    //!
    //! The parameters determine whether the lookup tables for lower or upper
    //! bounds are used.
    void update_constraints_(Solver &solver, var_t var, val_t diff) const {
        auto &watches = solver.var_watches_[var];
        watches.erase(std::remove_if(watches.begin(), watches.end(),
                                     [&](auto const &value_cs) {
                                         if (!value_cs.second->removable(level_)) {
                                             if (value_cs.second->update(value_cs.first, diff)) {
                                                 Level::mark_todo(solver, *value_cs.second);
                                             }
                                             return false;
                                         }
                                         solver.removed_var_watches_.emplace_back(var, value_cs.first, value_cs.second);
                                         return true;
                                     }),
                      watches.end());
    }

    //! Mark a constraint state as inactive.
    void mark_inactive(Solver &solver, AbstractConstraintState &cs) const {
        if (cs.removable() && !cs.marked_inactive()) {
            solver.inactive_.emplace_back(&cs);
            cs.mark_inactive(level_);
        }
    }

    //! Add the given constraint state to the todo list if it is not yet
    //! contained.
    static void mark_todo(Solver &solver, AbstractConstraintState &cs) {
        // Note: we have to enqueue only active constraints. When propagating
        // (and if enabled) the state of the constraint is checked. For
        // inactive constraints, the state is not updated once the watches were
        // removed. If we would enqueue an inactive constraint, then the state
        // check can fail.
        if (!cs.marked_inactive() && !cs.mark_todo(true)) {
            solver.todo_.emplace_back(&cs);
        }
    }

    //! This function undos decision level specific state.
    //!
    //! This includes undoing changed bounds of variables clearing constraints
    //! that where not propagated on the current decision level.
    void undo(Solver &solver) const {
        // undo lower bound changes
        for (auto it = solver.undo_lower_.begin() + static_cast<ptrdiff_t>(undo_lower_offset_),
                  ie = solver.undo_lower_.end();
             it != ie; ++it) {
            auto var = *it;
            auto &vs = solver.var_state(var);
            auto value = vs.lower_bound();
            vs.pop_lower();
            auto diff = value - vs.lower_bound() - solver.ldiff_[var];
            if (diff != 0) {
                for (auto &[co, cs] : solver.var_watches_[var]) {
                    cs->undo(co, diff);
                }
            }
            solver.ldiff_[var] = 0;
        }
        solver.undo_lower_.resize(undo_lower_offset_);
        solver.in_ldiff_.clear();

        // undo upper bound changes
        for (auto it = solver.undo_upper_.begin() + static_cast<ptrdiff_t>(undo_upper_offset_),
                  ie = solver.undo_upper_.end();
             it != ie; ++it) {
            auto var = *it;
            auto &vs = solver.var_state(var);
            auto value = vs.upper_bound();
            vs.pop_upper();
            auto diff = value - vs.upper_bound() - solver.udiff_[var];
            if (diff != 0) {
                for (auto &[co, cs] : solver.var_watches_[var]) {
                    cs->undo(co, diff);
                }
            }
            solver.udiff_[var] = 0;
        }
        solver.undo_upper_.resize(undo_upper_offset_);
        solver.in_udiff_.clear();

        // mark constraints as active again
        for (auto it = solver.inactive_.begin() + static_cast<ptrdiff_t>(inactive_offset_), ie = solver.inactive_.end();
             it != ie; ++it) {
            auto *cs = *it;
            cs->mark_active();
        }
        solver.inactive_.resize(inactive_offset_);

        // add removed watches
        for (auto it = solver.removed_var_watches_.begin() + static_cast<ptrdiff_t>(removed_var_watches_offset_),
                  ie = solver.removed_var_watches_.end();
             it != ie; ++it) {
            auto [var, val, cs] = *it;
            solver.var_watches_[var].emplace_back(val, cs);
        }
        solver.removed_var_watches_.resize(removed_var_watches_offset_);

        // clear remaining todo items
        for (auto *cs : solver.todo_) {
            cs->mark_todo(false);
        }
        solver.todo_.clear();
    }

    //! Remove the constraint state from the propagation state.
    void remove_constraint(Solver &solver, AbstractConstraintState &cs) const {
        static_cast<void>(level_);
        assert(level_ == 0);

        if (cs.marked_inactive()) {
            cs.mark_active();
            solver.inactive_.erase(std::find(solver.inactive_.begin(), solver.inactive_.end(), &cs));
        }
        if (cs.marked_todo()) {
            cs.mark_todo(false);
            solver.todo_.erase(std::find(solver.todo_.begin(), solver.todo_.end(), &cs));
        }
    }

    //! Remove a set of constraints from the propagation state.
    template <class F> void remove_constraints(Solver &solver, F in_removed) const {
        static_cast<void>(level_);
        assert(level_ == 0);

        for (auto &watches : solver.var_watches_) {
            watches.erase(std::remove_if(watches.begin(), watches.end(),
                                         [in_removed](auto &watch) { return in_removed(*watch.second); }),
                          watches.end());
        }

        solver.inactive_.erase(std::remove_if(solver.inactive_.begin(), solver.inactive_.end(),
                                              [in_removed](auto *cs) {
                                                  cs->mark_active();
                                                  return in_removed(*cs);
                                              }),
                               solver.inactive_.end());

        solver.todo_.erase(std::remove_if(solver.todo_.begin(), solver.todo_.end(),
                                          [in_removed](auto *cs) {
                                              cs->mark_todo(false);
                                              return in_removed(*cs);
                                          }),
                           solver.todo_.end());
    }

    //! Copy the given level.
    //!
    //! This function must only be called on the top level. It does not update
    //! variable and constraint states. This has to happen in
    //! Solver::copy_state.
    static void copy_state(Solver &solver, Solver const &master) {
        assert(solver.levels_.size() == 1 && master.levels_.size() == 1);

        auto &lvl = solver.levels_.front();
        auto const &lvl_master = master.levels_.front();

        // copy bound changes
        lvl.undo_lower_offset_ = lvl_master.undo_lower_offset_;
        solver.undo_lower_ = master.undo_lower_;
        solver.ldiff_ = master.ldiff_;
        solver.in_ldiff_ = master.in_ldiff_;

        lvl.undo_upper_offset_ = lvl_master.undo_upper_offset_;
        solver.undo_upper_ = master.undo_upper_;
        solver.udiff_ = master.udiff_;
        solver.in_udiff_ = master.in_udiff_;

        // copy inactive
        lvl.inactive_offset_ = lvl_master.inactive_offset_;
        solver.inactive_.clear();
        solver.inactive_.reserve(master.inactive_.size());
        for (auto *cs : master.inactive_) {
            solver.inactive_.emplace_back(&solver.constraint_state(cs->constraint()));
        }

        // copy watches
        solver.var_watches_ = master.var_watches_;
        for (auto &var_watches : solver.var_watches_) {
            for (auto &watch : var_watches) {
                watch.second = &solver.constraint_state(watch.second->constraint());
            }
        }

        // copy removed watches
        lvl.removed_var_watches_offset_ = lvl_master.removed_var_watches_offset_;
        solver.removed_var_watches_.clear();
        solver.removed_var_watches_.reserve(master.removed_var_watches_.size());
        for (auto const &[var, val, cs] : master.removed_var_watches_) {
            solver.removed_var_watches_.emplace_back(var, val, &solver.constraint_state(cs->constraint()));
        }

        // copy todo queue
        solver.todo_.clear();
        solver.todo_.reserve(master.todo_.size());
        for (auto const &cs : master.todo_) {
            solver.todo_.emplace_back(&solver.constraint_state(cs->constraint()));
        }
    }

  private:
    //! The associated decision level.
    level_t level_;
    size_t undo_lower_offset_;
    size_t undo_upper_offset_;
    size_t inactive_offset_;
    size_t removed_var_watches_offset_;
};

//! Helper class to efficiently handle order literal lookups
class Solver::LitmapEntry {
  public:
    LitmapEntry() : var_{0}, sign_{0} {}

    LitmapEntry(lit_t lit, var_t var, val_t value, lit_t prev, lit_t succ)
        : var_{var}, sign_{get_sign_(lit)}, value_{value}, prev_{prev}, succ_{succ} {
        assert(!invalid());
    }

    [[nodiscard]] auto valid(lit_t lit) const -> bool { return prev_ != 0 && get_sign_(lit) == sign_; }

    [[nodiscard]] auto invalid() const -> bool { return prev_ == 0; }

    [[nodiscard]] auto var() const -> var_t {
        assert(!invalid());
        return var_;
    }

    [[nodiscard]] auto value() const -> val_t {
        assert(!invalid());
        return value_;
    }

    [[nodiscard]] auto prev() const -> lit_t {
        assert(!invalid());
        return prev_;
    }

    void set_prev(lit_t prev) {
        assert(!invalid());
        prev_ = prev;
    }

    [[nodiscard]] auto succ() const -> lit_t {
        assert(!invalid());
        return succ_;
    }

    void set_succ(lit_t succ) {
        assert(!invalid());
        succ_ = succ;
    }

    void unset() { prev_ = 0; }

    [[nodiscard]] static auto map_offset(lit_t lit) -> size_t { return std::abs(lit) - 1; }

    [[nodiscard]] auto map_lit(size_t offset) const -> lit_t { return prev_ != 0 ? add_sign_(offset + 1) : 0; }

  private:
    [[nodiscard]] auto add_sign_(size_t offset) const -> val_t {
        return sign_ == 1 ? -static_cast<val_t>(offset) : static_cast<val_t>(offset);
    }
    [[nodiscard]] static auto get_sign_(lit_t lit) -> var_t { return lit > 0 ? 1 : 0; }

    var_t var_ : 31;
    var_t sign_ : 1;
    val_t value_{0};
    lit_t prev_{0};
    lit_t succ_{0};
};

Solver::Solver(SolverConfig const &config, SolverStatistics &stats) : config_{config}, stats_{stats} {
    levels_.emplace_back(*this, 0);
}

#ifdef _MSC_VER
Solver::Solver(Solver &&x) noexcept
    : config_{x.config_}, stats_{x.stats_}, var2vs_{std::move(x.var2vs_)}, levels_{std::move(x.levels_)},
      litmap_{std::move(x.litmap_)}, factmap_{std::move(x.factmap_)}, c2cs_{std::move(x.c2cs_)},
      var_watches_{std::move(x.var_watches_)}, udiff_{std::move(x.udiff_)}, in_udiff_{std::move(x.in_udiff_)},
      ldiff_{std::move(x.ldiff_)}, in_ldiff_{std::move(x.in_ldiff_)}, todo_{std::move(x.todo_)},
      lit2cs_{std::move(x.lit2cs_)}, temp_reason_{std::move(x.temp_reason_)}, split_last_{x.split_last_},
      trail_offset_{x.trail_offset_}, minimize_bound_{std::move(x.minimize_bound_)},
      minimize_level_{x.minimize_level_} {}
#else
Solver::Solver(Solver &&x) noexcept = default;
#endif

Solver::~Solver() = default;

void Solver::shrink_to_fit() {
    var2vs_.shrink_to_fit();
    litmap_.shrink_to_fit();
    factmap_.shrink_to_fit();
    c2cs_.rehash(0);
    for (auto &watches : var_watches_) {
        watches.shrink_to_fit();
    }
    var_watches_.shrink_to_fit();
    udiff_.shrink_to_fit();
    ldiff_.shrink_to_fit();
    lit2cs_.rehash(0);
}

void Solver::copy_state(Solver const &master) {
    // just to be thorough
    split_last_ = master.split_last_;
    trail_offset_ = master.trail_offset_;
    minimize_level_ = master.minimize_level_;
    minimize_bound_ = master.minimize_bound_;

    // copy var states and lookups
    var2vs_ = master.var2vs_;
    factmap_ = master.factmap_;
    litmap_ = master.litmap_;

    // copy constraint states and lookups
    c2cs_.clear();
    lit2cs_.clear();
    c2cs_.reserve(master.c2cs_.size());
    lit2cs_.reserve(master.lit2cs_.size());
    for (auto const &[c, cs] : master.c2cs_) {
        auto ret = c2cs_.emplace(c, cs->copy());
        lit2cs_.emplace(c->literal(), ret.first->second.get());
    }

    // adjust levels
    Level::copy_state(*this, master);
}

auto Solver::add_variable(val_t min_int, val_t max_int) -> var_t {
    var_t idx = var2vs_.size();
    var2vs_.emplace_back(idx, min_int, max_int);
    var_watches_.emplace_back();
    ldiff_.emplace_back(0);
    udiff_.emplace_back(0);
    return idx;
}

auto Solver::minimize_bound() const -> std::optional<sum_t> { return minimize_bound_; }

void Solver::update_minimize(AbstractConstraint &constraint, level_t level, sum_t bound) {
    if (!minimize_bound_.has_value() || bound < *minimize_bound_) {
        minimize_bound_ = bound;
        minimize_level_ = level;
        Level::mark_todo(*this, constraint_state(constraint));
    } else if (level < minimize_level_) {
        minimize_level_ = level;
        Level::mark_todo(*this, constraint_state(constraint));
    }
}

auto Solver::get_value(var_t var) const -> val_t { return var2vs_[var].lower_bound(); }

auto Solver::level_() -> Solver::Level & {
    assert(!levels_.empty());
    return levels_.back();
}

void Solver::push_level_(level_t level) {
    assert(!levels_.empty());
    if (levels_.back().level() < level) {
        levels_.emplace_back(*this, level);
    }
}

auto Solver::litmap_at_(lit_t lit) -> Solver::LitmapEntry & {
    static LitmapEntry invalid;
    auto offset{LitmapEntry::map_offset(lit)};
    return offset < litmap_.size() ? litmap_[offset] : invalid;
}

void Solver::litmap_add_(VarState &vs, val_t val, lit_t lit) {
    size_t offset{LitmapEntry::map_offset(lit)};
    if (offset >= litmap_.size()) {
        litmap_.resize(offset + 1);
    }
    auto ps = update_litmap_(vs, lit, val);
    litmap_[offset] = LitmapEntry{lit, vs.var(), val, ps.first, ps.second};
}

auto Solver::get_literal(AbstractClauseCreator &cc, VarState &vs, val_t value) -> lit_t {
    if (value < vs.min_bound()) {
        return -TRUE_LIT;
    }
    if (value >= vs.max_bound()) {
        return TRUE_LIT;
    }
    auto &lit = vs.get_or_add_literal(value);
    if (lit == 0) {
        lit = cc.add_literal();
        // Note: By default clasp's heuristic makes literals false. By flipping
        // the literal for non-negative values, assignments close to zero are
        // preferred. This way, we might get solutions with small numbers
        // first.
        if (value >= config().sign_value) {
            lit = -lit;
        }
        litmap_add_(vs, value, lit);
        cc.add_watch(lit);
        cc.add_watch(-lit);
    }
    return lit;
}

auto Solver::update_litmap_(VarState &vs, lit_t lit, val_t value) -> std::pair<lit_t, lit_t> {
    std::pair<lit_t, lit_t> ret{-TRUE_LIT, TRUE_LIT};
    if (auto prev = vs.lit_lt(value); prev != 0) {
        ret.first = prev;
        if (auto &olit = litmap_at_(prev); olit.valid(prev)) {
            olit.set_succ(lit != 0 ? lit : vs.lit_succ(value));
        }
    }
    if (auto succ = vs.lit_gt(value); succ != 0) {
        ret.second = succ;
        if (auto &olit = litmap_at_(succ); olit.valid(succ)) {
            olit.set_prev(lit != 0 ? lit : vs.lit_prev(value));
        }
    }
    return ret;
}

auto Solver::update_literal(AbstractClauseCreator &cc, VarState &vs, val_t value, Clingo::TruthValue truth) -> lit_t {
    // order literals can only be update on level 0
    if (truth == Clingo::TruthValue::Free || cc.assignment().decision_level() > 0) {
        return get_literal(cc, vs, value);
    }
    // the value is out of bounds
    if (value < vs.min_bound()) {
        return -TRUE_LIT;
    }
    if (value >= vs.max_bound()) {
        return TRUE_LIT;
    }
    auto &old = vs.get_or_add_literal(value);
    // there was no literal yet
    if (old == 0) {
        old = truth == Clingo::TruthValue::True ? TRUE_LIT : -TRUE_LIT;
        auto ps = update_litmap_(vs, old, value);
        factmap_.emplace_back(old, vs.var(), value, truth == Clingo::TruthValue::True ? ps.second : ps.first);
    }
    // we keep the literal
    return old;
}

void Solver::add_var_watch(var_t var, val_t i, AbstractConstraintState &cs) {
    assert(var < var_watches_.size());
    var_watches_[var].emplace_back(i, &cs);
}

void Solver::remove_var_watch(var_t var, val_t i, AbstractConstraintState &cs) {
    assert(var < var_watches_.size());
    auto &watches = var_watches_[var];
    watches.erase(std::find(watches.begin(), watches.end(), std::pair(i, &cs)));
}

void Solver::mark_inactive(AbstractConstraintState &cs) { level_().mark_inactive(*this, cs); }

auto Solver::add_constraint(AbstractConstraint &constraint) -> AbstractConstraintState & {
    auto &cs = c2cs_.emplace(&constraint, std::unique_ptr<AbstractConstraintState>{nullptr}).first->second;

    if (cs == nullptr) {
        cs = constraint.create_state();
        lit2cs_.emplace(constraint.literal(), cs.get());
        cs->attach(*this);
        Level::mark_todo(*this, *cs);
    }

    return *cs;
}

void Solver::remove_constraint(AbstractConstraint &constraint) {
    auto lit = constraint.literal();
    auto it = c2cs_.find(&constraint);
    auto &cs = *it->second;
    cs.detach(*this);

    for (auto it = lit2cs_.find(lit), ie = lit2cs_.end(); it != ie && it->first == lit; ++it) {
        if (it->second == &cs) {
            lit2cs_.erase(it);
            break;
        }
    }

    level_().remove_constraint(*this, cs);
    c2cs_.erase(it);
}

auto Solver::translate(InitClauseCreator &cc, Statistics &stats, Config const &conf, ConstraintVec &constraints)
    -> bool {
    size_t jdx = 0, kdx = constraints.size(); // NOLINT
    for (size_t idx = jdx; idx < constraints.size(); ++idx) {
        auto &cs = add_constraint(*constraints[idx]);
        if (idx >= kdx) {
            ++stats.num_constraints;
            ++stats.translate_added;
        }
        auto ret = cs.translate(conf, *this, cc, constraints);
        if (!ret.first) {
            return false;
        }
        if (ret.second) {
            --stats.num_constraints;
            ++stats.translate_removed;
        } else {
            if (idx != jdx) {
                std::swap(constraints[idx], constraints[jdx]);
            }
            ++jdx;
        }
    }

    // Note: Constraints are removed by traversing the whole lookup table to
    // avoid potentially quadratic overhead if a large number of constraints
    // has to be removed.
    if (jdx < constraints.size()) {
        std::sort(constraints.begin() + static_cast<ptrdiff_t>(jdx), constraints.end());
        auto in_removed = [jdx, &constraints](AbstractConstraintState &cs) {
            struct {
                auto operator()(UniqueConstraint const &a, AbstractConstraint const *b) -> bool { return a.get() < b; }
                auto operator()(AbstractConstraint const *b, UniqueConstraint const &a) -> bool { return b < a.get(); }
            } pred;
            return std::binary_search(constraints.begin() + static_cast<ptrdiff_t>(jdx), constraints.end(),
                                      &cs.constraint(), pred);
        };

        level_().remove_constraints(*this, in_removed);

        for (auto it = lit2cs_.begin(); it != lit2cs_.end();) {
            if (in_removed(*it->second)) {
                it = lit2cs_.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = constraints.begin() + static_cast<ptrdiff_t>(jdx), ie = constraints.end(); it != ie; ++it) {
            c2cs_.erase(it->get());
        }
    }

    constraints.erase(constraints.begin() + static_cast<ptrdiff_t>(jdx), constraints.end());

    // This readds binary clauses when multishot-solving. Probably clasp can
    // handle this.
    if (conf.add_order_clauses) {
        for (auto &vs : var2vs_) {
            if (!vs.with([&](auto ib, auto ie, auto get_lit, auto, auto inc) { // NOLINT(readability-named-parameter)
                    lit_t prev = -TRUE_LIT;
                    for (auto it = ib; it != ie; inc(it)) {
                        auto lit = get_lit(it);
                        // lit<=val-1  => lit<=val
                        if (prev != -TRUE_LIT && !cc.add_clause({-prev, lit})) {
                            return false;
                        }
                        prev = lit;
                    }
                    return true;
                })) {
                return false;
            }
        }
    }

    return true;
}

auto Solver::simplify(AbstractClauseCreator &cc, bool check_state) -> bool {
    auto ass = cc.assignment();
    auto trail = ass.trail();

    // Note: The initial propagation below, will not introduce any order
    // literals other than true or false.
    while (true) {
        if (!cc.propagate()) {
            return false;
        }

        auto trail_offset = trail.size();
        if (trail_offset_ == trail_offset && todo_.empty()) {
            return true;
        }

        if (!propagate_(cc, trail.begin() + static_cast<int32_t>(trail_offset_),
                        trail.begin() + static_cast<int32_t>(trail_offset))) {
            return false;
        }
        trail_offset_ = trail_offset;

        if (!check(cc, check_state)) {
            return false;
        }
    }
}

[[nodiscard]] auto Solver::propagate(AbstractClauseCreator &cc, Clingo::LiteralSpan changes) -> bool {
    return propagate_(cc, changes.begin(), changes.end());
}

template <class It> [[nodiscard]] auto Solver::propagate_(AbstractClauseCreator &cc, It begin, It end) -> bool {
    Timer timer{stats_.time_propagate};

    auto ass = cc.assignment();

    // open a new decision level if necessary
    push_level_(ass.decision_level());

    // propagate order literals that became true/false
    for (auto it = begin; it != end; ++it) {
        if (!propagate_(cc, *it)) {
            return false;
        }
    }

    return true;
}

auto Solver::propagate_(AbstractClauseCreator &cc, lit_t lit) -> bool {
    for (auto it = lit2cs_.find(lit), ie = lit2cs_.end(); it != ie && it->first == lit; ++it) {
        Level::mark_todo(*this, *it->second);
    }
    return update_domain_(cc, lit);
}

template <int sign, class It, class L, class I>
auto Solver::propagate_variables_(AbstractClauseCreator &cc, lit_t reason_lit, It begin, It end, L get_lit, I inc)
    -> bool {
    auto ass = cc.assignment();

    for (; begin != end; inc(begin)) {
        auto lit = sign * get_lit(begin);
        if (ass.is_true(lit)) {
            break;
        }
        if (!cc.add_clause({-reason_lit, lit},
                           reason_lit != TRUE_LIT ? Clingo::ClauseType::Learnt : Clingo::ClauseType::Static)) {
            return false;
        }
        // Note: Literal reason_lit is already guaranteed to be a fact on level 0.
        if (config_.propagate_chain && ass.decision_level() > 0) {
            reason_lit = lit;
        }
    }

    return true;
}

auto Solver::update_upper_(Level &lvl, AbstractClauseCreator &cc, var_t var, lit_t lit, val_t value, lit_t succ_lit)
    -> bool {
    auto ass = cc.assignment();
    auto &vs = var_state(var);
    // Note: This keeps the state consistent.
    if (value < vs.lower_bound()) {
        static_cast<void>(cc.add_clause({get_literal(cc, vs, vs.lower_bound() - 1), -lit}) && cc.propagate());
        return false;
    }
    if (vs.upper_bound() > value) {
        lvl.update_upper(*this, vs, value);
    }
    assert(vs.lower_bound() <= vs.upper_bound());
    return ass.is_true(succ_lit) || vs.with_gt(value, [&](auto it, auto ie, auto &&get_lit, auto get_val, auto &&inc) {
        static_cast<void>(get_val);
        return propagate_variables_<1>(cc, lit, it, ie, std::forward<decltype(get_lit)>(get_lit),
                                       std::forward<decltype(inc)>(inc));
    });
}

auto Solver::update_lower_(Level &lvl, AbstractClauseCreator &cc, var_t var, lit_t lit, val_t value, lit_t prev_lit)
    -> bool {
    auto ass = cc.assignment();
    auto &vs = var_state(var);
    // Note: This keeps the state consistent.
    if (vs.upper_bound() < value + 1) {
        static_cast<void>(cc.add_clause({-get_literal(cc, vs, vs.upper_bound()), -lit}) && cc.propagate());
        return false;
    }
    if (vs.lower_bound() < value + 1) {
        lvl.update_lower(*this, vs, value);
    }
    assert(vs.lower_bound() <= vs.upper_bound());
    return ass.is_true(-prev_lit) || vs.with_lt(value, [&](auto it, auto ie, auto &&get_lit, auto get_val, auto &&inc) {
        static_cast<void>(get_val);
        return propagate_variables_<-1>(cc, lit, it, ie, std::forward<decltype(get_lit)>(get_lit),
                                        std::forward<decltype(inc)>(inc));
    });
}

auto Solver::update_domain_(AbstractClauseCreator &cc, lit_t lit) -> bool {
    auto &lvl = level_();
    auto ass = cc.assignment();
    assert(ass.is_true(lit));

    // On-the-fly simplification on the top-level.
    if (lit != TRUE_LIT && ass.decision_level() == 0 && ass.is_fixed(lit)) {
        if (auto &olit = litmap_at_(lit); olit.valid(lit)) {
            auto &vs = var_state(olit.var());
            assert(vs.get_literal(olit.value()) == lit);
            vs.set_literal(olit.value(), TRUE_LIT);
            update_litmap_(vs, TRUE_LIT, olit.value());
            factmap_.emplace_back(TRUE_LIT, olit.var(), olit.value(), olit.succ());
            olit.unset();
        }
        if (auto &olit = litmap_at_(-lit); olit.valid(-lit)) {
            auto &vs = var_state(olit.var());
            assert(vs.get_literal(olit.value()) == -lit);
            vs.set_literal(olit.value(), -TRUE_LIT);
            update_litmap_(vs, -TRUE_LIT, olit.value());
            factmap_.emplace_back(-TRUE_LIT, olit.var(), olit.value(), olit.prev());
            olit.unset();
        }
        lit = TRUE_LIT;
    }

    // Note: Neither factmap_ nor litmap_ will be modified by update_lower_ /
    // update_upper_.

    // Fact propagation. (Could also be put in the litmap...)
    if (lit == TRUE_LIT) {
        assert(ass.decision_level() == 0);
        for (auto [fact_lit, var, value, prec_lit] : factmap_) {
            auto &vs = var_state(var);
            if (fact_lit == TRUE_LIT) {
                if (!update_upper_(lvl, cc, var, TRUE_LIT, value, prec_lit)) {
                    return false;
                }
                assert(vs.get_literal(value) == TRUE_LIT);
                update_litmap_(vs, 0, value);
                vs.unset_literal(value);
            } else {
                if (!update_lower_(lvl, cc, var, TRUE_LIT, value, prec_lit)) {
                    return false;
                }
                assert(vs.get_literal(value) == -TRUE_LIT);
                update_litmap_(vs, 0, value);
                vs.unset_literal(value);
            }
        }
        factmap_.clear();
        return true;
    }

    if (auto const &olit = litmap_at_(lit);
        olit.valid(lit) && !update_upper_(lvl, cc, olit.var(), lit, olit.value(), olit.succ())) {
        return false;
    }
    if (auto const &olit = litmap_at_(-lit);
        olit.valid(-lit) && !update_lower_(lvl, cc, olit.var(), lit, olit.value(), olit.prev())) {
        return false; // NOLINT
    }

    return true;
}

auto Solver::check(AbstractClauseCreator &cc, bool check_state) -> bool {
    Timer timer(stats_.time_check);

    auto ass = cc.assignment();
    auto &lvl = level_();

    // Note: Most of the time check has to be called only for levels that have
    // also been propagated. The exception is if a minimize constraint has to
    // be integrated when backtracking from a bound update.
    if (ass.decision_level() != lvl.level() && lvl.level() >= minimize_level_) {
        return true;
    }

    // Note: We have to loop here because watches for the true/false literals
    // do not fire again.
    while (true) {
        // Note: This integrates any facts that have not been integrated yet on
        // the top level.
        if (!factmap_.empty()) {
            assert(ass.decision_level() == 0);
            if (!update_domain_(cc, TRUE_LIT)) {
                return false;
            }
        }

        // update the bounds of the constraints (this is the only place where
        // the todo queue is filled after initializaton)
        for (auto var : in_udiff_) {
            lvl.update_constraints_(*this, var, udiff_[var]);
            udiff_[var] = 0;
        }
        in_udiff_.clear();
        for (auto var : in_ldiff_) {
            lvl.update_constraints_(*this, var, ldiff_[var]);
            ldiff_[var] = 0;
        }
        in_ldiff_.clear();

        // propagate affected constraints
        bool ret{true};
        for (auto *cs : todo_) {
            cs->mark_todo(false);
            if (!ret) {
                continue;
            }

            if (!ass.is_false(cs->constraint().literal())) {
                if (!cs->propagate(*this, cc, check_state)) {
                    ret = false;
                }
            } else {
                lvl.mark_inactive(*this, *cs);
            }
        }
        todo_.clear();

        if (!ret || factmap_.empty()) {
            return ret;
        }
    }
}

void Solver::undo() {
    Timer timer{stats_.time_undo};

    auto &lvl = level_();
    assert(lvl.level() != 0);

    lvl.undo(*this);

    levels_.pop_back();
    assert(!levels_.empty());
}

auto Solver::decide(Clingo::Assignment const &assign, lit_t fallback) -> lit_t {
    static_cast<void>(assign);
    switch (config_.heuristic) {
        case Heuristic::None: {
            break;
        }
        case Heuristic::MaxChain: {
            if (auto const &olit = litmap_at_(fallback); olit.valid(fallback)) {
                auto &vs = var_state(olit.var());
                // make the literal as small as possible
                auto lit = vs.lit_ge(vs.lower_bound());
                assert(assign.truth_value(lit) == Clingo::TruthValue::Free);
                return lit;
            }
            if (auto const &olit = litmap_at_(-fallback); olit.valid(-fallback)) {
                auto &vs = var_state(olit.var());
                // make the literal as large as possible
                auto lit = -vs.lit_lt(vs.upper_bound());
                assert(assign.truth_value(lit) == Clingo::TruthValue::Free);
                return lit;
            }
            break;
        }
    }
    return fallback;
}

void Solver::check_full(AbstractClauseCreator &cc, bool check_solution) {
    auto split = [&](VarState &vs) {
        if (!vs.is_assigned()) {
            auto value = midpoint(vs.lower_bound(), vs.upper_bound());
            static_cast<void>(get_literal(cc, vs, value));
            return true;
        }
        return false;
    };

    if (config_.split_all) {
        bool res{false};
        for (auto &vs : var2vs_) {
            res = split(vs) || res;
        }
        if (res) {
            return;
        }
    } else {
        auto ib = var2vs_.begin();
        auto im = ib + split_last_;
        auto ie = var2vs_.end();

        auto split_once = [&](auto it) {
            if (split(*it)) {
                split_last_ = it - ib;
                return true;
                ;
            }
            return false;
        };

        for (auto it = im; it != ie; ++it) {
            if (split_once(it)) {
                return;
            }
        }
        for (auto it = ib; it != im; ++it) {
            if (split_once(it)) {
                return;
            }
        }
    }

    if (check_solution) {
        auto ass = cc.assignment();
        for (auto [lit, cs] : lit2cs_) {
            if (ass.is_true(lit)) {
                cs->check_full(*this);
            }
        }
    }
}

void Solver::update() {
    // reset minimize state
    minimize_bound_.reset();
    minimize_level_ = 0;

    // remove solve step local variables from litmap_
    size_t offset = 0;
    for (auto &olit : litmap_) {
        if (auto var = static_cast<var_t>(std::abs(olit.map_lit(offset))); var != 0 && var > max_static_var_) {
            auto &vs = var_state(olit.var());
            vs.unset_literal(olit.value());
            update_litmap_(vs, 0, olit.value());
            olit.unset();
        }
        ++offset;
    }
}

auto Solver::update_bounds(AbstractClauseCreator &cc, Solver &other, bool check_state) -> bool {
    auto it = var2vs_.begin();
    for (auto &vs_other : other.var2vs_) {
        auto &vs = *it++;

        // update upper bounds
        if (vs_other.upper_bound() < vs.upper_bound()) {
            auto lit = update_literal(cc, vs, vs_other.upper_bound(), Clingo::TruthValue::True);
            if (!cc.add_clause({lit})) {
                return false;
            }
        }

        // update lower bounds
        if (vs.lower_bound() < vs_other.lower_bound()) {
            auto lit = update_literal(cc, vs, vs_other.lower_bound() - 1, Clingo::TruthValue::False);
            if (!cc.add_clause({-lit})) {
                return false;
            }
        }
    }

    // update_domain_ in check makes sure that unnecassary facts are removed
    return check(cc, check_state);
}

auto Solver::add_dom(AbstractClauseCreator &cc, lit_t lit, var_t var, IntervalSet<val_t> const &domain) -> bool {
    auto ass = cc.assignment();
    if (ass.is_false(lit)) {
        return true;
    }
    if (ass.is_true(lit)) {
        lit = TRUE_LIT;
    }
    auto &vs = var_state(var);

    std::optional<val_t> py;
    for (auto [x, y] : domain) {
        auto ly = py.has_value() ? -get_literal(cc, vs, *py - 1) : TRUE_LIT;
        auto truth = Clingo::TruthValue::Free;
        if (lit == TRUE_LIT && ass.is_true(ly)) {
            truth = Clingo::TruthValue::False;
        }
        auto lx = update_literal(cc, vs, x - 1, truth);
        if (!cc.add_clause({-lit, -ly, -lx})) {
            return false;
        }
        py = y;
    }

    std::optional<val_t> px;
    for (auto it = domain.rbegin(), ie = domain.rend(); it != ie; ++it) {
        auto [x, y] = *it;
        auto lx = px.has_value() ? get_literal(cc, vs, *px - 1) : TRUE_LIT;
        auto truth = Clingo::TruthValue::Free;
        if (lit == TRUE_LIT && ass.is_true(lx)) {
            truth = Clingo::TruthValue::True;
        }
        auto ly = update_literal(cc, vs, y - 1, truth);
        if (!cc.add_clause({-lit, -lx, ly})) {
            return false;
        }
        px = x;
    }

    return true;
}

auto Solver::add_simple(AbstractClauseCreator &cc, lit_t clit, val_t co, var_t var, val_t rhs, bool strict) -> bool {
    auto ass = cc.assignment();

    // the constraint is never propagated
    if (!strict && ass.is_false(clit)) {
        return true;
    }

    auto &vs = var_state(var);

    Clingo::TruthValue truth{Clingo::TruthValue::Free};
    val_t value{0};
    if (co > 0) {
        truth = ass.truth_value(clit);
        value = floordiv(rhs, co);
    } else {
        truth = ass.truth_value(-clit);
        value = -floordiv(rhs, -co) - 1;
    }

    // in this case we can use the literal of the constraint as order variable
    if (strict && vs.min_bound() <= value && value < vs.max_bound() && !vs.has_literal(value)) {
        auto lit = clit;
        if (co < 0) {
            lit = -lit;
        }
        if (truth == Clingo::TruthValue::Free) {
            if (auto const &olit = litmap_at_(lit); !olit.invalid()) {
                auto old = lit;
                lit = cc.add_literal();
                if (!cc.add_clause({-old, lit}) || !cc.add_clause({-lit, old})) {
                    return false;
                }
            }
            cc.add_watch(lit);
            cc.add_watch(-lit);
            litmap_add_(vs, value, lit);
        } else {
            lit = truth == Clingo::TruthValue::True ? TRUE_LIT : -TRUE_LIT;
            ;
            auto ps = update_litmap_(vs, lit, value);
            factmap_.emplace_back(lit, vs.var(), value, truth == Clingo::TruthValue::True ? ps.second : ps.first);
        }
        vs.set_literal(value, lit);
    }
    // otherwise we just update the existing order literal
    else {
        auto lit = update_literal(cc, vs, value, truth);
        if (co < 0) {
            lit = -lit;
        }
        if (!cc.add_clause({-clit, lit})) {
            return false;
        }
        if (strict && !cc.add_clause({-lit, clit})) {
            return false;
        }
    }

    return true;
}

#if 0
// This is a usefull function for debugging.
void Solver::check_litmap_() {
    for (auto [lit, tup] : litmap_) {
        auto [var, value, prev_lit, succ_lit] = tup;
        auto &vs = var_state(var);
        if (auto it = vs.lit_lt(value); it != vs.rend()) {
            assert(it->second == prev_lit);
        }
        else {
            assert(prev_lit == -TRUE_LIT);
        }
        if (auto it = vs.lit_gt(value); it != vs.end()) {
            assert(it->second == succ_lit);
        }
        else {
            assert(succ_lit == TRUE_LIT);
        }
    }
    for (auto [lit, var, value, prec_lit] : factmap_) {
        auto &vs = var_state(var);
        if (lit == -TRUE_LIT) {
            if (auto it = vs.lit_lt(value); it != vs.rend()) {
                assert(it->second == prec_lit);
            }
            else {
                assert(prec_lit == -TRUE_LIT);
            }
        }
        if (lit == TRUE_LIT) {
            if (auto it = vs.lit_gt(value); it != vs.end()) {
                assert(it->second == prec_lit);
            }
            else {
                assert(prec_lit == TRUE_LIT);
            }
        }
    }
}
#endif

} // namespace Clingcon
