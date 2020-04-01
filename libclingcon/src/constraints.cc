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

#include "clingcon/constraints.hh"

namespace Clingcon {

namespace {

//! Implements propagation for sum and minimize constraints.
template <bool tagged, typename T, typename C>
class SumConstraintStateImpl final : public T {
public:
    SumConstraintStateImpl(C &constraint)
    : T{constraint} {
    }

    C &constraint() override {
        return T::constraint_;
    }

    [[nodiscard]] lit_t literal() const {
        return T::constraint_.literal();
    }

    bool mark_todo(bool todo) override {
        auto ret = T::todo_;
        T::todo_ = todo;
        return ret;
    }

    [[nodiscard]] bool marked_todo() const override {
        return T::todo_;
    }

    [[nodiscard]] level_t inactive_level() const override {
        return T::inactive_level_;
    }

    void inactive_level(level_t level) override {
        T::inactive_level_ = level;
    }

    void attach(Solver &solver) override {
        T::lower_bound_ = T::upper_bound_ = 0;
        for (auto [co, var] : T::constraint_) {
            auto &vs = solver.var_state(var);
            solver.add_var_watch(var, co, *this);
            if (co > 0) {
                T::lower_bound_ += static_cast<sum_t>(vs.lower_bound()) * co;
                T::upper_bound_ += static_cast<sum_t>(vs.upper_bound()) * co;
            }
            else {
                T::lower_bound_ += static_cast<sum_t>(vs.upper_bound()) * co;
                T::upper_bound_ += static_cast<sum_t>(vs.lower_bound()) * co;
            }
        }
    }

    void detach(Solver &solver) override {
        for (auto [co, var] : T::constraint_) {
            solver.remove_var_watch(var, co, *this);
        }
    }

    void undo(val_t i, val_t diff) override {
        sum_t x = static_cast<sum_t>(i) * diff;
        if (x > 0) {
            T::lower_bound_ -= x;
        }
        else {
            T::upper_bound_ -= x;
        }
    }

    [[nodiscard]] bool update(val_t i, val_t diff) override {
        sum_t x = static_cast<sum_t>(i) * diff;
        assert(x != 0);
        if (x < 0) {
            T::upper_bound_ += x;
            return false;
        }
        T::lower_bound_ += x;
        return true;
    }

    void check_state(Solver &solver) {
        sum_t lower = 0;
        sum_t upper = 0;
        for (auto [co, var] : T::constraint_) {
            auto &vs = solver.var_state(var);
            if (co > 0) {
                lower += static_cast<sum_t>(co) * vs.lower_bound();
                upper += static_cast<sum_t>(co) * vs.upper_bound();
            }
            else {
                lower += static_cast<sum_t>(co) * vs.upper_bound();
                upper += static_cast<sum_t>(co) * vs.lower_bound();
            }
        }
        if (lower != T::lower_bound_) {
            throw std::logic_error("invalid lower bound");
        }
        if (upper != T::upper_bound_) {
            throw std::logic_error("invalid lower bound");
        }
        if (lower > upper) {
            throw std::logic_error("lower bound exceeds upper bound");
        }
    }

    void check_full(Solver &solver) override {
        if (!T::has_rhs(solver)) {
            return;
        }
        auto rhs = T::rhs(solver);

        sum_t lhs = 0;
        for (auto [co, var] : T::constraint_) {
            auto &vs = solver.var_state(var);
            if (!vs.is_assigned()) {
                throw std::logic_error("variable is not assigned");
            }
            lhs += static_cast<sum_t>(co) * vs.lower_bound();
        }

        if (T::marked_inactive()) {
            if (lhs > T::upper_bound_) {
                throw std::logic_error("invalid solution");
            }
        }
        else {
            if (lhs != T::upper_bound_) {
                throw std::logic_error("invalid solution");
            }
            if (lhs > T::lower_bound_) {
                throw std::logic_error("invalid solution");
            }
        }

        if (lhs > rhs) {
            throw std::logic_error("invalid solution");
        }
    }

