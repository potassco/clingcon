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

#include "clingcon/propagator.hh"
#include "clingcon/parsing.hh"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

namespace Clingcon {

namespace {

//! Warning printed if the application does not call Propagator::step after a
//! solve call that needs one.
constexpr char const *STEP_MISSING_WARNING =
    "warning: clingcon: the previous solve call was not followed by clingcon_step; the optimization is incomplete";

//! Warning printed if the levels are folded into one minimize constraint
//! while a &heuristic directive is scoped to a level.
constexpr char const *SCOPED_ANNOTATION_WARNING =
    "warning: clingcon: &heuristic directives scoped to a level do not act in weighted mode";

//! Error reported if the levels do not fit into one weighted minimize
//! constraint.
[[nodiscard]] auto fold_error(char const *reason) -> std::runtime_error {
    return std::runtime_error(std::string{"the optimization levels cannot be folded into one minimize constraint: "} +
                              reason + "; --lex-opt=multi-shot optimizes levels of any range");
}

//! Add `a * b` to `acc`, where all three are non-negative.
//!
//! Returns false if the result does not fit into Clingcon::sum_t.
[[nodiscard]] auto add_mul(sum_t &acc, sum_t a, sum_t b) -> bool {
    if (a != 0 && b > std::numeric_limits<sum_t>::max() / a) {
        return false;
    }
    auto product = a * b;
    if (acc > std::numeric_limits<sum_t>::max() - product) {
        return false;
    }
    acc += product;
    return true;
}

//! CSP builder to use with the parse_theory function.
class ConstraintBuilder final : public AbstractConstraintBuilder {
  public:
    ConstraintBuilder(Propagator &propagator, InitClauseCreator &cc, UniqueMinimizeConstraint minimize)
        : propagator_{propagator}, cc_{cc}, minimize_{std::move(minimize)} {}

    ConstraintBuilder(ConstraintBuilder const &) = delete;
    ConstraintBuilder(ConstraintBuilder &&) noexcept = delete;
    auto operator=(ConstraintBuilder const &) -> ConstraintBuilder & = delete;
    auto operator=(ConstraintBuilder &&) noexcept -> ConstraintBuilder & = delete;
    ~ConstraintBuilder() override = default;

    [[nodiscard]] auto solver_literal(lit_t literal) -> lit_t override { return cc_.solver_literal(literal); }
    [[nodiscard]] auto add_literal() -> lit_t override { return cc_.add_literal(); }
    [[nodiscard]] auto is_true(lit_t literal) -> bool override { return cc_.assignment().is_true(literal); }
    [[nodiscard]] auto add_clause(Clingo::LiteralSpan clause) -> bool override { return cc_.add_clause(clause); }
    void add_show() override { propagator_.show(); }
    void show_signature(char const *name, size_t arity) override { propagator_.show_signature(name, arity); }
    void show_variable(var_t var) override { propagator_.show_variable(var); }
    [[nodiscard]] auto add_variable(Clingo::Symbol sym) -> var_t override { return propagator_.add_variable(sym); }
    [[nodiscard]] auto add_constraint(lit_t lit, CoVarVec const &elems, val_t rhs, bool strict) -> bool override {
        if (!strict && cc_.assignment().is_false(lit)) {
            return true;
        }

        if (elems.size() == 1) {
            auto [co, var] = elems.front();
            return propagator_.add_simple(cc_, lit, co, var, rhs, strict);
        }

        propagator_.add_constraint(SumConstraint::create(lit, rhs, elems, propagator_.config().sort_constraints));
        if (strict) {
            CoVarVec ielems;
            ielems.reserve(elems.size());
            for (auto const &elem : elems) {
                ielems.emplace_back(safe_inv(elem.first), elem.second);
            }
            propagator_.add_constraint(
                SumConstraint::create(-lit, safe_inv(safe_add(rhs, 1)), ielems, propagator_.config().sort_constraints));
        }
        return true;
    }

    [[nodiscard]] auto add_nonlinear(lit_t lit, val_t co_ab, var_t var_a, var_t var_b, val_t co_c, var_t var_c,
                                     val_t rhs, bool strict) -> bool override {
        if (co_ab == 0) {
            CoVarVec vars;
            if (co_c != 0) {
                vars.emplace_back(co_c, var_c);
            }
            return add_constraint(lit, vars, rhs, strict);
        }
        propagator_.add_constraint(std::make_unique<NonlinearConstraint>(lit, co_ab, var_a, var_b, co_c, var_c, rhs));
        if (strict) {
            propagator_.add_constraint(std::make_unique<NonlinearConstraint>(
                lit, safe_inv(co_ab), var_a, var_b, safe_inv(co_c), var_c, safe_inv(safe_add(rhs, 1))));
        }
        return true;
    }

    void add_minimize(val_t co, var_t var, int level) override {
        minimize_elems_.push_back({co, var, level});
        if (var != INVALID_VAR) {
            levels_.insert(level);
        }
    }

    void add_search_annotation(SearchAnnotation annotation) override {
        search_annotations_.emplace_back(std::move(annotation));
    }

