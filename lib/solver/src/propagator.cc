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

#include <algorithm>

#include "clingcon/parsing.hh"
#include "clingcon/propagator.hh"

namespace Clingcon {

namespace {

//! CSP builder to use with the parse_theory function.
class ConstraintBuilder final : public AbstractConstraintBuilder {
  public:
    ConstraintBuilder(Propagator &propagator, InitClauseCreator &cc, UniqueMinimizeConstraint minimize)
        : propagator_{propagator}, cc_{cc}, minimize_{std::move(minimize)} {}
    ConstraintBuilder(ConstraintBuilder &&) noexcept = delete;

    [[nodiscard]] auto solver_literal(lit_t literal) -> lit_t override { return cc_.solver_literal(literal); }
    [[nodiscard]] auto add_literal() -> lit_t override { return cc_.add_literal(); }
    [[nodiscard]] auto is_true(lit_t literal) -> bool override { return cc_.assignment().is_true(literal); }
    [[nodiscard]] auto add_clause(Clingo::SolverLiteralSpan clause) -> bool override { return cc_.add_clause(clause); }
    void add_show() override { propagator_.show(); }
    void show_signature(std::string_view name, size_t arity) override { propagator_.show_signature(name, arity); }
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
        propagator_.add_constraint(
            UniqueConstraint{new NonlinearConstraint{lit, co_ab, var_a, var_b, co_c, var_c, rhs}});
        if (strict) {
            propagator_.add_constraint(UniqueConstraint{new NonlinearConstraint{
                lit, safe_inv(co_ab), var_a, var_b, safe_inv(co_c), var_c, safe_inv(safe_add(rhs, 1))}});
        }
        return true;
    }

    void add_minimize(val_t co, var_t var) override { minimize_elems_.emplace_back(co, var); }

    //! Add a distinct constraint.
    //!
    //! Binary distinct constraints will be represented with a sum constraint.
    [[nodiscard]] auto add_distinct(lit_t lit, std::vector<std::pair<CoVarVec, val_t>> const &elems) -> bool override {
        auto truth = map_truth(cc_.assignment().value(lit));
        if (truth == TruthValue::False) {
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
                        return cc_.add_clause(std::to_array({-lit}));
                    }
                    continue;
                }