    std::pair<bool, lit_t> calculate_reason(Solver &solver, AbstractClauseCreator &cc, sum_t &slack, VarState &vs, val_t co) {
        auto ass = cc.assignment();
        uint64_t found = 0;
        lit_t lit{0};
        bool ret = true;

        if (co > 0) {
            sum_t current = vs.lower_bound();
            // the direct reason literal
            lit = solver.get_literal(cc, vs, current-1);
            assert (ass.is_false(lit));
            if (solver.config().refine_reasons && slack + co < 0 && ass.decision_level() > 0) {
                auto delta = -floordiv<sum_t>(slack + 1, -co);
                auto value = std::max<sum_t>(current + delta, vs.min_bound());
                if (value < current) {
                    // refine reason literal
                    if (auto it = vs.lit_ge(value - 1); it != vs.end()) {
                        if (auto [value_ge, lit_ge] = *it; value_ge + 1 < current) {
                            found = 1;
                            slack -= static_cast<sum_t>(co) * (value_ge + 1 - current);
                            current = value_ge + 1;
                            assert(slack < 0);
                            lit = lit_ge;
                            assert(ass.is_false(lit));
                        }

                    }
                    // introduce reason literal
                    // Note: It is important to imply literals by the smallest
                    // available literal to keep the state consistent.
                    // Furthermore, we only introduce literals implied on the
                    // current decision level to avoid backtracking.
                    if (solver.config().refine_introduce && ass.level(lit) == ass.decision_level() && value < current) {
                        ++solver.statistics().introduced_reason;
                        found = 1;
                        slack -= static_cast<sum_t>(co) * (value - current);
                        assert(slack < 0);
                        auto refined = solver.get_literal(cc, vs, value-1);
                        assert(!ass.is_true(refined));
                        ret = ass.is_false(refined) || cc.add_clause({lit, -refined});
                        lit = refined;
                    }
                }
            }
        }
        else {
            // symmetric case
            auto current = vs.upper_bound();
            lit = -solver.get_literal(cc, vs, current);
            assert(ass.is_false(lit));
            if (solver.config().refine_reasons && slack - co < 0 && ass.decision_level() > 0) {
                auto delta = floordiv<sum_t>(slack + 1, co);
                auto value = std::min<sum_t>(current + delta, vs.max_bound());
                if (value > current) {
                    // refine reason literal
                    if (auto it = vs.lit_le(value); it != vs.rend()) {
                        if (auto [value_le, lit_le] = *it; value_le > current) {
                            found = 1;
                            slack -= static_cast<sum_t>(co) * (value_le - current);
                            current = value_le;
                            assert(slack < 0);
                            lit = -lit_le;
                            assert(ass.is_false(lit));
                        }

                    }
                    // introduce reason literal
                    if (solver.config().refine_introduce && ass.level(lit) == ass.decision_level() && value > current) {
                        ++solver.statistics().introduced_reason;
                        found = 1;
                        slack -= static_cast<sum_t>(co) * (value - current);
                        assert(slack < 0);
                        auto refined = -solver.get_literal(cc, vs, value);
                        assert(!ass.is_true(refined));
                        ret = ass.is_false(refined) || cc.add_clause({lit, -refined});
                        lit = refined;
                    }
                }
            }
        }

        solver.statistics().refined_reason += found;
        assert(!ret || ass.is_false(lit));
        return {ret, lit};
    }