    //! Add a distinct constraint.
    //!
    //! Binary distinct constraints will be represented with a sum constraint.
    [[nodiscard]] auto add_distinct(lit_t lit, std::vector<std::pair<CoVarVec, val_t>> const &elems) -> bool override {
        auto truth = cc_.assignment().truth_value(lit);
        if (truth == Clingo::TruthValue::False) {
            return true;
        }

        if (elems.size() > 2) {
            propagator_.add_constraint(DistinctConstraint::create(lit, elems, propagator_.config().sort_constraints));
            return true;
        }

        // Note: even though translation is also handled in constraints, it is
        // easier to do it here right away because at this point we can also
        // add trivial constraints that will be simplified further later on.
        // The only advantage of doing it in the constraint would be that we
        // potentially have to introduce fewer literals.
        CoVarVec celems;
        for (auto it = elems.begin(), ie = elems.end(); it != ie; ++it) {
            for (auto jt = it + 1; jt != ie; ++jt) {
                auto rhs = jt->second - it->second;
                celems.assign(it->first.begin(), it->first.end());
                for (auto [co, var] : jt->first) {
                    celems.emplace_back(-co, var);
                }
                rhs += simplify(celems, true);

                if (celems.empty()) {
                    if (rhs == 0) {
                        return cc_.add_clause({-lit});
                    }
                    continue;
                }

                auto a = cc_.add_literal();
                auto b = -a;
                if (truth != Clingo::TruthValue::True) {
                    b = cc_.add_literal();
                    if (!cc_.add_clause({a, b, -lit})) {
                        return false;
                    }
                    if (!cc_.add_clause({-a, -b})) {
                        return false;
                    }
                    if (!cc_.add_clause({lit, -a})) {
                        return false;
                    }
                    if (!cc_.add_clause({lit, -b})) {
                        return false;
                    }
                }

                if (!add_constraint(a, celems, check_valid_value(rhs - 1), false)) {
                    return false;
                }
                for (auto &co_var : celems) {
                    co_var.first = -co_var.first;
                }
                if (!add_constraint(b, celems, check_valid_value(-rhs - 1), false)) {
                    return false;
                }
            }
        }

        return true;
    }

    static auto translate_disjoint_(var_t const &i, var_t const &j, val_t rhs) -> std::tuple<lit_t, CoVarVec, val_t> {
        CoVarVec elems;
        elems.emplace_back(1, i);
        elems.emplace_back(-1, j);
        rhs += simplify(elems);
        lit_t lit = 0;
        if (elems.empty()) {
            lit = rhs >= 0 ? TRUE_LIT : -TRUE_LIT;
        }
        return {lit, elems, rhs};
    }

    auto translate_disjoint_(lit_t &lit, CoVarVec const &elems, val_t rhs) -> bool {
        if (lit == 0) {
            lit = add_literal();
            if (!add_constraint(lit, elems, rhs, true)) {
                return false;
            }
        }
        return true;
    }

    auto translate_disjoint_(lit_t lit, co_var_t const &i, co_var_t const &j) -> bool {
        assert(i.first > 0 && j.first > 0);

        // lower_i >= lower_j    (lower_j - lower_i <= 0)
        auto [lit_a, elems_a, rhs_a] = translate_disjoint_(j.second, i.second, 0);
        if (lit_a == -TRUE_LIT) {
            return true;
        }
        // lower_i <= upper_j    (lower_i - upper_j <= 0)
        auto [lit_b, elems_b, rhs_b] = translate_disjoint_(i.second, j.second, j.first - 1);
        if (lit_b == -TRUE_LIT) {
            return true;
        }

        if (!translate_disjoint_(lit_a, elems_a, rhs_a)) {
            return false;
        }
        if (!translate_disjoint_(lit_b, elems_b, rhs_b)) {
            return false;
        }

        return cc_.add_clause({-lit, -lit_a, -lit_b});
    }

    [[nodiscard]] auto add_disjoint(lit_t lit, CoVarVec const &elems) -> bool override {
        if (cc_.assignment().is_false(lit)) {
            return true;
        }

        if (elems.size() > 2) {
            propagator_.add_constraint(DisjointConstraint::create(lit, elems));
            return true;
        }

        CoVarVec celems;
        for (auto it = elems.begin(), ie = elems.end(); it != ie; ++it) {
            for (auto jt = it + 1; jt != ie; ++jt) {
                // TODO: this is a really bad translation. The following would
                // be way better:
                //
                //     c = add_literal()
                //     c => start_i >= end_j
                //     -c => start_j >= end_i
                //
                // But currently it is
                //
                //     :- start_i >= start_j, start_i <= end_j.
                //     :- start_j >= start_i, start_j <= end_i.
                //
                // using *strict* constraints.
                if (!translate_disjoint_(lit, *it, *jt) || !translate_disjoint_(lit, *jt, *it)) {
                    return false;
                }
            }
        }

        return true;
    }

    [[nodiscard]] auto add_dom(lit_t lit, var_t var, IntervalSet<val_t> const &elems) -> bool override {
        return cc_.assignment().is_false(lit) || propagator_.add_dom(cc_, lit, var, elems);
    }

    //! Get the &heuristic directives of the program.
    auto search_annotations() -> std::vector<SearchAnnotation> & { return search_annotations_; }

    //! Check whether the minimize constraint has elements on more than one level.
    [[nodiscard]] auto has_leveled_minimize() const -> bool { return levels_.size() > 1; }

    //! The levels of the elements over variables that the program part adds.
    [[nodiscard]] auto levels() const -> std::set<int> const & { return levels_; }

    //! Check whether the program part adds elements to the minimize constraint.
    [[nodiscard]] auto has_minimize_elements() const -> bool { return !minimize_elems_.empty(); }