                auto a = cc_.add_literal();
                auto b = -a;
                if (truth != TruthValue::True) {
                    b = cc_.add_literal();
                    if (!cc_.add_clause(std::to_array({a, b, -lit}))) {
                        return false;
                    }
                    if (!cc_.add_clause(std::to_array({-a, -b}))) {
                        return false;
                    }
                    if (!cc_.add_clause(std::to_array({lit, -a}))) {
                        return false;
                    }
                    if (!cc_.add_clause(std::to_array({lit, -b}))) {
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

        return cc_.add_clause(std::to_array({-lit, -lit_a, -lit_b}));
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

    //! Prepare the minimize constraint.
    auto prepare_minimize() -> UniqueMinimizeConstraint {
        // copy values of old minimize constraint
        if (minimize_ != nullptr) {
            for (auto elem : *minimize_) {
                minimize_elems_.emplace_back(elem);
            }
            minimize_elems_.emplace_back(minimize_->adjust(), INVALID_VAR);
        }
        // simplify minimize
        if (!minimize_elems_.empty()) {
            auto adjust = simplify(minimize_elems_, true);
            minimize_ = MinimizeConstraint::create(adjust, minimize_elems_, propagator_.config().sort_constraints);
        }

        return std::move(minimize_);
    }

  private:
    Propagator &propagator_;
    InitClauseCreator &cc_;
    UniqueMinimizeConstraint minimize_;
    CoVarVec minimize_elems_;
};

} // namespace

void Propagator::on_model(Clingo::Model &model) {
    std::vector<Clingo::Symbol> symbols_;
    for (auto const &[sym, var] : sym_map_) {
        if (shown(var)) {
            auto value = Clingo::Number(get_value(var, model.thread_id()));
            symbols_.emplace_back(Clingo::Function(lib_, "__csp", {sym, value}));
        }
    }

    if (has_minimize()) {
        auto bound = get_minimize_value(model.thread_id());
        auto value = Clingo::String(lib_, std::to_string(bound));
        symbols_.emplace_back(Clingo::Function(lib_, "__csp_cost", {value}));
        if (bound <= minimize_bound_.load(std::memory_order_relaxed)) {
            stats_step_.cost = bound;
            update_minimize(bound - 1);
        }
    }

    model.extend(symbols_);
}

void Propagator::on_statistics(Clingo::Stats step, Clingo::Stats accu) {
    stats_accu_.accu(stats_step_);
    add_statistics_(step, stats_step_);
    add_statistics_(accu, stats_accu_);
    stats_step_.reset();
}

void Propagator::add_statistics_(Clingo::Stats root, Statistics &stats) {
    using namespace Clingo;

    auto clingcon = root.map().insert("Clingcon", StatsType::map).map();

    if (stats.cost.has_value()) {
        clingcon.insert("Cost", StatsType::value) = static_cast<double>(*stats.cost);
    }

    auto init_time = clingcon.insert("Init time in seconds", StatsType::map).map();
    init_time.insert("Total", StatsType::value) = stats.time_init;
    init_time.insert("Simplify", StatsType::value) = stats.time_simplify;
    init_time.insert("Translate", StatsType::value) = stats.time_translate;

    auto problem = clingcon.insert("Problem", StatsType::map).map();
    problem.insert("Constraints", StatsType::value) = static_cast<double>(stats.num_constraints);
    problem.insert("Variables", StatsType::value) = static_cast<double>(stats.num_variables);
    problem.insert("Clauses", StatsType::value) = static_cast<double>(stats.num_clauses);
    problem.insert("Literals", StatsType::value) = static_cast<double>(stats.num_literals);

    auto translate = clingcon.insert("Translate", StatsType::map).map();
    translate.insert("Constraints removed", StatsType::value) = static_cast<double>(stats.translate_removed);
    translate.insert("Constraints added", StatsType::value) = static_cast<double>(stats.translate_added);
    translate.insert("Clauses", StatsType::value) = static_cast<double>(stats.translate_clauses);
    translate.insert("Weight constraints", StatsType::value) = static_cast<double>(stats.translate_wcs);
    translate.insert("Literals", StatsType::value) = static_cast<double>(stats.translate_literals);

    auto threads = clingcon.insert("Thread", StatsType::array).array();
    size_t i = 0;
    for (auto &solver_stat : stats.solver_statistics) {
        auto thread = threads.ensure(i++, StatsType::map).map();

        auto time = thread.insert("Time in seconds", StatsType::map).map();
        auto total = solver_stat.time_propagate + solver_stat.time_check + solver_stat.time_undo;
        time.insert("Total", StatsType::value) = total;
        time.insert("Propagation", StatsType::value) = solver_stat.time_propagate;
        time.insert("Check", StatsType::value) = solver_stat.time_check;
        time.insert("Undo", StatsType::value) = solver_stat.time_undo;

        thread.insert("Refined reason", StatsType::value) = static_cast<double>(solver_stat.refined_reason);
        thread.insert("Introduced reason", StatsType::value) = static_cast<double>(solver_stat.introduced_reason);
        thread.insert("Literals introduced", StatsType::value) = static_cast<double>(solver_stat.literals);
    }
}

auto Propagator::add_variable(Clingo::Symbol const &sym) -> var_t {
    auto [it, ret] = sym_map_.emplace(sym, 0);

    if (ret) {
        it->second = master_().add_variable(config_.min_int(), config_.max_int());
        var_map_.emplace(it->second, sym);
        ++stats_step_.num_variables;
    }

    return it->second;
}

void Propagator::show_variable(var_t var) {
    show_variable_.emplace(var);
}

void Propagator::show_signature(std::string_view name, size_t arity) {
    show_signature_.emplace(name, arity);
}

auto Propagator::add_dom(AbstractClauseCreator &cc, lit_t lit, var_t var, IntervalSet<val_t> const &domain) -> bool {
    return master_().add_dom(cc, lit, var, domain);
}

auto Propagator::add_simple(AbstractClauseCreator &cc, lit_t clit, val_t co, var_t var, val_t rhs, bool strict)
    -> bool {
    return master_().add_simple(cc, clit, co, var, rhs, strict);
}

void Propagator::add_constraint_(UniqueConstraint constraint) {
    constraints_.emplace_back(std::move(constraint));
}

void Propagator::add_constraint(UniqueConstraint constraint) {
    ++stats_step_.num_constraints;
    master_().add_constraint(*constraint);
    add_constraint_(std::move(constraint));
}

void Propagator::do_init(Clingo::Assignment assignment, Clingo::PropagateInit init) {
    init.check_mode(Clingo::PropagatorCheckMode::fixpoint);

    Timer timer{stats_step_.time_init};
    InitClauseCreator cc{assignment, init, stats_step_};

    // remove minimize constraint
    UniqueMinimizeConstraint minimize{remove_minimize()};

    // remove solve step local and fixed literals
    for (auto &solver : solvers_) {
        solver.update();
    }

    // add constraints
    ConstraintBuilder builder{*this, cc, std::move(minimize)};
    if (!parse(lib_, builder, init.base().theory())) {
        return;
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

    // propagate the newly added constraints
    if (!simplify_(cc)) {
        return;
    }

    // translate (simple enough) constraints
    if (!translate_(cc, builder.prepare_minimize())) {
        return;
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
        solvers_.emplace_back(SolverConfig(config_, i), stats_step_.solver_stats(i));
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
    if (has_minimize()) {
        init.check_mode(Clingo::PropagatorCheckMode::both);
        update_minimize(no_bound);
    }

    auto max_var = static_cast<var_t>(cc.assignment().size());
    for (auto &solver : solvers_) {
        solver.mark_variables(max_var);
    }
}

auto Propagator::simplify_(AbstractClauseCreator &cc) -> bool {
    Timer timer{stats_step_.time_simplify};
    struct Reset { // NOLINT
        ~Reset() {
            master.statistics().time_propagate = 0;
            master.statistics().time_check = 0;
        }
        Solver &master; // NOLINT
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

void Propagator::do_propagate(Clingo::Assignment assignment, Clingo::PropagateControl control,
                              Clingo::SolverLiteralSpan changes) {
    auto &solver = solver_(assignment.thread_id());
    ControlClauseCreator cc{assignment, control, solver.statistics()};
    static_cast<void>(solver.propagate(cc, changes));
}

void Propagator::do_check(Clingo::Assignment ass, Clingo::PropagateControl control) {
    auto size = ass.size();
    auto &solver = solver_(ass.thread_id());
    auto dl = ass.decision_level();

    if (minimize_ != nullptr) {
        auto minimize_bound = minimize_bound_.load(std::memory_order_relaxed);
        if (minimize_bound != no_bound) {
            auto bound = minimize_bound + minimize_->adjust();
            solver.update_minimize(*minimize_, dl, bound);
        }
    }

    ControlClauseCreator cc{ass, control, solver.statistics()};

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

void Propagator::do_undo(Clingo::Assignment assignment, [[maybe_unused]] Clingo::ProgramLiteralSpan changes) noexcept {
    solver_(assignment.thread_id()).undo();
}

auto Propagator::do_decide(Clingo::Assignment assign, lit_t fallback) -> lit_t {
    return solver_(assign.thread_id()).decide(assign, fallback);
}

auto Propagator::shown(var_t var) -> bool {
    auto sym = get_symbol(var);
    if (!sym.has_value()) {
        return false;
    }

    if (!show_) {
        return true;
    }

    if (show_variable_.contains(var)) {
        return true;
    }

    return sym->type() == Clingo::SymbolType::function &&
           show_signature_.contains(Sig(sym->name(), sym->arguments().size()));
}

auto Propagator::get_index(Clingo::Symbol const &sym) const -> std::optional<var_t> {
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

auto Propagator::get_value(var_t var, uint32_t thread_id) const -> val_t {
    return solver_(thread_id).get_value(var);
}

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

    auto it = std::ranges::find_if(constraints_, [this](UniqueConstraint const &x) { return x.get() == minimize_; });
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

} // namespace Clingcon