    //! This function propagates a constraint that became active because its
    //! associated literal became true or because the bound of one of its
    //! variables changed.
    //!
    //! The function calculates the slack of the constraint w.r.t. to the
    //! lower/upper bounds of its values. The order values are then propagated
    //! in such a way that the slack is non-negative. The trick here is that we
    //! can use the ordering of variables to restrict the number of
    //! propagations. For example, for positive coefficients, we just have to
    //! enforce the smallest order variable that would make the slack
    //! non-negative.
    //!
    //! The function returns False if propagation fails, True otherwise.
    [[nodiscard]] bool propagate(Solver &solver, AbstractClauseCreator &cc, bool check_state) override {
        if (!T::has_rhs(solver)) {
            return true;
        }
        auto ass = cc.assignment();
        auto rhs = T::rhs(solver);
        auto clit = literal();

        // Note: this has a noticible cost because of the shortcuts below
        if (check_state) {
            this->check_state(solver);
        }
        assert(!ass.is_false(clit));

        // skip constraints that cannot become false
        if (T::upper_bound_ <= rhs) {
            solver.mark_inactive(*this);
            return true;
        }
        auto slack = rhs - T::lower_bound_;

        // this is necessary to correctly handle empty constraints (and do
        // propagation of false constraints)
        if (slack < 0) {
            auto &reason = solver.temp_reason();

            // add reason literals
            for (auto [co, var] : T::constraint_) {
                auto &vs = solver.var_state(var);

                // calculate reason literal
                auto [ret, lit] = calculate_reason(solver, cc, slack, vs, co);
                if (!ret) {
                    return false;
                }

                // append the reason literal
                if (!ass.is_fixed(lit)) {
                    reason.emplace_back(lit);
                }
            }

            // append the consequence
            reason.emplace_back(-clit);

            solver.mark_inactive(*this);
            return cc.add_clause(reason, tagged ? Clingo::ClauseType::Volatile : Clingo::ClauseType::Learnt);
        }

        if (!ass.is_true(clit)) {
            return true;
        }
        for (auto [co_r, var_r] : T::constraint_) {
            auto &vs_r = solver.var_state(var_r);
            lit_t lit_r = 0;
            sum_t delta_r = 0;
            sum_t value_r = 0;

            // calculate the first value that would violate the constraint
            if (co_r > 0) {
                delta_r = -floordiv<sum_t>(slack + 1, -co_r);
                value_r = vs_r.lower_bound() + delta_r;
                assert (slack - co_r * delta_r < 0 && 0 <= slack - co_r * (delta_r - 1));
                // values above the upper bound are already true;
                if (value_r > vs_r.upper_bound()) {
                    continue;
                }
                // get the literal of the value;
                if (vs_r.has_literal(value_r - 1)) {
                    lit_r = solver.get_literal(cc, vs_r, value_r - 1);
                }
            }
            else {
                delta_r = floordiv<sum_t>(slack + 1, co_r);
                value_r = vs_r.upper_bound() + delta_r;
                assert (slack-co_r * delta_r < 0 && 0 <= slack - co_r * (delta_r + 1));
                // values below the lower bound are already false
                if (value_r < vs_r.lower_bound()) {
                    continue;
                }
                // get the literal of the value
                if (vs_r.has_literal(value_r)) {
                    lit_r = -solver.get_literal(cc, vs_r, value_r);
                }
            }

            // build the reason if the literal has not already been propagated
            if (lit_r == 0 || !ass.is_true(lit_r)) {
                auto slack_r = slack - co_r * delta_r;
                assert (slack_r < 0);
                auto &reason = solver.temp_reason();
                // add the constraint itself
                if (!ass.is_fixed(-clit)) {
                    reason.emplace_back(-clit);
                }
                for (auto [co_a, var_a] : T::constraint_) {
                    if (var_a == var_r) {
                        continue;
                    }
                    auto &vs_a = solver.var_state(var_a);

                    // calculate reason literal
                    auto [ret, lit_a] = calculate_reason(solver, cc, slack_r, vs_a, co_a);
                    if (!ret) {
                        return false;
                    }

                    // append the reason literal
                    if (!ass.is_fixed(lit_a)) {
                        reason.emplace_back(lit_a);
                    }
                }

                // append the consequence
                bool guess = !reason.empty() || tagged;
                if (co_r > 0) {
                    lit_r = solver.update_literal(cc, vs_r, value_r-1, guess ? Clingo::TruthValue::Free : Clingo::TruthValue::True);
                    reason.emplace_back(lit_r);
                }
                else {
                    lit_r = -solver.update_literal(cc, vs_r, value_r, guess ? Clingo::TruthValue::Free : Clingo::TruthValue::False);
                    reason.emplace_back(lit_r);
                }

                // propagate the clause
                if (!cc.add_clause(reason, tagged ? Clingo::ClauseType::Volatile : Clingo::ClauseType::Learnt)) {
                    return false;
                }

                // minimize constraints cannot be propagated on decision level 0
                assert(ass.is_true(lit_r) || (tagged && ass.decision_level() == 0));
            }
        }
        return true;
    }
};


//! A translateable constraint state.
class SumConstraintState : public AbstractConstraintState {
    friend class SumConstraintStateImpl<false, SumConstraintState, SumConstraint>;

    SumConstraintState(SumConstraint &constraint)
    : constraint_{constraint} {
    }

    [[nodiscard]] static constexpr bool has_rhs(Solver &solver) {
        static_cast<void>(solver);
        return true;
    }

    [[nodiscard]] val_t rhs(Solver &solver) const {
        static_cast<void>(solver);
        return constraint_.rhs();
    }

    [[nodiscard]] bool removable() final {
        return true;
    }

    [[nodiscard]] UniqueConstraintState copy() const final {
        auto ret = std::make_unique<SumConstraintStateImpl<false, SumConstraintState, SumConstraint>>(constraint_);
        ret->lower_bound_ = lower_bound_;
        ret->upper_bound_ = upper_bound_;
        ret->inactive_level_ = inactive_level_;
        ret->todo_ = todo_;
        return ret;
    }