    //! Check whether the levels are optimized in separate solve calls.
    //!
    //! Multi-shot needs the application to drive the solve calls. Without it,
    //! and with more than one thread, the levels are folded into one weighted
    //! minimize constraint that a portfolio of threads can work on together.
    [[nodiscard]] auto use_multi_shot(uint32_t n_threads) const -> bool {
        auto mode = propagator_.config().lex_opt;
        if (mode == LexOptMode::Weighted || !propagator_.steps_enabled()) {
            return false;
        }
        return mode == LexOptMode::MultiShot || n_threads <= 1;
    }

    //! Create one minimize constraint per level, highest level first.
    auto create_level_constraints() -> std::vector<std::pair<int, UniqueMinimizeConstraint>> {
        std::vector<std::pair<int, UniqueMinimizeConstraint>> result;
        auto by_level = group_by_level_();
        for (auto it = by_level.rbegin(), ie = by_level.rend(); it != ie; ++it) {
            // the elements of a level are parsed ones, whose coefficients are values
            CoVarVec elems;
            elems.reserve(it->second.size());
            for (auto const &elem : it->second) {
                elems.emplace_back(static_cast<val_t>(elem.co), elem.var);
            }
            auto adjust = simplify(elems, true);
            result.emplace_back(it->first, MinimizeConstraint::create(adjust, SumVarVec{elems.begin(), elems.end()},
                                                                      propagator_.config().sort_constraints));
        }
        return result;
    }

    //! Fold the levels into one minimize constraint using mixed-radix weights.
    //!
    //! The weight of the lowest level is one and the weight of each following
    //! level is the weight of the level below times its value range plus one.
    //! This throws if the weights, the value the folded constraint can take,
    //! or a weighted coefficient get out of range.
    void flatten_leveled_minimize(Solver &solver) {
        auto weights = level_weights_(solver);

        // A variable that occurs on several levels gets one element with the
        // sum of its weighted coefficients. The propagation of a constraint
        // relies on every variable occurring once. The sums are formed in a
        // wider type so that a coefficient beyond the range of the cost is
        // detected instead of overflowing.
        std::map<var_t, nsum_t> merged;
        nsum_t constant = 0;
        for (auto const &elem : minimize_elems_) {
            auto weighted = nsum_t{elem.co} * nsum_t{weights[elem.level]};
            if (elem.var == INVALID_VAR) {
                constant += weighted;
            } else {
                merged[elem.var] += weighted;
            }
        }
        std::vector<MinimizeElem> flat;
        flat.reserve(merged.size() + 1);
        for (auto const &[var, co] : merged) {
            if (co < -MAX_COST || co > MAX_COST) {
                throw fold_error("a weighted coefficient does not fit into the cost");
            }
            if (co != 0) {
                flat.push_back({static_cast<sum_t>(co), var, 0});
            }
        }
        if (constant < -MAX_COST || constant > MAX_COST) {
            throw fold_error("the weighted constant does not fit into the cost");
        }
        if (constant != 0) {
            flat.push_back({static_cast<sum_t>(constant), INVALID_VAR, 0});
        }
        minimize_elems_ = std::move(flat);
        levels_.clear();
    }

    //! Prepare the minimize constraint.
    //!
    //! This returns nullptr if the levels are optimized in separate solve
    //! calls; their constraints are created with create_level_constraints.
    auto prepare_minimize(Solver &solver, uint32_t n_threads) -> UniqueMinimizeConstraint {
        auto fits = [](sum_t x) -> bool { return MIN_VAL <= x && x <= MAX_VAL; };

        // copy values of old minimize constraint
        bool folded = false;
        if (minimize_ != nullptr) {
            for (auto [co, var] : *minimize_) {
                minimize_elems_.push_back({co, var, 0});
                folded = folded || !fits(co);
            }
            // The adjustment is the negated constant of the objective.
            minimize_elems_.push_back({-minimize_->adjust(), INVALID_VAR, 0});
            folded = folded || !fits(minimize_->adjust());
        }

        if (minimize_elems_.empty()) {
            return std::move(minimize_);
        }

        if (has_leveled_minimize()) {
            if (use_multi_shot(n_threads)) {
                return nullptr;
            }
            flatten_leveled_minimize(solver);
            folded = true;
        }

        SumVarVec elems;
        elems.reserve(minimize_elems_.size());
        sum_t adjust = 0;
        if (folded) {
            // The elements of a folded constraint are merged per variable
            // already and their coefficients can exceed the range of a value,
            // so they are taken as they are. Only the constant elements are
            // moved into the adjustment.
            for (auto const &elem : minimize_elems_) {
                if (elem.var == INVALID_VAR) {
                    adjust = safe_sub(adjust, elem.co);
                } else {
                    elems.emplace_back(elem.co, elem.var);
                }
            }
        } else {
            CoVarVec parsed;
            parsed.reserve(minimize_elems_.size());
            for (auto const &elem : minimize_elems_) {
                parsed.emplace_back(static_cast<val_t>(elem.co), elem.var);
            }
            adjust = simplify(parsed, true);
            elems.assign(parsed.begin(), parsed.end());
        }
        minimize_ = MinimizeConstraint::create(adjust, elems, !folded && propagator_.config().sort_constraints);

        return std::move(minimize_);
    }

  private:
    //! An element of the minimize constraint together with its level.
    //!
    //! The coefficient is a sum because the weights of folded levels exceed
    //! the range of a value.
    struct MinimizeElem {
        sum_t co;
        var_t var;
        int level;
    };

