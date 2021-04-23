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

#include <set>

namespace Clingcon {

namespace {

//! Implements propagation for sum and minimize constraints.
template <bool tagged, typename T>
class SumConstraintStateImpl final : public T {
public:
    SumConstraintStateImpl(decltype(T::constraint_) constraint)
    : T{constraint} {
    }

    SumConstraintStateImpl(SumConstraintStateImpl &&x) = delete;
    SumConstraintStateImpl& operator=(SumConstraintStateImpl const &x) = delete;
    SumConstraintStateImpl& operator=(SumConstraintStateImpl &&x) = delete;
    ~SumConstraintStateImpl() override = default;

    [[nodiscard]] UniqueConstraintState copy() const override {
        return std::unique_ptr<SumConstraintStateImpl>{new SumConstraintStateImpl(*this)};
    }

    [[nodiscard]] bool removable() override {
        return !tagged;
    }

    decltype(T::constraint_) constraint() override {
        return T::constraint_;
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
        auto clit = T::constraint_.literal();

        // Note: this has a noticible cost because of the shortcuts below
        if (check_state) {
            check_state_(solver);
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
                auto [ret, lit] = calculate_reason_(solver, cc, slack, vs, co);
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
                if (value_r >= vs_r.upper_bound()) {
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
                    auto [ret, lit_a] = calculate_reason_(solver, cc, slack_r, vs_a, co_a);
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

                // Literals might not be propagated on level 0.
                assert(ass.is_true(lit_r) || ass.decision_level() == 0);
            }
        }
        return true;
    }

private:
    SumConstraintStateImpl(SumConstraintStateImpl const &x)
    : T{x} {
    }