    //! Estimate the size of the translation in terms of the number of literals
    //! necessary for the weight constraint.
    sum_t weight_estimate_(Solver &solver) const {
        sum_t estimate = 0;
        sum_t slack = rhs(solver) - lower_bound_;
        for (auto [co, var] : constraint_) {
            auto &vs = solver.var_state(var);
            if (co > 0) {
                auto diff = slack + static_cast<sum_t>(co) * vs.lower_bound();
                auto value = floordiv<sum_t>(diff, co);
                assert (value >= vs.lower_bound());
                estimate += std::min<sum_t>(value + 1, vs.upper_bound()) - vs.lower_bound();
            }
            else {
                auto diff = slack + static_cast<sum_t>(co) * vs.upper_bound();
                auto value = -floordiv<sum_t>(diff, -co);
                assert(value <= vs.upper_bound());
                estimate += vs.upper_bound()-std::max<sum_t>(value - 1, vs.lower_bound());
            }
        }
        return estimate;
    }

    //! Translate the constraint to weight a constraint.
    std::pair<bool, bool> weight_translate_(Solver &solver, InitClauseCreator &cc, sum_t slack) { // NOLINT
        // translate small enough constraint
        // Note: this magic number can be dangerous if there is a huge number of
        // variables.
        std::vector<Clingo::WeightedLiteral> wlits;
        for (auto [co, var] : constraint_) {
            auto &vs = solver.var_state(var);
            if (co > 0) {
                auto diff = slack + static_cast<sum_t>(co) * vs.lower_bound();
                auto value = floordiv<sum_t>(diff, co);
                assert (value >= vs.lower_bound());
                for (sum_t i = vs.lower_bound(), e = std::min<sum_t>(value + 1, vs.upper_bound()); i != e; ++i) {
                    wlits.emplace_back(-solver.get_literal(cc, vs, i), co);
                }
            }
            else {
                auto diff = slack + static_cast<sum_t>(co) * vs.upper_bound();
                auto value = -floordiv<sum_t>(diff, -co);
                assert(value <= vs.upper_bound());
                for (sum_t i = std::max<sum_t>(value - 1, vs.lower_bound()), e = vs.upper_bound(); i != e; ++i) {
                    wlits.emplace_back(solver.get_literal(cc, vs, i), -co);
                }
            }
        }
        if (std::numeric_limits<val_t>::min() < slack && slack < std::numeric_limits<val_t>::max()) {
            // Note: For strict constraints, we can actually use the
            // equivalence here and only add one weight constraint instead of
            // two. In the current system design, this requires storing the
            // weight constraints and detect complementary constraints. It
            // might be a good idea in general to add translation constraints
            // later because we can run into the problem of successivly adding
            // variables and constraints here.
            return {cc.add_weight_constraint(constraint_.literal(), wlits, slack, Clingo::WeightConstraintType::RightImplication), false};
        }
        return {true, false};
    }

    bool clause_estimate_(Solver &solver, sum_t lower, sum_t upper, sum_t maximum) {
        std::vector<std::tuple<size_t, sum_t, sum_t, sum_t>> todo{{0, 1, lower, upper}};
        sum_t estimate{0};

        while (!todo.empty()) {
            auto [i, n, lower, upper] = todo.back();

            if (n <= 0) {
                todo.pop_back();
                continue;
            }

            if (i > 0) {
                auto [co, var] = constraint_[i - 1];
                if (co > 0) {
                    todo.back() = std::tuple(i, n - 1, lower + co, upper + co);
                }
                else {
                    todo.back() = std::tuple(i, n - 1, lower - co, upper - co);
                }
                estimate -= 1;
            }
            else {
                todo.pop_back();
            }

            if (lower < 0) {
                estimate += 1;
                if (estimate >= maximum) {
                    return false;
                }
                continue;
            }

            assert (upper < 0 && i < constraint_.size());

            auto [co, var] = constraint_[i];
            auto &vs = solver.var_state(var);

            sum_t value_lower{0};
            sum_t value_upper{0};
            if (co > 0) {
                auto diff_lower = upper + static_cast<sum_t>(co) * vs.upper_bound();
                auto diff_upper = lower + static_cast<sum_t>(co) * vs.lower_bound();
                value_lower = std::max<sum_t>(vs.lower_bound(), floordiv<sum_t>(diff_lower, co) + 1);
                value_upper = std::min<sum_t>(vs.upper_bound(), floordiv<sum_t>(diff_upper, co) + 1);
                lower = lower - static_cast<sum_t>(co) * (value_upper - vs.lower_bound());
                upper = upper + static_cast<sum_t>(co) * (vs.upper_bound() - value_upper);
            }
            else {
                auto diff_upper = upper + static_cast<sum_t>(co) * vs.lower_bound();
                auto diff_lower = lower + static_cast<sum_t>(co) * vs.upper_bound();
                value_lower = std::max<sum_t>(vs.lower_bound(), -floordiv<sum_t>(diff_lower, -co) - 1);
                value_upper = std::min<sum_t>(vs.upper_bound(), -floordiv<sum_t>(diff_upper, -co) - 1);
                lower = lower - static_cast<sum_t>(co) * (value_lower - vs.upper_bound());
                upper = upper + static_cast<sum_t>(co) * (vs.lower_bound() - value_lower);
            }

            n = value_upper - value_lower + 1;
            estimate += n;
            if (estimate >= maximum) {
                return false;
            }
            todo.emplace_back(i+1, n, lower, upper);
        }

        return true;
    }