    //! Group the elements of the minimize constraint by level.
    auto group_by_level_() -> std::map<int, std::vector<MinimizeElem>> {
        std::map<int, std::vector<MinimizeElem>> by_level;
        for (auto const &elem : minimize_elems_) {
            by_level[elem.level].push_back(elem);
        }
        return by_level;
    }

    //! Compute the weight of each level for the weighted mode.
    //!
    //! The weight of a level is the weight of the level below times its value
    //! range plus one, so that an improvement on a level outweighs any change
    //! on the levels below. The value the folded constraint accumulates is
    //! bounded by the magnitudes of the elements, which has to fit into the
    //! type of the cost.
    auto level_weights_(Solver &solver) -> std::map<int, sum_t> {
        // The value range and the largest magnitude of each level, ordered
        // from the lowest level to the highest one.
        struct LevelSize {
            int level;
            sum_t range;
            sum_t magnitude;
        };
        std::vector<LevelSize> sizes;
        for (auto const &[level, elems] : group_by_level_()) {
            LevelSize size{level, 0, 0};
            for (auto const &elem : elems) {
                if (elem.var == INVALID_VAR) {
                    continue;
                }
                auto const &vs = solver.var_state(elem.var);
                auto co = static_cast<sum_t>(std::abs(elem.co));
                auto lower = static_cast<sum_t>(vs.min_bound());
                auto upper = static_cast<sum_t>(vs.max_bound());
                if (!add_mul(size.range, co, upper - lower)) {
                    throw fold_error("the weight of a level does not fit into the cost");
                }
                if (!add_mul(size.magnitude, co, std::max(std::abs(lower), std::abs(upper)))) {
                    throw fold_error("the folded objective does not fit into the cost");
                }
            }
            sizes.push_back(size);
        }

        std::map<int, sum_t> weights;
        sum_t weight = 1;
        for (size_t i = 0, ie = sizes.size(); i != ie; ++i) {
            weights[sizes[i].level] = weight;
            if (i + 1 == ie) {
                break;
            }
            auto radix = sizes[i].range + 1;
            if (weight > std::numeric_limits<sum_t>::max() / radix) {
                throw fold_error("the weight of a level does not fit into the cost");
            }
            weight *= radix;
        }

        sum_t total = 0;
        for (auto const &size : sizes) {
            if (!add_mul(total, weights[size.level], size.magnitude) || total > MAX_COST) {
                throw fold_error("the folded objective does not fit into the cost");
            }
        }

        return weights;
    }