    void check_state_(Solver &solver) {
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

    [[nodiscard]] std::pair<bool, lit_t> calculate_reason_(Solver &solver, AbstractClauseCreator &cc, sum_t &slack, VarState &vs, val_t co) {
        auto ass = cc.assignment();
        uint64_t found = 0;
        lit_t lit{0};
        bool ret = true;

        if (co > 0) {
            sum_t current = vs.lower_bound();
            // the direct reason literal
            auto lit_reason = solver.get_literal(cc, vs, current-1);
            lit = lit_reason;
            assert (ass.is_false(lit));
            if (solver.config().refine_reasons && slack + co < 0 && ass.decision_level() > 0) {
                auto delta = -floordiv<sum_t>(slack + 1, -co);
                auto value = std::max<sum_t>(current + delta, vs.min_bound());
                if (value < current) {
                    // refine reason literal
                    if (auto olit = vs.order_lit_ge(value - 1); olit.has_value() && olit->second + 1 < current) {
                        found = 1;
                        slack -= static_cast<sum_t>(co) * (olit->second + 1 - current);
                        current = olit->second + 1;
                        assert(slack < 0);
                        lit = olit->first;
                        // Note: The literal might have been introduced and
                        // made true during constraint propagation.
                        if (!ass.is_false(lit)) {
                            assert(ass.is_true(lit));
                            ret = cc.add_clause({lit_reason, -lit});
                        }
                    }
                    // introduce reason literal
                    // Note: It is important to imply literals by the smallest
                    // available literal to keep the state consistent.
                    // Furthermore, we only introduce literals implied on the
                    // current decision level to avoid backtracking.
                    if (ret && solver.config().refine_introduce && ass.level(lit) == ass.decision_level() && value < current) {
                        ++solver.statistics().introduced_reason;
                        found = 1;
                        slack -= static_cast<sum_t>(co) * (value - current);
                        assert(slack < 0);
                        auto refined = solver.get_literal(cc, vs, value - 1);
                        assert(!ass.is_true(refined));
                        ret = ass.is_false(refined) || cc.add_clause({lit, -refined});
                        lit = refined;
                    }
                }
            }
        }
        else {
            // symmetric case
            sum_t current = vs.upper_bound();
            auto lit_reason = -solver.get_literal(cc, vs, current);
            lit = lit_reason;
            assert(ass.is_false(lit));
            if (solver.config().refine_reasons && slack - co < 0 && ass.decision_level() > 0) {
                auto delta = floordiv<sum_t>(slack + 1, co);
                auto value = std::min<sum_t>(current + delta, vs.max_bound());
                if (value > current) {
                    // refine reason literal
                    if (auto olit = vs.order_lit_le(value); olit.has_value() && olit->second > current) {
                        found = 1;
                        slack -= static_cast<sum_t>(co) * (olit->second - current);
                        current = olit->second;
                        assert(slack < 0);
                        lit = -olit->first;
                        if (!ass.is_false(lit)) {
                            assert(ass.is_true(lit));
                            ret = cc.add_clause({lit_reason, -lit});
                        }
                    }
                    // introduce reason literal
                    if (ret && solver.config().refine_introduce && ass.level(lit) == ass.decision_level() && value > current) {
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
};


//! A translateable constraint state.
class SumConstraintState : public AbstractConstraintState {
public:
    friend class SumConstraintStateImpl<false, SumConstraintState>;

    SumConstraintState(SumConstraintState &&x) = delete;
    SumConstraintState& operator=(SumConstraintState const &x) = delete;
    SumConstraintState& operator=(SumConstraintState &&x) = delete;
    ~SumConstraintState() override = default;

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
        bool translate =
            cc.statistics().translate_clauses < config.clause_limit_total &&
            clause_estimate_(solver, lower, upper, config.clause_limit);
        if (translate) {
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
    SumConstraintState(SumConstraint &constraint)
    : constraint_{constraint} {
    }

    SumConstraintState(SumConstraintState const &x)
    : AbstractConstraintState{} // NOLINT
    , constraint_{x.constraint_}
    , lower_bound_{x.lower_bound_}
    , upper_bound_{x.upper_bound_}
    , inactive_level_{x.inactive_level_}
    , todo_{x.todo_} {
    }

    [[nodiscard]] static constexpr bool has_rhs(Solver &solver) {
        static_cast<void>(solver);
        return true;
    }

    [[nodiscard]] val_t rhs(Solver &solver) const {
        static_cast<void>(solver);
        return constraint_.rhs();
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
                estimate += vs.upper_bound() - std::max<sum_t>(value - 1, vs.lower_bound());
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
            return {cc.add_weight_constraint(constraint_.literal(), wlits, slack, Clingo::WeightConstraintType::RightImplication), true};
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
                auto co = constraint_[i - 1].first;
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

#if 0
        CoVarVec elements;
        elements.assign(constraint_.begin(), constraint_.end());
        std::sort(elements.begin(), elements.end(), [&](auto const &a, auto const &b) {
            // sort by coefficient (ascending)
            if (std::abs(a.first) != std::abs(b.first)) {
                return std::abs(a.first) > std::abs(b.first);
            }
            // i do not see why any of the below should matter...
            auto &vs_a = solver.var_state(a.second);
            auto &vs_b = solver.var_state(b.second);
            // sort by minimum value (ascending)
            sum_t min_a{0};
            sum_t min_b{0};
            if (a.first > 0) {
                min_a = a.first * vs_a.lower_bound();
            }
            else {
                min_a = a.first * vs_a.upper_bound();
            }
            if (b.first > 0) {
                min_b = b.first * vs_b.lower_bound();
            }
            else {
                min_b = b.first * vs_b.upper_bound();
            }
            if (min_a != min_b) {
                return min_a > min_b;
            }
            // sort by domain size (descending)
            val_t size_a = vs_a.upper_bound() - vs_a.lower_bound();
            val_t size_b = vs_b.upper_bound() - vs_b.lower_bound();
            if (size_a != size_b) {
                return size_a < size_b;
            }
            // sort by variable name
            return a.second < b.second;
        });
#else
        auto &elements = constraint_;
#endif

        while (!todo.empty()) {
            auto [i, j, value_lower, value_upper, lower, upper] = todo.back();

            if (value_lower > value_upper) {
                todo.pop_back();
                continue;
            }

            lit_t lit{0};

            if (i > 0) {
                auto [co, var] = elements[i-1];
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

            assert(upper < 0 && i < elements.size());

            auto [co, var] = elements[i];
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

    SumConstraint &constraint_;
    sum_t lower_bound_{0};
    sum_t upper_bound_{0};
    level_t inactive_level_{0};
    bool todo_{false};
};

//! A translateable constraint state.
class MinimizeConstraintState : public AbstractConstraintState {
public:
    friend class SumConstraintStateImpl<true, MinimizeConstraintState>;

    MinimizeConstraintState(MinimizeConstraintState &&x) = delete;
    MinimizeConstraintState& operator=(MinimizeConstraintState const &x) = delete;
    MinimizeConstraintState& operator=(MinimizeConstraintState &&x) = delete;
    ~MinimizeConstraintState() override = default;

    //! Get the number of literals required to translate the minimize constraint.
    [[nodiscard]] int64_t required_literals(Solver &solver) const {
        int64_t size = 0;
        for (auto [co, var] : constraint_) {
            auto &vs = solver.var_state(var);
            size += static_cast<int64_t>(vs.max_bound()) - vs.min_bound() - 1;
        }
        return size;
    }

    //! Translate the minimize constraint into clasp's minimize constraint.
    [[nodiscard]] std::pair<bool, bool> translate(Config const &config, Solver &solver, InitClauseCreator &cc, ConstraintVec &added) final {
        static_cast<void>(added);

        bool translate = solver.translate_minimize();
        translate = translate || config.translate_minimize == std::numeric_limits<uint32_t>::max();
        translate = translate || required_literals(solver) <= config.translate_minimize;

        if (!translate) {
            return {true, false};
        }

        solver.enable_translate_minimize();
        cc.add_minimize(TRUE_LIT, -constraint_.adjust(), 0);
        for (auto [co, var] : constraint_) {
            auto &vs = solver.var_state(var);
            cc.add_minimize(TRUE_LIT, safe_mul(co, vs.min_bound()), 0);
            for (auto value = vs.min_bound(); value < vs.max_bound(); ++value) {
                cc.add_minimize(-solver.get_literal(cc, vs, value), co, 0);
            }
        }

        return {true, true};
    }

private:
    MinimizeConstraintState(MinimizeConstraint &constraint)
    : constraint_{constraint} {
    }

    MinimizeConstraintState(MinimizeConstraintState const &x)
    : AbstractConstraintState{} // NOLINT
    , constraint_{x.constraint_}
    , lower_bound_{x.lower_bound_}
    , upper_bound_{x.upper_bound_}
    , inactive_level_{x.inactive_level_}
    , todo_{x.todo_} {
    }

    [[nodiscard]] static bool has_rhs(Solver &solver) {
        return solver.minimize_bound().has_value();
    }

    [[nodiscard]] static val_t rhs(Solver &solver) {
        return *solver.minimize_bound();
    }

    MinimizeConstraint &constraint_;
    sum_t lower_bound_{0};
    sum_t upper_bound_{0};
    level_t inactive_level_{0};
    bool todo_{false};
};

//! Capture the state of a distinct constraint.
class DistinctConstraintState final : public AbstractConstraintState {
    using DiffElement = std::tuple<val_t, val_t, var_t>;
    using DiffVec = std::vector<DiffElement>;
public:
    DistinctConstraintState(DistinctConstraint &constraint)
    : constraint_{constraint} {
        assigned_.resize(constraint_.size());
        in_dirty_.resize(constraint_.size(), false);
        in_todo_lower_.resize(constraint_.size(), false);
        in_todo_upper_.resize(constraint_.size(), false);
        dirty_.reserve(constraint_.size());
        todo_lower_.reserve(constraint_.size());
        todo_upper_.reserve(constraint_.size());
    }

    DistinctConstraintState(DistinctConstraintState &&) = delete;
    DistinctConstraintState &operator=(DistinctConstraintState const &) = delete;
    DistinctConstraintState &operator=(DistinctConstraintState &&) = delete;
    ~DistinctConstraintState() override = default;

    DistinctConstraint& constraint() override {
        return constraint_;
    }

    void attach(Solver &solver) override {
        val_t idx = 0;
        for (auto const &element : constraint_) {
            auto [lower, upper] = init_(solver, idx);
            lower_.emplace(lower, idx);
            upper_.emplace(upper, idx);
            for (auto [co, var] : element) {
                solver.add_var_watch(var, co > 0 ? idx + 1 : -idx - 1, *this);
            }
            ++idx;
        }
    }

    void detach(Solver &solver) override {
        val_t idx = 0;
        for (auto const &element : constraint_) {
            for (auto [co, var] : element) {
                solver.remove_var_watch(var, co > 0 ? idx + 1 : -idx - 1, *this);
            }
            ++idx;
        }
    }

    //! Translate small enough distinct constraints to weight constraints.
    [[nodiscard]] std::pair<bool, bool> translate(Config const &config, Solver &solver, InitClauseCreator &cc, ConstraintVec &added) override {
        if (!estimate_(config.distinct_limit)) {
            return {true, false};
        }

        // count how often values occur in term domains
        std::map<sum_t, uint32_t> counts;
        std::vector<IntervalSet<sum_t>> domains;
        for (uint32_t idx = 0, end = constraint_.size(); idx != end; ++idx) {
            domains.emplace_back(domain_(solver, idx));
            domains.back().enumerate([&](sum_t value) {
                ++counts[value];
                return true;
            });
        }

        DiffVec diff_elems;

        // calculate variables for terms avoiding unnecessary variables
        for (uint32_t idx = 0, end = constraint_.size(); idx != end; ++idx) {
            DiffElement elem{constraint_[idx].fixed(), 1, INVALID_VAR};
            domains[idx].enumerate([&](sum_t value) {
                if (counts[value] > 1) {
                    elem = var_(config, solver, idx, added);
                    return false;
                }
                return true;
            });
            diff_elems.emplace_back(elem);
        }

        // add weight constraints
        for (auto [value, count] : counts) {
            if (count <= 1) {
                continue;
            }
            // variables that have to be different
            std::vector<Clingo::WeightedLiteral> wlits;

            for (uint32_t idx = 0, end = constraint_.size(); idx != end; ++idx) {
                auto &domain = domains[idx];
                if (!domain.contains(value)) {
                    continue;
                }
                auto [fixed, co, var] = diff_elems[idx];
                auto lit = TRUE_LIT;
                if (var != INVALID_VAR) {
                    // lit == var<=value && var>=value
                    //     == var<=value && not var<=value-1
                    auto &vs = solver.var_state(var);

                    auto adjust = (value - fixed) / co;
                    auto a = solver.get_literal(cc, vs, adjust);
                    auto b = -solver.get_literal(cc, vs, adjust - 1);

                    if (a == TRUE_LIT) {
                        lit = b;
                    }
                    else if (b == -TRUE_LIT) {
                        lit = a;
                    }
                    else {
                        lit = cc.add_literal();
                        if (!cc.add_clause({-a, -b, lit})) {
                            return {false, false};
                        }
                        if (!cc.add_clause({a, -lit})) {
                            return {false, false};
                        }
                        if (!cc.add_clause({b, -lit})) {
                            return {false, false};
                        }
                    }
                }
                wlits.emplace_back(lit, 1);
            }

            assert(wlits.size() > 1);
            if (!cc.add_weight_constraint(constraint_.literal(), wlits, 1, Clingo::WeightConstraintType::RightImplication)) {
                return {false, false};
            }
        }

        return {true, true};
    }

    [[nodiscard]] UniqueConstraintState copy() const override {
        return std::unique_ptr<DistinctConstraintState>{new DistinctConstraintState(*this)};
    }

    //! Add an element whose bound has changed to the todo list and mark it as
    //! dirty.
    [[nodiscard]] bool update(val_t i, val_t diff) override {
        static_cast<void>(diff);
        uint32_t idx = std::abs(i) - 1;

        mark_dirty_(idx);
        mark_todo_(i, idx);
        return true;
    }

    void undo(val_t i, val_t diff) override {
        static_cast<void>(diff);
        uint32_t idx = std::abs(i) - 1;

        // mark given element as dirty
        mark_dirty_(idx);

        // clear todo lists
        clear_todo_();
    }

    //! See `_propagate` for exmamples what is propagated.
    [[nodiscard]] bool propagate(Solver &solver, AbstractClauseCreator &cc, bool check_state) override {
        static_cast<void>(check_state); // TODO: the state could still be checked...
        update_(solver);

        for (auto i : todo_lower_) {
            auto [lower, upper] = assigned_[i];
            if (lower == upper) {
                if (!propagate_assigned_(solver, cc, lower, i)) {
                    return false;
                }
            }
            else {
                for (auto it = lower_.lower_bound({lower, 0}), ie = lower_.lower_bound({lower+1, 0}); it != ie; ++it) {
                    auto j = it->second;
                    if (assigned_[j].first == assigned_[j].second) {
                        if (in_todo_lower_[j] || in_todo_upper_[j]) {
                            break;
                        }
                        if (!propagate_(solver, cc, -1, j, i)) {
                            return false;
                        }
                        break;
                    }
                }
            }
        }

        for (auto i : todo_upper_) {
            auto [lower, upper] = assigned_[i];
            if (lower == upper) {
                if (!propagate_assigned_(solver, cc, lower, i)) {
                    return false;
                }
            }
            else {
                for (auto it = upper_.lower_bound({upper, 0}), ie = upper_.lower_bound({upper+1, 0}); it != ie; ++it) {
                    auto j = it->second;
                    if (assigned_[j].first == assigned_[j].second) {
                        if (in_todo_lower_[j] || in_todo_upper_[j]) {
                            break;
                        }
                        if (!propagate_(solver, cc, 1, j, i)) {
                            return false;
                        }
                        break;
                    }
                }
            }
        }

        clear_todo_();

        return true;
    }

    void check_full(Solver &solver) override {
        std::set<sum_t> values;
        for (auto const &element : constraint_)  {
            sum_t value = element.fixed();
            for (auto [co, var] : element) {
                auto &vs = solver.var_state(var);
                if (!vs.is_assigned()) {
                    throw std::logic_error("variable is not fully assigned");
                }
                value += static_cast<sum_t>(co) * vs.lower_bound();
            }
            if (!values.emplace(value).second) {
                throw std::logic_error("invalid distinct constraint");
            }
        }
    }

    [[nodiscard]] bool mark_todo(bool todo) override {
        auto ret = todo_;
        todo_ = todo;
        return ret;
    }

    [[nodiscard]] bool marked_todo() const override {
        return todo_;
    }

    [[nodiscard]] bool removable() override {
        return true;
    }

protected:
    [[nodiscard]] level_t inactive_level() const override {
        return inactive_level_;
    }

    void inactive_level(level_t level) override {
        inactive_level_ = level;
    }

private:
    //! Introduce a variable and make it equal to the term.
    [[nodiscard]] DiffElement var_(Config const &config, Solver &solver, uint32_t idx, ConstraintVec &added) {
        auto [lower, upper] = assigned_[idx];
        auto const &elements = constraint_[idx];

        assert(!elements.empty());
        if (elements.size() == 1) {
            return {elements.fixed(), elements.begin()->first, elements.begin()->second};
        }

        auto var = solver.add_variable(lower, upper);
        CoVarVec sum_elems;
        sum_elems.emplace_back(-1, var);
        sum_elems.insert(sum_elems.end(), elements.begin(), elements.end());
        added.emplace_back(SumConstraint::create(TRUE_LIT, -elements.fixed(), sum_elems, config.sort_constraints));
        for (auto &co_var : sum_elems) {
            co_var.first = -co_var.first;
        }
        added.emplace_back(SumConstraint::create(TRUE_LIT, elements.fixed(), sum_elems, config.sort_constraints));

        return {0, 1, var};
    }

    //! Estimate the size of the translation in terms of the number of weight
    //! constraints and return whether the constraint should be translated.
    //!
    //! Since terms might be represented by by auxiliary variables, we also
    //! make sure that they can be represented by auxiliary variables.
    bool estimate_(sum_t maximum) {
        sum_t cost = 0;

        IntervalSet<sum_t> intervals;
        for (auto [lower, upper] : assigned_) {
            if (lower < MIN_VAL || upper > MAX_VAL) {
                return false;
            }
            intervals.add(lower, upper);
        }
        for (auto [lower, upper] : intervals) {
            cost += upper - lower;
        }

        return cost < maximum;
    }

    //! Calculate the domain of a term.
    IntervalSet<sum_t> domain_(Solver &solver, uint32_t idx) {
        IntervalSet<sum_t> values;
        auto lower = assigned_[idx].first;

        values.add(lower, lower+1);
        for (auto [co, var] : constraint_[idx]) {
            IntervalSet<sum_t> current{values};
            auto &vs = solver.var_state(var);
            sum_t add = std::abs(co);
            for (val_t i = vs.lower_bound(), e = vs.upper_bound(); i != e; ++i) {
                for (auto [l, u] : current) {
                    values.add(l + add, u + add);
                }
                add += std::abs(co);
            }
        }

        return values;
    }

    bool propagate_assigned_(Solver &solver, AbstractClauseCreator &cc, sum_t value, uint32_t idx) {
        for (auto it = upper_.lower_bound({value, 0}), ie = upper_.lower_bound({value+1, 0}); it != ie; ++it) {
            assert(value == it->first);
            if (it->second != idx && !propagate_(solver, cc, 1, idx, it->second)) {
                return false;
            }
        }
        for (auto it = lower_.lower_bound({value, 0}), ie = lower_.lower_bound({value+1, 0}); it != ie; ++it) {
            assert(value == it->first);
            if (it->second != idx && !propagate_(solver, cc, -1, idx, it->second)) {
                return false;
            }
        }
        return true;

    }

    //! Propagate a distinct constraint assuming that element i is assigned and
    //! one of element element j's bounds as determined by s match the value of
    //! element i.
    //!
    //! The reasons generated by this function are not necessarily unit.
    //! Implementing proper unit propagation for arbitrary linear terms would
    //! be quite involved. This implementation still works because it
    //! guarantees conflict detection and the added constraints might become
    //! unit later. The whole situation could be simplified by restricting the
    //! syntax of distinct constraints.
    //!
    //! case s > 0:
    //!   example: x != y+z
    //!     x <= 10 & not x <= 9 & y <= 5 & z <= 5 => y <= 4 | z <= 4
    //!   example: x != -y
    //!     x <= 9 & not x <= 8 &     y >= -9  =>     y >= -8
    //!     x <= 9 & not x <= 8 & not y <= -10 => not y <= -9
    //! case s < 0:
    //!   example: x != y
    //!     x = 9 & y >= 9 => y >= 10
    //!     x <= 9 & not x <= 8 & not y <= 8 => not y <= 9
    //!   example: x != -y
    //!     x <= 9 & not x <= 8 & y <= -9 => y <= -10
    bool propagate_(Solver &solver, AbstractClauseCreator &cc, int s, uint32_t i, uint32_t j) {
        auto ass = cc.assignment();

        auto &reason = solver.temp_reason();
        auto const &elem_i = constraint_[i];
        auto const &elem_j = constraint_[j];
        bool is_fact = elem_j.size() == 1;

        auto lit = -constraint_.literal();
        if (!ass.is_fixed(lit)) {
            reason.emplace_back(lit);
        }

        // assigned index
        for (auto [co, var] : elem_i) {
            static_cast<void>(co);
            auto &vs = solver.var_state(var);
            assert(vs.is_assigned());

            auto lit = -solver.get_literal(cc, vs, vs.upper_bound());
            assert(ass.is_false(lit));
            if (!ass.is_fixed(lit)) {
                reason.emplace_back(lit);
            }

            lit = solver.get_literal(cc, vs, vs.lower_bound() - 1);
            assert(ass.is_false(lit));
            if (!ass.is_fixed(lit)) {
                reason.emplace_back(lit);
            }
        }

        // bounded index
        for (auto [co, var] : elem_j) {
            auto &vs = solver.var_state(var);
            if (s * co > 0) {
                lit = -solver.get_literal(cc, vs, vs.upper_bound());
                assert(ass.is_false(lit));
                if (!ass.is_fixed(lit)) {
                    reason.emplace_back(lit);
                }

                // add consequence
                lit = solver.update_literal(cc, vs, vs.upper_bound() - 1, is_fact && reason.empty() ? Clingo::TruthValue::True : Clingo::TruthValue::Free);
                if (ass.is_true(lit)) {
                    return true;
                }
                reason.emplace_back(lit);
            }
            else {
                lit = solver.get_literal(cc, vs, vs.lower_bound() - 1);
                assert(ass.is_false(lit));
                if (!ass.is_fixed(lit)) {
                    reason.emplace_back(lit);
                }

                lit = -solver.update_literal(cc, vs, vs.lower_bound(), is_fact && reason.empty() ? Clingo::TruthValue::False : Clingo::TruthValue::Free);
                if (ass.is_true(lit)) {
                    return true;
                }
                reason.emplace_back(lit);
            }
        }

        return cc.add_clause(reason);
    }

    void mark_dirty_(uint32_t idx) {
        if (!in_dirty_[idx]) {
            in_dirty_[idx] = true;
            dirty_.emplace_back(idx);
        }
    }

    void mark_todo_(val_t sign, uint32_t idx) {
        if (sign > 0) {
            if (!in_todo_lower_[idx]) {
                in_todo_lower_[idx] = true;
                todo_lower_.emplace_back(idx);
            }
        }
        else {
            if (!in_todo_upper_[idx]) {
                in_todo_upper_[idx] = true;
                todo_upper_.emplace_back(idx);
            }
        }
    }

    void clear_todo_() {
        for (auto j : todo_lower_) {
            in_todo_lower_[j] = false;
        }
        todo_lower_.clear();

        for (auto j : todo_upper_) {
            in_todo_upper_[j] = false;
        }
        todo_upper_.clear();
    }

    //! Recalculates the bounds of the i-th element of the constraint assuming
    //! that the bounds of this element are not currently in the bound maps.
    [[nodiscard]] std::pair<sum_t, sum_t> init_(Solver &solver, uint32_t idx) {
        // calculate new values
        auto const &element = constraint_[idx];
        sum_t upper = element.fixed();
        sum_t lower = element.fixed();
        for (auto [co, var] : element) {
            auto &vs = solver.var_state(var);
            if (co > 0) {
                upper += static_cast<sum_t>(co) * vs.upper_bound();
                lower += static_cast<sum_t>(co) * vs.lower_bound();
            }
            else {
                upper += static_cast<sum_t>(co) * vs.lower_bound();
                lower += static_cast<sum_t>(co) * vs.upper_bound();
            }
        }
        // set new values
        assigned_[idx] = {lower, upper};
        return {lower, upper};
    }

    //! Recalculate all elements marked dirty.
    void update_(Solver &solver) {
        for (auto i : dirty_) {
            auto [old_lower, old_upper] = assigned_[i];
            auto node_lower = lower_.extract(std::pair{old_lower, i});
            auto node_upper = upper_.extract(std::pair{old_upper, i});
            auto [new_lower, new_upper] = init_(solver, i);
            node_lower.value().first = new_lower;
            node_upper.value().first = new_upper;
            lower_.insert(std::move(node_lower));
            upper_.insert(std::move(node_upper));
            in_dirty_[i] = false;
        }
        dirty_.clear();
    }

    DistinctConstraintState(DistinctConstraintState const &x)
    : AbstractConstraintState{} // NOLINT
    , constraint_{x.constraint_}
    , assigned_{x.assigned_}
    , dirty_{x.dirty_}
    , todo_upper_{x.todo_upper_}
    , todo_lower_{x.todo_lower_}
    , in_dirty_{x.in_dirty_}
    , in_todo_upper_{x.in_todo_upper_}
    , in_todo_lower_{x.in_todo_lower_}
    , lower_{x.lower_}
    , upper_{x.upper_}
    , inactive_level_{x.inactive_level_}
    , todo_{x.todo_} {
    }

    DistinctConstraint &constraint_;
    // TODO: The members assigned_, dirty_, todo_upper_, todo_lower_,
    // in_dirty_, in_todo_upper_, in_todo_lower_ all have predetermined sizes
    // and can be packed into contiguous memory. Maybe even the sets can be
    // avoided. The best data structures and algorithms for distinct
    // propagation should be explored in the literature.
    std::vector<std::pair<sum_t, sum_t>> assigned_;
    std::vector<uint32_t> dirty_;
    std::vector<uint32_t> todo_upper_;
    std::vector<uint32_t> todo_lower_;
    std::vector<bool> in_dirty_;
    std::vector<bool> in_todo_upper_;
    std::vector<bool> in_todo_lower_;
    std::set<std::pair<sum_t, uint32_t>> lower_;
    std::set<std::pair<sum_t, uint32_t>> upper_;
    level_t inactive_level_{0};
    bool todo_{false};
};

//! Capture the state of a disjoint constraint.
class DisjointConstraintState final : public AbstractConstraintState {
    struct Interval {
        size_t var;
        val_t left;
        val_t right;
        val_t last_left;
        val_t last_right;
        val_t weight;
        val_t u;
    };

    enum class PropagateType { Lower, Upper };
    template <PropagateType type>
    struct Algorithm { // NOLINT(cppcoreguidelines-pro-type-member-init)
        using It = std::vector<Interval>::iterator;

        [[nodiscard]] static val_t lower(Interval const &i) {
            if constexpr (type == PropagateType::Lower) {
                return i.left;
            }
            else {
                return -i.right;
            }
        }

        [[nodiscard]] static val_t upper(Interval const &i) {
            if constexpr (type == PropagateType::Lower) {
                return i.right;
            }
            else {
                return -i.left;
            }
        }

        [[nodiscard]] static val_t lower(It i) {
            return lower(*i);
        }

        [[nodiscard]] static val_t upper(It i) {
            return upper(*i);
        }

        [[nodiscard]] static val_t weight(It i) {
            return i->weight;
        }

        [[nodiscard]] static val_t &u(It i) {
            return i->u;
        }

        [[nodiscard]] static bool changed(Interval const &x) {
            return x.left != x.last_left || x.right != x.last_right;
        }

        [[nodiscard]] bool update_bound(std::vector<lit_t> &reason, It i, val_t b) {
            auto &vs = solver.var_state(i->var);
            if constexpr (type == PropagateType::Lower) {
                reason.emplace_back(solver.get_literal(cc, vs, vs.lower_bound() - 1));
                // Note: we cap the new lower bound here to the upper bound to
                // avoid introducing variables above the upper bound. It is
                // possible to introduce a new variable but it would have to be
                // implied by the closest order literal to not have it dangling
                // around later. And it should only be done on the current
                // decision level to avoid backtracking with out a conflict.
                //
                // See for example the sum constraint state, which refines
                // reasons in the hope of getting better conflict but making
                // sure to avoid the above mentioned cases.
                auto val = std::min(b - 1, vs.upper_bound());
                auto lit = -solver.update_literal(cc, vs, val, reason.empty() ? Clingo::TruthValue::False : Clingo::TruthValue::Free);
                reason.emplace_back(lit);
                i->last_left = b;
            }
            else {
                reason.emplace_back(-solver.get_literal(cc, vs, vs.upper_bound()));
                // Note: similar to the note above.
                auto val = std::max(-b - weight(i) + 1, vs.lower_bound() - 1);
                auto lit = solver.update_literal(cc, vs, val, reason.empty() ? Clingo::TruthValue::True : Clingo::TruthValue::Free);
                reason.emplace_back(lit);
                i->last_right = -b;
            }
            return cc.add_clause(reason);
        }

        [[nodiscard]] std::vector<lit_t> &calculate_reason(val_t a, It j) {
            auto ass = cc.assignment();
            auto &reason = solver.temp_reason();
            if (!ass.is_fixed(clit)) {
                reason.emplace_back(-clit);
            }
            for (auto i = begin; i != j; ++i) {
                // Since [a, b] is a maximal hall interval, the reason
                // includes all variables contained in the range [a, b].
                // Because b is the current upper bound, all upper
                // bounds of intervals k are smaller or equal than b.
                if (lower(i) >= a) {
                    auto &vs = solver.var_state(i->var);
                    auto l = solver.get_literal(cc, vs, vs.lower_bound() - 1);
                    auto u = -solver.get_literal(cc, vs, vs.upper_bound());
                    if (!ass.is_fixed(l)) {
                        reason.emplace_back(l);
                    }
                    if (!ass.is_fixed(u)) {
                        reason.emplace_back(u);
                    }
                }
            }
            return reason;
        }

        //! Propagate hall interval [a, b].
        [[nodiscard]] bool propagate(val_t a, val_t b, It i) {
            std::vector<lit_t> *reason{nullptr};
            size_t size{0};
            for (auto j = i + 1; j != end; ++j) {
                if (lower(j) >= a && lower(j) <= b) {
                    if (reason == nullptr) {
                        reason = &calculate_reason(a, i + 1);
                        size = reason->size();
                    }
                    if (!update_bound(*reason, j, b + 1)) {
                        return false;
                    }
                    reason->resize(size);
                }
            }
            return true;
        }

        [[nodiscard]] bool insert(It i) {
            auto k = end;

            i->last_left = i->left;
            i->last_right = i->right;

            u(i) = lower(i) + weight(i) - 1;

            for (auto j = begin; j != i; ++j) {
                if (lower(j) < lower(i)) {
                    u(j) += weight(i);
                    if (u(j) > upper(i)) {
                        return cc.add_clause(calculate_reason(lower(j), i + 1));
                    }
                    if (u(j) == upper(i) && (k == end || lower(j) < lower(k))) {
                        k = j;
                    }
                }
                else {
                    u(i) += weight(j);
                }
            }

            if (u(i) > upper(i)) {
                return cc.add_clause(calculate_reason(lower(i), i + 1));
            }
            if (u(i) == upper(i) && (k == end || lower(i) < lower(k))) {
                k = i;
            }

            return k == end || propagate(lower(k), upper(i), i);
        }

        [[nodiscard]] bool propagate() {
            for (auto i = begin; i != end; ++i) {
                if (!insert(i)) {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] static bool run(Solver &solver, AbstractClauseCreator &cc, lit_t clit, bool force_update, It begin, It end) {
            // Note: the constraint could also be disabled as soon as all
            // subsequences became singletons.
            std::sort(begin, end, [](auto const &a, auto const &b) { return upper(a) < upper(b); });
            for (auto it = std::make_reverse_iterator(end), ie = std::make_reverse_iterator(begin); it != ie; ) {
                val_t min = lower(*it);
                auto je = it.base();
                bool has_change = force_update || changed(*it);
                for (++it; it != ie && upper(*it) >= min; ++it) {
                    min = std::min(lower(*it), min);
                    has_change = has_change || changed(*it);
                }
                if (has_change && !Algorithm{solver, cc, it.base(), je, clit}.propagate()) {
                    return false;
                }
            }
            return true;
        }

        Solver &solver;
        AbstractClauseCreator &cc;
        It begin;
        It end;
        lit_t clit;
    };

public:
    DisjointConstraintState(DisjointConstraint &constraint)
    : constraint_{constraint} {
        intervals_.reserve(constraint.size());
        for (auto const &[val, var] : constraint_) {
            intervals_.emplace_back(Interval{var, 0, 0, 0, 0, val, 0});
        }
    }

    DisjointConstraintState(DisjointConstraintState &&) = delete;
    DistinctConstraintState &operator=(DisjointConstraintState const &) = delete;
    DisjointConstraintState &operator=(DisjointConstraintState &&) = delete;
    ~DisjointConstraintState() override = default;

    DisjointConstraint& constraint() override {
        return constraint_;
    }

    void attach(Solver &solver) override {
        for (auto const &element : constraint_) {
            solver.add_var_watch(element.second, 1, *this);
        }
    }

    void detach(Solver &solver) override {
        for (auto const &element : constraint_) {
            solver.remove_var_watch(element.second, 1, *this);
        }
    }

    [[nodiscard]] std::pair<bool, bool> translate(Config const &config, Solver &solver, InitClauseCreator &cc, ConstraintVec &added) override {
        static_cast<void>(config);
        static_cast<void>(solver);
        static_cast<void>(cc);
        static_cast<void>(added);
        return {true, false};
    }

    [[nodiscard]] UniqueConstraintState copy() const override {
        return std::unique_ptr<DisjointConstraintState>{new DisjointConstraintState(*this)};
    }

    [[nodiscard]] bool update(val_t i, val_t diff) override {
        static_cast<void>(i);
        static_cast<void>(diff);
        return true;
    }

    void undo(val_t i, val_t diff) override {
        static_cast<void>(i);
        static_cast<void>(diff);
        force_update_ = true;
    }

    [[nodiscard]] bool propagate(Solver &solver, AbstractClauseCreator &cc, bool check_state) override {
        // Note: One could even track whole partitions but currently the
        // sorting does not dominate the runtime. Maybe this would change with
        // a fast O(n*log(n)) propagation algorithm.
        static_cast<void>(check_state); // TODO: the state could still be checked...

        bool force_update = force_update_;
        force_update_ = false;

        for (auto &x : intervals_) {
            auto &vs = solver.var_state(x.var);
            x.left = vs.lower_bound();
            x.right = vs.upper_bound() + x.weight - 1;
        }

        return
            Algorithm<PropagateType::Lower>::run(solver, cc, constraint_.literal(), force_update, intervals_.begin(), intervals_.end()) &&
            Algorithm<PropagateType::Upper>::run(solver, cc, constraint_.literal(), force_update, intervals_.begin(), intervals_.end());
    }

    void check_full(Solver &solver) override {
        IntervalSet<val_t> assignment;
        for (auto [val, var] : constraint_) {
            auto &vs = solver.var_state(var);
            auto l = vs.lower_bound();
            auto u = vs.upper_bound() + val;
            if (assignment.intersects(l, u)) {
                throw std::logic_error("invalid assignment to distinct constraint");
            }
            assignment.add(l, u);
        }
    }

    [[nodiscard]] bool mark_todo(bool todo) override {
        auto ret = todo_;
        todo_ = todo;
        return ret;
    }

    [[nodiscard]] bool marked_todo() const override {
        return todo_;
    }

    [[nodiscard]] bool removable() override {
        return true;
    }

protected:
    [[nodiscard]] level_t inactive_level() const override {
        return inactive_level_;
    }

    void inactive_level(level_t level) override {
        inactive_level_ = level;
    }

private:
    DisjointConstraintState(DisjointConstraintState const &x)
    : AbstractConstraintState{} // NOLINT
    , constraint_{x.constraint_}
    , intervals_{x.intervals_}
    , inactive_level_{x.inactive_level_}
    , force_update_{x.force_update_}
    , todo_{x.todo_} {
    }

    DisjointConstraint &constraint_;
    std::vector<Interval> intervals_;
    level_t inactive_level_{0};
    bool force_update_{true};
    bool todo_{false};

};

} // namespace

UniqueConstraintState SumConstraint::create_state() {
    return std::make_unique<SumConstraintStateImpl<false, SumConstraintState>>(*this);
}

UniqueConstraintState MinimizeConstraint::create_state() {
    return std::make_unique<SumConstraintStateImpl<true, MinimizeConstraintState>>(*this);
}

DistinctElement::DistinctElement(val_t fixed, size_t size, co_var_t *elements, bool sort)
: fixed_{fixed}
, size_{static_cast<uint32_t>(size)}
, elements_{elements} {
    if (sort) {
        std::sort(elements_, elements_ + size_, [](auto a, auto b) { return std::abs(a.first) > std::abs(b.first); } ); // NOLINT
    }
}

// class DistinctConstraint

DistinctConstraint::DistinctConstraint(lit_t lit, Elements const &elements, bool sort)
: lit_{lit}
, size_{static_cast<uint32_t>(elements.size())} {
    auto *start = reinterpret_cast<unsigned char *>(elements_) + elements.size() * sizeof(DistinctElement); // NOLINT
    auto *co_var_it = reinterpret_cast<co_var_t*>(start); // NOLINT
    auto *element_it = elements_;
    for (auto const &element : elements) {
        auto *co_var_ib = co_var_it;
        for (auto const &co_var : element.first) {
            new (co_var_it++) co_var_t{co_var}; // NOLINT
        }
        new (element_it++) DistinctElement{element.second, element.first.size(), co_var_ib, sort}; // NOLINT
    }
}

std::unique_ptr<DistinctConstraint> DistinctConstraint::create(lit_t lit, Elements const &elements, bool sort) {
    size_t size = sizeof(DistinctConstraint) + elements.size() * sizeof(DistinctElement);
    for (auto const &element : elements) {
        size += element.first.size() * sizeof(co_var_t);
    }
    return std::unique_ptr<DistinctConstraint>{new (operator new(size)) DistinctConstraint(lit, elements, sort)};
}

UniqueConstraintState DistinctConstraint::create_state() {
    return std::make_unique<DistinctConstraintState>(*this);
}

// class DisjointConstraint

DisjointConstraint::DisjointConstraint(lit_t lit, CoVarVec const &elements)
: lit_{lit}
, size_{static_cast<uint32_t>(elements.size())} {
    std::copy(elements.begin(), elements.end(), elements_);
}

std::unique_ptr<DisjointConstraint> DisjointConstraint::create(lit_t lit, CoVarVec const &elements) {
    auto size = sizeof(DisjointConstraint) + elements.size() * sizeof(co_var_t);
    return std::unique_ptr<DisjointConstraint>{new (operator new(size)) DisjointConstraint(lit, elements)};
}

UniqueConstraintState DisjointConstraint::create_state() {
    return std::make_unique<DisjointConstraintState>(*this);
}

} // namespace Clingcon