    bool clause_translate_(Solver &solver, AbstractClauseCreator &cc, sum_t lower, sum_t upper, bool literals_only) { // NOLINT
        auto clit = cc.assignment().is_false(-constraint_.literal()) ? -TRUE_LIT : -constraint_.literal();
        std::vector<std::tuple<size_t, size_t, sum_t, sum_t, sum_t, sum_t>> todo{{0, 0, 0, 0, lower, upper}};
        std::vector<lit_t> clause(constraint_.size() + 1, 0);

        while (!todo.empty()) {
            auto [i, j, value_lower, value_upper, lower, upper] = todo.back();

            if (value_lower > value_upper) {
                todo.pop_back();
                continue;
            }

            lit_t lit{0};

            if (i > 0) {
                auto [co, var] = constraint_[i-1];
                auto &vs = solver.var_state(var);
                if (co > 0) {
                    todo.back() = std::tuple(i, j, value_lower, value_upper - 1, lower + co, upper + co);
                    lit = solver.get_literal(cc, vs, value_upper);
                }
                else {
                    todo.back() = std::tuple(i, j, value_lower + 1, value_upper, lower - co, upper - co);
                    lit = -solver.get_literal(cc, vs, value_lower);
                }
            }
            else {
                todo.pop_back();
                lit = clit;
            }

            if (lit != -TRUE_LIT) {
                clause[j] = lit;
                j += 1;
            }

            if (lower < 0) {
                if (!literals_only && !cc.add_clause({clause.data(), clause.data() + j})) { // NOLINT
                    return false;
                }
                continue;
            }

            assert(upper < 0 && i < constraint_.size());

            auto [co, var] = constraint_[i];
            auto &vs = solver.var_state(var);

            if (co > 0) {
                auto diff_lower = upper + static_cast<sum_t>(co) * vs.upper_bound();
                auto diff_upper = lower + static_cast<sum_t>(co) * vs.lower_bound();
                value_lower = std::max<sum_t>(vs.lower_bound() - 1, floordiv<sum_t>(diff_lower, co));
                value_upper = std::min<sum_t>(vs.upper_bound() - 1, floordiv<sum_t>(diff_upper, co));
                lower = lower - static_cast<sum_t>(co) * (value_upper - vs.lower_bound() + 1);
                upper = upper + static_cast<sum_t>(co) * (vs.upper_bound() - value_upper - 1);
            }
            else {
                auto diff_upper = upper + static_cast<sum_t>(co) * vs.lower_bound();
                auto diff_lower = lower + static_cast<sum_t>(co) * vs.upper_bound();
                value_lower = std::max<sum_t>(vs.lower_bound(), -floordiv<sum_t>(diff_lower, -co) - 1);
                value_upper = std::min<sum_t>(vs.upper_bound(), -floordiv<sum_t>(diff_upper, -co) - 1);
                upper = upper + static_cast<sum_t>(co) * (vs.lower_bound() - value_lower);
                lower = lower - static_cast<sum_t>(co) * (value_lower - vs.upper_bound());
            }

            assert (vs.lower_bound() <= value_upper + 1);
            assert (value_lower <= vs.upper_bound());
            todo.emplace_back(i + 1, j, value_lower, value_upper, lower, upper);
        }

        return true;
    }