    Propagator &propagator_;
    InitClauseCreator &cc_;
    UniqueMinimizeConstraint minimize_;
    std::vector<MinimizeElem> minimize_elems_;
    std::vector<SearchAnnotation> search_annotations_;
    std::set<int> levels_; //!< the levels of the elements over variables
};

} // namespace

void Propagator::on_model(Clingo::Model &model) {
    has_model_ = true;
    std::vector<Clingo::Symbol> symbols_;
    for (auto [sym, var] : sym_map_) {
        if (shown(var)) {
            auto value = Clingo::Number(get_value(var, model.thread_id()));
            symbols_.emplace_back(Clingo::Function("__csp", {sym, value}));
        }
    }

    if (has_minimize()) {
        auto bound = get_minimize_value(model.thread_id());
        if (minimize_levels_.empty()) {
            auto value = Clingo::String(std::to_string(bound).c_str());
            symbols_.emplace_back(Clingo::Function("__csp_cost", {value}));
        } else {
            // With levels, the cost is the value of each level separated by
            // spaces, highest level first.
            auto &solver = solver_(model.thread_id());
            std::string cost;
            for (size_t i = 0, ie = minimize_levels_.size(); i != ie; ++i) {
                auto const &level = minimize_levels_[i];
                sum_t value = 0;
                if (level.locked) {
                    value = level.locked_value;
                } else if (level.ptr != nullptr) {
                    for (auto [co, var] : *level.ptr) {
                        value += static_cast<sum_t>(co) * solver.get_value(var);
                    }
                    value -= level.ptr->adjust();
                }
                level_costs_[i] = value;
                if (!cost.empty()) {
                    cost += " ";
                }
                cost += std::to_string(value);
            }
            auto value = Clingo::String(cost.c_str());
            symbols_.emplace_back(Clingo::Function("__csp_cost", {value}));
        }
        if (bound <= minimize_bound_.load(std::memory_order_relaxed)) {
            stats_step_.cost = bound;
            last_known_cost_ = bound;
            update_minimize(bound - 1);
        }
    }

    model.extend(symbols_);
}

void Propagator::on_statistics(Clingo::UserStatistics &step, Clingo::UserStatistics &accu) {
    stats_accu_.accu(stats_step_);
    add_statistics_(step, stats_step_);
    add_statistics_(accu, stats_accu_);
    stats_step_.reset();

    if (minimize_levels_.empty()) {
        return;
    }
    for (auto *root : {&step, &accu}) {
        add_level_statistics_(*root);
    }
}

void Propagator::add_level_statistics_(Clingo::UserStatistics &root) {
    using namespace Clingo;

    UserStatistics opt = root["Clingcon"].add_subkey("Optimization", StatisticsType::Map);
    opt.add_subkey("Levels", StatisticsType::Value).set_value(static_cast<double>(minimize_levels_.size()));
    opt.add_subkey("Active level", StatisticsType::Value).set_value(static_cast<double>(active_level_idx_));
    if (level_costs_.empty()) {
        return;
    }
    UserStatistics costs = opt.add_subkey("Level costs", StatisticsType::Array);
    costs.ensure_size(level_costs_.size(), StatisticsType::Value);
    for (size_t i = 0, ie = level_costs_.size(); i != ie; ++i) {
        costs[i].set_value(static_cast<double>(level_costs_[i]));
    }
}

void Propagator::add_statistics_(Clingo::UserStatistics &root, Statistics &stats) {
    using namespace Clingo;

    UserStatistics clingcon = root.add_subkey("Clingcon", StatisticsType::Map);

    if (stats.cost.has_value()) {
        clingcon.add_subkey("Cost", StatisticsType::Value).set_value(static_cast<double>(*stats.cost));
    }

    auto init_time = clingcon.add_subkey("Init time in seconds", StatisticsType::Map);
    init_time.add_subkey("Total", StatisticsType::Value).set_value(stats.time_init);
    init_time.add_subkey("Simplify", StatisticsType::Value).set_value(stats.time_simplify);
    init_time.add_subkey("Translate", StatisticsType::Value).set_value(stats.time_translate);

    auto problem = clingcon.add_subkey("Problem", StatisticsType::Map);
    problem.add_subkey("Constraints", StatisticsType::Value).set_value(static_cast<double>(stats.num_constraints));
    problem.add_subkey("Variables", StatisticsType::Value).set_value(static_cast<double>(stats.num_variables));
    problem.add_subkey("Clauses", StatisticsType::Value).set_value(static_cast<double>(stats.num_clauses));
    problem.add_subkey("Literals", StatisticsType::Value).set_value(static_cast<double>(stats.num_literals));

    auto translate = clingcon.add_subkey("Translate", StatisticsType::Map);
    translate.add_subkey("Constraints removed", StatisticsType::Value)
        .set_value(static_cast<double>(stats.translate_removed));
    translate.add_subkey("Constraints added", StatisticsType::Value)
        .set_value(static_cast<double>(stats.translate_added));
    translate.add_subkey("Clauses", StatisticsType::Value).set_value(static_cast<double>(stats.translate_clauses));
    translate.add_subkey("Weight constraints", StatisticsType::Value)
        .set_value(static_cast<double>(stats.translate_wcs));
    translate.add_subkey("Literals", StatisticsType::Value).set_value(static_cast<double>(stats.translate_literals));

    UserStatistics threads = clingcon.add_subkey("Thread", StatisticsType::Array);
    threads.ensure_size(std::distance(stats.solver_statistics.begin(), stats.solver_statistics.end()),
                        StatisticsType::Map);
    size_t i = 0;
    for (auto &solver_stat : stats.solver_statistics) {
        auto thread = threads[i++];

        auto time = thread.add_subkey("Time in seconds", StatisticsType::Map);
        auto total = solver_stat.time_propagate + solver_stat.time_check + solver_stat.time_undo;
        time.add_subkey("Total", StatisticsType::Value).set_value(total);
        time.add_subkey("Propagation", StatisticsType::Value).set_value(solver_stat.time_propagate);
        time.add_subkey("Check", StatisticsType::Value).set_value(solver_stat.time_check);
        time.add_subkey("Undo", StatisticsType::Value).set_value(solver_stat.time_undo);

        thread.add_subkey("Refined reason", StatisticsType::Value)
            .set_value(static_cast<double>(solver_stat.refined_reason));
        thread.add_subkey("Introduced reason", StatisticsType::Value)
            .set_value(static_cast<double>(solver_stat.introduced_reason));
        thread.add_subkey("Literals introduced", StatisticsType::Value)
            .set_value(static_cast<double>(solver_stat.literals));
        thread.add_subkey("Propagation cycles", StatisticsType::Value)
            .set_value(static_cast<double>(solver_stat.cycles));
    }
}

auto Propagator::add_variable(Clingo::Symbol sym) -> var_t {
    auto [it, ret] = sym_map_.emplace(sym, 0);

    if (ret) {
        it->second = master_().add_variable(config_.min_int, config_.max_int);
        var_map_.emplace(it->second, sym);
        ++stats_step_.num_variables;
    }

    return it->second;
}

void Propagator::show_variable(var_t var) { show_variable_.emplace(var); }

void Propagator::show_signature(char const *name, size_t arity) { show_signature_.emplace(name, arity); }

auto Propagator::add_dom(AbstractClauseCreator &cc, lit_t lit, var_t var, IntervalSet<val_t> const &domain) -> bool {
    return master_().add_dom(cc, lit, var, domain);
}

auto Propagator::add_simple(AbstractClauseCreator &cc, lit_t clit, val_t co, var_t var, val_t rhs, bool strict)
    -> bool {
    return master_().add_simple(cc, clit, co, var, rhs, strict);
}

void Propagator::add_constraint_(UniqueConstraint constraint) { constraints_.emplace_back(std::move(constraint)); }

void Propagator::add_constraint(UniqueConstraint constraint) {
    ++stats_step_.num_constraints;
    master_().add_constraint(*constraint);
    add_constraint_(std::move(constraint));
}

void Propagator::init(Clingo::PropagateInit &init) {
    init.set_check_mode(Clingo::PropagatorCheckMode::Partial);

    Timer timer{stats_step_.time_init};
    InitClauseCreator cc{init, stats_step_};

    if (step_due_) {
        std::cerr << STEP_MISSING_WARNING << "\n";
    }

    // remove minimize constraint
    UniqueMinimizeConstraint minimize{remove_minimize()};

    // The constraint of the active level belongs to that level. Give it back
    // instead of passing it to the builder, which discards it on a program
    // with levels and leaves the level with a dangling pointer.
    if (active_level_idx_ < minimize_levels_.size() && minimize != nullptr) {
        auto &active = minimize_levels_[active_level_idx_];
        active.constraint = std::move(minimize);
        active.ptr = active.constraint.get();
        minimize = nullptr;
    }

    // remove solve step local and fixed literals
    for (auto &solver : solvers_) {
        solver.update();
    }

    // add constraints
    ConstraintBuilder builder{*this, cc, std::move(minimize)};
    if (!parse(builder, init.theory_atoms())) {
        return;
    }

    // Collect the &heuristic directives. Later program parts add to the
    // directives of the earlier ones, and a directive over all variables
    // covers the variables of later parts as well.
    for (auto &ann : builder.search_annotations()) {
        search_annotations_.emplace_back(std::move(ann));
    }
    for (auto &ann : search_annotations_) {
        if (!ann.all_vars) {
            continue;
        }
        ann.vars.clear();
        ann.var_set.clear();
        ann.vars.reserve(var_map_.size());
        for (auto const &[var, sym] : var_map_) {
            static_cast<void>(sym);
            ann.vars.emplace_back(var);
        }
        ann.var_set.insert(ann.vars.begin(), ann.vars.end());
    }
    // Directives with a smaller priority are consulted first. A directive
    // scoped to an optimization level goes before an unscoped one of the same
    // priority.
    std::stable_sort(search_annotations_.begin(), search_annotations_.end(), [](auto const &a, auto const &b) -> auto {
        if (a.priority != b.priority) {
            return a.priority < b.priority;
        }
        return a.opt_level.has_value() && !b.opt_level.has_value();
    });

    // A library user may not call clingcon_validate_options, so the mode is
    // checked here as well.
    if (builder.has_leveled_minimize() && config_.lex_opt == LexOptMode::MultiShot && !steps_enabled_) {
        throw std::runtime_error(STEPS_REQUIRED_MESSAGE);
    }

    // The weighted mode folds the levels into one minimize constraint, so
    // there is no level for a scoped directive to be active on.
    if (builder.has_leveled_minimize() && !builder.use_multi_shot(static_cast<uint32_t>(init.number_of_threads())) &&
        std::any_of(search_annotations_.begin(), search_annotations_.end(),
                    [](auto const &ann) -> auto { return ann.opt_level.has_value(); })) {
        std::cerr << SCOPED_ANNOTATION_WARNING << "\n";
    }

    // get the master solver and make sure it stays valid
    solvers_.reserve(init.number_of_threads());
    auto &master = master_();

    // gather bounds of states in master
    for (auto it = solvers_.begin() + 1, ie = solvers_.end(); it != ie; ++it) {
        if (!master.update_bounds(cc, *it, config_.check_state)) {
            return;
        }
    }

    // Levels are set up from the first program part with an objective. A
    // later part can extend a minimize constraint without levels as long as
    // its elements stay on that level; it cannot add to a constraint that
    // has levels or folds them, and its elements would otherwise be merged
    // with the earlier ones as if they were on the same level.
    if (builder.has_minimize_elements() && !minimize_levels_seen_.empty()) {
        if (!minimize_levels_.empty() || minimize_levels_seen_.size() > 1) {
            throw std::runtime_error("the minimize constraint cannot be extended once it has optimization levels");
        }
        if (!builder.levels().empty() && builder.levels() != minimize_levels_seen_) {
            throw std::runtime_error("the minimize constraint cannot be extended on another optimization level");
        }
    }
    minimize_levels_seen_.insert(builder.levels().begin(), builder.levels().end());

    auto n_threads = static_cast<uint32_t>(init.number_of_threads());
    // create the constraints of the levels on the first initialization
    if (minimize_levels_.empty() && builder.has_leveled_minimize() && builder.use_multi_shot(n_threads)) {
        for (auto &[priority, constraint] : builder.create_level_constraints()) {
            MinimizeLevel level;
            level.priority = priority;
            level.ptr = constraint.get();
            level.constraint = std::move(constraint);
            minimize_levels_.push_back(std::move(level));
        }
        active_level_idx_ = 0;
        level_costs_.assign(minimize_levels_.size(), 0);
    }

    // activate the constraint of the level that is optimized next
    if (active_level_idx_ < minimize_levels_.size()) {
        auto &active = minimize_levels_[active_level_idx_];
        if (active.constraint != nullptr) {
            active.constraint->set_no_translate(true);
            add_minimize_(std::move(active.constraint));
            active.ptr = minimize_;
        }
    }

    // fix the value of the levels that have been optimized
    for (auto &level : minimize_levels_) {
        if (!level.locked || level.lock_added || level.ptr == nullptr) {
            continue;
        }
        // the constraint of a level has parsed elements, whose coefficients are values
        CoVarVec elems;
        elems.reserve(level.ptr->size());
        for (auto [co, var] : *level.ptr) {
            elems.emplace_back(static_cast<val_t>(co), var);
        }
        auto rhs = level.locked_value + level.ptr->adjust();
        if (MIN_VAL > rhs || rhs > MAX_VAL) {
            throw std::runtime_error("the optimum of an optimization level does not fit into a value, so the level "
                                     "cannot be fixed for the next solve call; --lex-opt=weighted has no such limit");
        }
        add_constraint(SumConstraint::create(TRUE_LIT, static_cast<val_t>(rhs), elems, config_.sort_constraints));
        level.lock_added = true;
    }

    // propagate the newly added constraints
    if (!simplify_(cc)) {
        return;
    }

    // prepare minimize constraint
    auto prepared = builder.prepare_minimize(master, n_threads);

    // translate (simple enough) constraints
    if (!translate_(cc, std::move(prepared))) {
        return;
    }

    // Translation destroys the constraints it replaces, and the origins of
    // the bounds point into constraints.
    for (auto &solver : solvers_) {
        solver.reset_bound_origins();
    }

    // watch all the remaining constraints
    for (auto &constraint : constraints_) {
        cc.add_watch(constraint->literal());
    }
    if (!cc.commit()) {
        return;
    }

    // copy order literals from master to other states
    auto n = static_cast<size_t>(init.number_of_threads());
    for (size_t i = solvers_.size(); i < n; ++i) {
        solvers_.emplace_back(config_.solver_config(i), stats_step_.solver_stats(i));
    }
    while (solvers_.size() > n) {
        solvers_.pop_back();
    }
    master.shrink_to_fit();
    for (auto it = solvers_.begin() + 1, ie = solvers_.end(); it != ie; ++it) {
        it->copy_state(master);
    }

    // If there is a minimize constraint we have to enable total checks subject
    // to the model lock too.
    if (has_minimize() || has_multi_level()) {
        init.set_check_mode(Clingo::PropagatorCheckMode::Both);
        if (has_minimize()) {
            update_minimize(no_bound);
        }
    }

    // The levels after the active one are optimized in further solve calls.
    step_due_ = has_multi_level();

    auto max_var = static_cast<var_t>(cc.assignment().size());
    for (auto &solver : solvers_) {
        solver.mark_variables(max_var);
    }

    update_solver_annotations_();
}

void Propagator::update_solver_annotations_() {
    // The priority of the level that is optimized. In weighted mode there is
    // no active level, so a directive scoped to a level never applies.
    std::optional<int> active_priority;
    if (active_level_idx_ < minimize_levels_.size()) {
        active_priority = minimize_levels_[active_level_idx_].priority;
    }

    std::vector<SearchAnnotation> active;
    active.reserve(search_annotations_.size());
    for (auto const &ann : search_annotations_) {
        if (!ann.opt_level.has_value() || ann.opt_level == active_priority) {
            active.emplace_back(ann);
        }
    }

    for (auto &solver : solvers_) {
        solver.set_search_annotations(active);
    }
}

auto Propagator::simplify_(AbstractClauseCreator &cc) -> bool {
    Timer timer{stats_step_.time_simplify};
    struct Reset { // NOLINT
        ~Reset() {
            master.statistics().time_propagate = 0;
            master.statistics().time_check = 0;
        }
        Solver &master;
    } reset{master_()};
    return master_().simplify(cc, config_.check_state);
}

auto Propagator::translate_(InitClauseCreator &cc, UniqueMinimizeConstraint minimize) -> bool {
    Timer timer{stats_step_.time_translate};

    // add minimize constraint
    // Note: the minimize constraint is added after simplification to avoid
    // propagating tagged clauses, which is not supported at the moment.
    if (minimize != nullptr) {
        add_minimize_(std::move(minimize));
    }

    // translate (simple enough) constraints
    cc.set_state(InitState::Translate);
    bool ret = master_().translate(cc, stats_step_, config_, constraints_);
    if (!ret) {
        return false;
    }
    cc.set_state(InitState::Init);

    // mark minimize constraint as translated if necessary
    if (minimize_ != nullptr && master_().translate_minimize()) {
        minimize_ = nullptr;
    }

    return true;
}

void Propagator::propagate(Clingo::PropagateControl &control, Clingo::LiteralSpan changes) {
    auto &solver = solver_(control.thread_id());
    ControlClauseCreator cc{control, solver.statistics()};
    static_cast<void>(solver.propagate(cc, changes));
}

void Propagator::check(Clingo::PropagateControl &control) {
    auto ass = control.assignment();
    auto size = ass.size();
    auto &solver = solver_(control.thread_id());
    auto dl = ass.decision_level();

    if (minimize_ != nullptr) {
        auto minimize_bound = minimize_bound_.load(std::memory_order_relaxed);
        if (minimize_bound != no_bound) {
            auto bound = minimize_bound + minimize_->adjust();
            solver.update_minimize(*minimize_, dl, bound);
        }
    }

    ControlClauseCreator cc{control, solver.statistics()};

    if (!solver.check(cc, config_.check_state)) {
        return;
    }

    // Note: Makes sure that all variables are assigned in the end. But even if
    // the assignment is total, we do not have to introduce fresh variables if
    // variables have been introduced during check. In this case, there is a
    // guaranteed follow-up propagate call because all newly introduced
    // variables are watched.
    if (size == ass.size() && ass.is_total()) {
        solver.check_full(cc, config_.check_solution);
    }
}

void Propagator::undo(Clingo::PropagateControl const &control, Clingo::LiteralSpan changes) noexcept {
    static_cast<void>(changes);
    solver_(control.thread_id()).undo();
}

auto Propagator::decide(Clingo::id_t thread_id, Clingo::Assignment const &assign, lit_t fallback) -> lit_t {
    return solver_(thread_id).decide(assign, fallback);
}

auto Propagator::shown(var_t var) -> bool {
    auto sym = get_symbol(var);
    if (!sym.has_value()) {
        return false;
    }

    if (!show_) {
        return true;
    }

    if (show_variable_.find(var) != show_variable_.end()) {
        return true;
    }

    return sym->type() == Clingo::SymbolType::Function &&
           show_signature_.find(Clingo::Signature(sym->name(), sym->arguments().size())) != show_signature_.end();
}

auto Propagator::get_index(Clingo::Symbol sym) const -> std::optional<var_t> {
    auto it = sym_map_.find(sym);
    if (it != sym_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

auto Propagator::get_symbol(var_t var) const -> std::optional<Clingo::Symbol> {
    auto it = var_map_.find(var);
    if (it != var_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

auto Propagator::get_value(var_t var, uint32_t thread_id) const -> val_t { return solver_(thread_id).get_value(var); }

void Propagator::add_minimize_(UniqueMinimizeConstraint minimize) {
    assert(minimize_ == nullptr);
    minimize_ = minimize.get();
    add_constraint(std::move(minimize));
}

auto Propagator::remove_minimize() -> UniqueMinimizeConstraint {
    if (minimize_ == nullptr) {
        return nullptr;
    }

    --stats_step_.num_constraints;

    auto it = std::find_if(constraints_.begin(), constraints_.end(),
                           [this](UniqueConstraint const &x) -> bool { return x.get() == minimize_; });
    assert(it != constraints_.end());

    UniqueMinimizeConstraint minimize{(it->release(), minimize_)};
    for (auto &solver : solvers_) {
        solver.remove_constraint(*minimize_);
    }
    constraints_.erase(it);
    minimize_ = nullptr;
    return minimize;
}

auto Propagator::get_minimize_value(uint32_t thread_id) -> sum_t {
    assert(has_minimize());
    auto &solver = solver_(thread_id);

    sum_t bound = 0;
    for (auto [co, var] : *minimize_) {
        bound += static_cast<sum_t>(co) * solver.get_value(var);
    }
    return bound - minimize_->adjust();
}

void Propagator::update_minimize(sum_t bound) {
    assert(has_minimize());
    minimize_bound_ = bound;
}

auto Propagator::step(bool exhausted) -> bool {
    // Nothing to step: no solve call since the last step, or none at all.
    if (!step_due_) {
        return false;
    }
    step_due_ = false;
    // A solve call that was cut short has not finished its level.
    if (!exhausted) {
        return false;
    }
    return advance_level();
}

auto Propagator::advance_level() -> bool {
    if (active_level_idx_ >= minimize_levels_.size()) {
        return false;
    }

    // Without a model, the program is unsatisfiable and there is nothing to
    // optimize. A solve call of a level after the first one can end without a
    // model although the program is satisfiable: the level is seeded with its
    // value in the last model and a solve call that asks for a strictly
    // better value is then unsatisfiable.
    if (!has_model_) {
        return false;
    }

    // Lock the active level at its optimum. Propagator::on_statistics clears
    // the cost of the solve call before the application calls step, and the
    // cost of the last model is cleared when a level is seeded, so the value
    // of the level in the last model is the final fallback.
    auto &current = minimize_levels_[active_level_idx_];
    if (stats_step_.cost.has_value()) {
        current.locked_value = *stats_step_.cost;
    } else if (last_known_cost_.has_value()) {
        current.locked_value = *last_known_cost_;
    } else {
        current.locked_value = level_costs_[active_level_idx_];
    }
    current.locked = true;

    // Move the constraint back to its level. Propagator::init installs the
    // constraint of the next level and reads the locked ones.
    if (minimize_ != nullptr) {
        current.constraint = remove_minimize();
        current.ptr = current.constraint.get();
    }

    ++active_level_idx_;
    if (active_level_idx_ >= minimize_levels_.size()) {
        return false;
    }

    // The value of the next level in the last model. It is reachable, so a
    // solve call that finds no better model reports it as its cost and the
    // level is locked at it. The bound of the next solve call is not seeded
    // with it: Propagator::init starts every solve call without a bound.
    auto seed = level_costs_[active_level_idx_];
    stats_step_.cost = seed;
    last_known_cost_.reset();

    for (auto &solver : solvers_) {
        solver.disable_translate_minimize();
    }

    // The lower bound of the next level that follows from the domains of
    // its variables can already reach the seed. The seed is then the optimum
    // of the level, so the level is locked right away instead of letting the
    // next solve call prove that nothing better exists.
    auto const &next = minimize_levels_[active_level_idx_];
    auto const *constraint = next.constraint != nullptr ? next.constraint.get() : next.ptr;
    if (constraint != nullptr) {
        sum_t lower = 0;
        auto &master = master_();
        for (auto [co, var] : *constraint) {
            if (var == INVALID_VAR) {
                continue;
            }
            auto const &vs = master.var_state(var);
            if (co > 0) {
                lower += static_cast<sum_t>(co) * vs.min_bound();
            } else if (co < 0) {
                lower += static_cast<sum_t>(co) * vs.max_bound();
            }
        }
        lower -= constraint->adjust();

        if (lower >= seed) {
            return advance_level();
        }
    }

    // The directives scoped to the level that is now active take over.
    update_solver_annotations_();

    return true;
}

Propagator::~Propagator() {
    if (step_due_ && active_level_idx_ + 1 < minimize_levels_.size()) {
        std::cerr << STEP_MISSING_WARNING << "\n";
    }
}

} // namespace Clingcon