    //! Translate a constraint to clauses or weight constraints.
    [[nodiscard]] std::pair<bool, bool> translate(Config const &config, Solver &solver, InitClauseCreator &cc, ConstraintVec &added) final {
        static_cast<void>(added);
        auto ass = cc.assignment();

        sum_t rhs = this->rhs(solver);
        if (ass.is_false(constraint_.literal()) || upper_bound_ <= rhs) {
            return {true, true};
        }

        auto lower = rhs - lower_bound_;
        auto upper = rhs - upper_bound_;

        // Note: otherwise propagation is broken
        assert(lower >= 0);

        if (clause_estimate_(solver, lower, upper, config.clause_limit)) {
            auto ret = clause_translate_(solver, cc, lower, upper, config.literals_only);
            return {ret, !config.literals_only};
        }

        // translation to weight constraints
        if (weight_estimate_(solver) < config.weight_constraint_limit) {
            return weight_translate_(solver, cc, lower);
        }

        return {true, false};
    }
private:
    SumConstraint &constraint_;
    sum_t lower_bound_{0};
    sum_t upper_bound_{0};
    level_t inactive_level_{0};
    bool todo_{false};
};

/*
class MinimizeConstraintState(AbstractSumConstraintState):
    """
    A translateable minimize constraint state.

    Class Variables
    ======
    tagged           -- True if constraint applies only during current solving step.
    tagged_removable -- True if the constraint can be temporarily removed.
    """

    tagged = True
    tagged_removable = False

    def __init__(self, constraint):
        AbstractSumConstraintState.__init__(self)
        self.constraint = constraint

    def copy(self):
        """
        Return a copy of the constraint state to be used with another state.
        """
        cs = MinimizeConstraintState(self.constraint)
        cs.inactive_level = self.inactive_level
        cs.lower_bound = self.lower_bound
        cs.upper_bound = self.upper_bound
        return cs

    @property
    def literal(self):
        """
        Return the literal of the associated constraint.
        """
        return self.constraint.literal

    @property
    def elements(self):
        """
        Return the elements of the constraint.
        """
        return self.constraint.elements

    def rhs(self, state):
        """
        Return the bound of the constraint
        """
        return state.minimize_bound

    def translate(self, cc, state, config, added):
        """
        Translate the minimize constraint into clasp's minimize constraint.
        """
        if not config.translate_minimize:
            return True, False

        cc.add_minimize(TRUE_LIT, -self.constraint.adjust, 0)
        for co, var in self.constraint.elements:
            vs = state.var_state(var)
            cc.add_minimize(TRUE_LIT, co*vs.min_bound, 0)
            for v in range(vs.min_bound, vs.max_bound):
                cc.add_minimize(-state.get_literal(vs, v, cc), co, 0)
        return True, True


class DistinctState(AbstractConstraintState):
    """
    Capture the state of a distinct constraint.

    Class Variables
    ======
    tagged_removable -- True if the constraint can be temporarily removed.
    """

    tagged_removable = True

    def __init__(self, constraint):
        AbstractConstraintState.__init__(self)
        self.constraint = constraint
        self.dirty = TodoList()
        self.todo = TodoList()
        self.map_upper = {}
        self.map_lower = {}
        self.assigned = {}

    def copy(self):
        """
        Return a copy of the constraint state to be used with another state.
        """
        ds = DistinctState(self.constraint)
        ds.inactive_level = self.inactive_level
        for value, indices in self.map_upper.items():
            ds.map_upper[value] = indices[:]
        for value, indices in self.map_lower.items():
            ds.map_lower[value] = indices[:]
        ds.assigned = self.assigned.copy()
        ds.dirty = self.dirty.copy()
        return ds

    @property
    def literal(self):
        """
        Return the literal of the associated constraint.
        """
        return self.constraint.literal

    def _init(self, state, i):
        """
        Recalculates the bounds of the i-th element of the constraint assuming
        that the bounds of this element are not currently in the bound maps.
        """
        # calculate new values
        value, elements = self.constraint.elements[i]
        upper = lower = value
        for co, var in elements:
            if co > 0:
                upper += co*state.var_state(var).upper_bound
                lower += co*state.var_state(var).lower_bound
            else:
                upper += co*state.var_state(var).lower_bound
                lower += co*state.var_state(var).upper_bound
        # set new values
        self.assigned[i] = (lower, upper)
        self.map_upper.setdefault(upper, []).append(i)
        self.map_lower.setdefault(lower, []).append(i)

    def attach(self, state):
        """
        Attach the distinct constraint to the state.
        """
        for i, (_, elements) in enumerate(self.constraint.elements):
            self._init(state, i)
            for co, var in elements:
                state.add_var_watch(var, i+1 if co > 0 else -i-1, self)

    def detach(self, state):
        """
        Detach the constraint frow the given state.
        """
        for i, (_, elements) in enumerate(self.constraint.elements):
            for co, var in elements:
                state.remove_var_watch(var, i+1 if co > 0 else -i-1, self)

    def update(self, i, _):
        """
        Add an element whose bound has changed to the todo list and mark it as
        dirty.

        If i is greater zero, than the lower bound changed; otherwise the upper
        bound.
        """
        self.dirty.add(abs(i)-1)
        self.todo.add(i)
        return True

    def undo(self, i, _):
        """
        Clear the todo list and mark the given element as dirty.
        """
        self.dirty.add(abs(i)-1)
        self.todo.clear()

    def _update(self, state):
        """
        Recalculate all elements marked dirty.
        """
        for i in self.dirty:
            lower, upper = self.assigned[i]
            self.map_lower[lower].remove(i)
            self.map_upper[upper].remove(i)
            self._init(state, i)
        self.dirty.clear()

    def _estimate(self):
        """
        Estimate the translation cost of the constraint in terms of the
        required number of weight constraints.
        """
        cost = 0

        intervals = IntervalSet()
        for lower, upper in self.assigned.values():
            intervals.add(lower, upper+1)
        for lower, upper in intervals:
            cost += upper - lower

        return cost

    def _domain(self, state, lower, elements):
        """
        Calculate the domain of a term.
        """
        values = IntervalSet()

        values.add(lower, lower+1)
        for co, var in elements:
            current = values.copy()
            cs = state.var_state(var)
            add = abs(co)
            for _ in range(cs.lower_bound, cs.upper_bound):
                for l, u in current:
                    values.add(l+add, u+add)
                add += abs(co)

        return values

    def _var(self, state, lower, upper, elements, added):
        """
        Introduce a variable and make it equal to the term.
        """
        assert elements

        if len(elements) == 1:
            return elements[0][1]

        var = state.add_variable(lower, upper)
        elems = [(-1, var)] + elements
        added.append(SumConstraint(TRUE_LIT, elems, 0))
        added.append(SumConstraint(TRUE_LIT, [(-c, v) for c, v in elems], 0))
        return var

    def translate(self, cc, state, config, added):
        """
        Translate small enough distinct constraints to weight constraints.
        """
        if self._estimate() >= config.distinct_limit:
            return True, False

        # compute domain of terms and identify values involved in at least two terms
        union = set()
        counts = {}
        elem_values = []
        for i, (_, elements) in enumerate(self.constraint.elements):
            elem_values.append(self._domain(state, self.assigned[i][0], elements))
            for value in elem_values[-1].enum():
                if value not in counts:
                    counts[value] = 0
                counts[value] += 1
                if counts[value] > 1:
                    union.add(value)

        # calculate variables for terms avoiding unnecessary variables
        elem_vars = []
        for (fixed, elements), (lower, upper), values in zip(self.constraint.elements, self.assigned.values(), elem_values):
            var = None
            for value in values.enum():
                if value not in union:
                    continue
                var = self._var(state, lower, upper, elements, added)
                break
            elem_vars.append((fixed, var))

        # add weight constraints
        for value in sorted(union):
            # variables that have to be different
            wlits = []
            for (fixed, var), values in zip(elem_vars, elem_values):
                if value not in values:
                    continue
                lit = TRUE_LIT
                if var is not None:
                    # lit == var<=value && var>=value
                    #     == var<=value && not var<=value-1
                    vs = state.var_state(var)
                    a = state.get_literal(vs, value-fixed, cc)
                    b = -state.get_literal(vs, value-fixed-1, cc)
                    if a == TRUE_LIT:
                        lit = b
                    elif b == -TRUE_LIT:
                        lit = a
                    else:
                        lit = cc.add_literal()
                        cc.add_clause([-a, -b, lit])
                        cc.add_clause([a, -lit])
                        cc.add_clause([b, -lit])
                wlits.append((lit, 1))

            assert len(wlits) > 1
            cc.add_weight_constraint(self.literal, wlits, 1, 1)

        return True, True

    def _propagate(self, cc, state, s, i, j):
        """
        Propagate a distinct constraint assuming that element i is assigned and
        one of element element j's bounds as determined by s match the value of
        element i.

        The reasons generated by this function are not necessarily unit.
        Implementing proper unit propagation for arbitrary linear terms would
        be quite involved. This implementation still works because it
        guarantees conflict detection and the added constraints might become
        unit later. The whole situation could be simplified by restricting the
        syntax of distinct constraints.
        """
        # case s > 0:
        #   example: x != y+z
        #     x <= 10 & not x <= 9 & y <= 5 & z <= 5 => y <= 4 | z <= 4
        #   example: x != -y
        #     x <= 9 & not x <= 8 &     y >= -9  =>     y >= -8
        #     x <= 9 & not x <= 8 & not y <= -10 => not y <= -9
        # case s < 0:
        #   example: x != y
        #     x = 9 & y >= 9 => y >= 10
        #     x <= 9 & not x <= 8 & not y <= 8 => not y <= 9
        #   example: x != -y
        #     x <= 9 & not x <= 8 & y <= -9 => y <= -10
        ass = cc.assignment

        reason = []
        is_fact = len(self.constraint.elements[j][1]) == 1

        # assigned index
        for _, var in self.constraint.elements[i][1]:
            vs = state.var_state(var)
            assert vs.is_assigned
            reason.append(-state.get_literal(vs, vs.upper_bound, cc))
            assert ass.is_false(reason[-1])
            if not ass.is_fixed(reason[-1]):
                is_fact = False
            reason.append(state.get_literal(vs, vs.lower_bound-1, cc))
            assert ass.is_false(reason[-1])
            if not ass.is_fixed(reason[-1]):
                is_fact = False

        # bounded index
        for co, var in self.constraint.elements[j][1]:
            vs = state.var_state(var)
            if s*co > 0:
                reason.append(-state.get_literal(vs, vs.upper_bound, cc))
                assert ass.is_false(reason[-1])
                if not ass.is_fixed(reason[-1]):
                    is_fact = False
                # add consequence
                ret, lit = state.update_literal(vs, vs.upper_bound-1, cc, is_fact or None)
                if not ret:
                    return False
                reason.append(lit)
                if ass.is_true(reason[-1]):
                    return True
            else:
                reason.append(state.get_literal(vs, vs.lower_bound-1, cc))
                assert ass.is_false(reason[-1])
                if not ass.is_fixed(reason[-1]):
                    is_fact = False
                # add consequence
                ret, lit = state.update_literal(vs, vs.lower_bound, cc, not is_fact and None)
                if not ret:
                    return False
                reason.append(-lit)

                if ass.is_true(reason[-1]):
                    return True

        return cc.add_clause(reason)

    def propagate(self, state, cc, config, check_state):
        """
        Prepagates the distinct constraint.

        See `_propagate` for exmamples what is propagated.
        """
        self._update(state)

        for i in self.todo:
            j = abs(i)-1
            lower, upper = self.assigned[j]
            if lower == upper:
                for k in self.map_upper[upper]:
                    if j != k and not self._propagate(cc, state, 1, j, k):
                        return False
                for k in self.map_lower[lower]:
                    if j != k and not self._propagate(cc, state, -1, j, k):
                        return False
            elif i < 0:
                for k in self.map_upper[upper]:
                    if self.assigned[k][0] == self.assigned[k][1]:
                        if k+1 in self.todo or -k-1 in self.todo:
                            break
                        if not self._propagate(cc, state, 1, k, j):
                            return False
                        break
            else:
                for k in self.map_lower[lower]:
                    if self.assigned[k][0] == self.assigned[k][1]:
                        if k+1 in self.todo or -k-1 in self.todo:
                            break
                        if not self._propagate(cc, state, -1, k, j):
                            return False
                        break
        self.todo.clear()

        return True

    def check_full(self, state):
        """
        This function checks if a constraint is satisfied w.r.t. the final
        values of its integer variables.

        This function should only be called total assignments.
        """
        values = set()
        for value, elements in self.constraint.elements:
            for co, var in elements:
                vs = state.var_state(var)
                assert vs.is_assigned
                value += co*vs.upper_bound

            if value in values:
                return False
            values.add(value)

        return True
*/

} // namespace

UniqueConstraintState SumConstraint::create_state() {
    return std::make_unique<SumConstraintStateImpl<false, SumConstraintState, SumConstraint>>(*this);
}

UniqueConstraintState MinimizeConstraint::create_state() {
    throw std::runtime_error("implement me!!!");
}

UniqueConstraintState DistinctConstraint::create_state() {
    throw std::runtime_error("implement me!!!");
}

} // namespace Clingcon
