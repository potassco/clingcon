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

namespace Clingcon {

namespace {

//! CSP builder to use with the parse_theory function.
class ConstraintBuilder final : public AbstractConstraintBuilder {
public:
    ConstraintBuilder(Propagator &propgator, InitClauseCreator &cc, UniqueMinimizeConstraint minimize)
    : propagator_{propgator}
    , cc_{cc}
    , minimize_{std::move(minimize)} {
    }

    ConstraintBuilder(ConstraintBuilder const &) = delete;
    ConstraintBuilder(ConstraintBuilder &&) noexcept = delete;
    ConstraintBuilder& operator=(ConstraintBuilder const &) = delete;
    ConstraintBuilder& operator=(ConstraintBuilder &&) noexcept = delete;
    ~ConstraintBuilder() override = default;

    [[nodiscard]] lit_t solver_literal(lit_t literal) override {
        return cc_.solver_literal(literal);
    }
    [[nodiscard]] lit_t add_literal() override {
        return cc_.add_literal();
    }
    [[nodiscard]] bool is_true(lit_t literal) override {
        return cc_.assignment().is_true(literal);
    }
    [[nodiscard]] bool add_clause(Clingo::LiteralSpan clause) override {
        return cc_.add_clause(clause);
    }
    void add_show() override {
        propagator_.show();
    }
    void show_signature(char const *name, size_t arity) override {
        propagator_.show_signature(name, arity);
    }
    void show_variable(var_t var) override {
        propagator_.show_variable(var);
    }
    [[nodiscard]] var_t add_variable(Clingo::Symbol sym) override {
        return propagator_.add_variable(sym);
    }
    [[nodiscard]] bool add_constraint(lit_t lit, CoVarVec const &elems, val_t rhs, bool strict) override {
        if (!strict && cc_.assignment().is_false(lit)) {
            return true;
        }

        if (elems.size() == 1) {
            auto [co, var] = elems.front();
            return propagator_.add_simple(cc_, lit, co, var, rhs, strict);
        }

        assert (!strict);
        propagator_.add_constraint(SumConstraint::create(lit, rhs, elems, propagator_.config().sort_constraints));
        return true;
    }
    void add_minimize(val_t co, var_t var) override {
        minimize_elems_.emplace_back(co, var);
    }

    //! Add a distinct constraint.
    //!
    //! Binary distinct constraints will be represented with a sum constraint.
    [[nodiscard]] bool add_distinct(lit_t lit, std::vector<std::pair<CoVarVec, val_t>> const &elems) override {
        if (cc_.assignment().is_false(lit)) {
            return true;
        }

        if (elems.size() > 2) {
            propagator_.add_constraint(DistinctConstraint::create(lit, elems, propagator_.config().sort_constraints));
            return true;
        }

        CoVarVec celems;
        for (auto it = elems.begin(), ie = elems.end(); it != ie; ++it) {
            for (auto jt = it + 1; jt != ie; ++jt) {
                auto rhs = it->second - jt->second;
                celems.assign(it->first.begin(), it->first.end());
                for (auto [co, var] : jt->first) {
                    celems.emplace_back(-co, var);
                }
                rhs += simplify(celems, true);

                if (celems.empty()) {
                    return rhs != 0 || cc_.add_clause({-lit});
                }

                auto a = cc_.add_literal();
                auto b = cc_.add_literal();
                if (!cc_.add_clause({a, b, -lit})) {
                    return false;
                }
                if (!cc_.add_clause({-a, -b})) {
                    return false;
                }

                if (!add_constraint(a, celems, check_valid_value(rhs-1), false)) {
                    return false;
                }
                for (auto &co_var : celems) {
                    co_var.first = -co_var.first;
                }
                if (!add_constraint(b, celems, check_valid_value(-rhs-1), false)) {
                    return false;
                }
            }
        }

        return true;

    }

    [[nodiscard]] bool add_dom(lit_t lit, var_t var, IntervalSet<val_t> const &elems) override {
        return cc_.assignment().is_false(lit) || propagator_.add_dom(cc_, lit, var, elems);
    }

    //! Prepare the minimize constraint.
    UniqueMinimizeConstraint prepare_minimize() {
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
    for (auto [sym, var] : sym_map_) {
        if (shown(var)) {
            auto value = Clingo::Number(get_value(var, model.thread_id()));
            symbols_.emplace_back(Clingo::Function("__csp", {sym, value}));
        }
    }

    if (has_minimize()) {
        auto bound = get_minimize_value(model.thread_id());
        auto value = Clingo::String(std::to_string(bound).c_str());
        symbols_.emplace_back(Clingo::Function("__csp_cost", {value}));
        if (!minimize_bound_.has_value() || bound <= *minimize_bound_) {
            stats_step_.cost = bound;
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
}

void Propagator::add_statistics_(Clingo::UserStatistics &root, Statistics &stats) {
    using namespace Clingo;

    UserStatistics clingcon = root.add_subkey("Clingcon", StatisticsType::Map);

    if (stats.cost.has_value()) {
        clingcon.add_subkey("Cost", StatisticsType::Value).set_value(*stats.cost);
    }

    auto init_time = clingcon.add_subkey("Init time in seconds", StatisticsType::Map);
    init_time.add_subkey("Total", StatisticsType::Value).set_value(stats.time_init);
    init_time.add_subkey("Simplify", StatisticsType::Value).set_value(stats.time_simplify);
    init_time.add_subkey("Translate", StatisticsType::Value).set_value(stats.time_translate);

    auto problem = clingcon.add_subkey("Problem", StatisticsType::Map);
    problem.add_subkey("Constraints", StatisticsType::Value).set_value(stats.num_constraints);
    problem.add_subkey("Variables", StatisticsType::Value).set_value(stats.num_variables);
    problem.add_subkey("Clauses", StatisticsType::Value).set_value(stats.num_clauses);
    problem.add_subkey("Literals", StatisticsType::Value).set_value(stats.num_literals);

    auto translate = clingcon.add_subkey("Translate", StatisticsType::Map);
    translate.add_subkey("Constraints removed", StatisticsType::Value).set_value(stats.translate_removed);
    translate.add_subkey("Constraints added", StatisticsType::Value).set_value(stats.translate_added);
    translate.add_subkey("Clauses", StatisticsType::Value).set_value(stats.translate_clauses);
    translate.add_subkey("Weight constraints", StatisticsType::Value).set_value(stats.translate_wcs);
    translate.add_subkey("Literals", StatisticsType::Value).set_value(stats.translate_literals);

    UserStatistics threads = clingcon.add_subkey("Thread", StatisticsType::Array);
    threads.ensure_size(std::distance(stats.solver_statistics.begin(), stats.solver_statistics.end()), StatisticsType::Map);
    size_t i = 0;
    for (auto &solver_stat : stats.solver_statistics) {
        auto thread = threads[i++];

        auto time = thread.add_subkey("Time in seconds", StatisticsType::Map);
        auto total = solver_stat.time_propagate + solver_stat.time_check + solver_stat.time_undo;
        time.add_subkey("Total", StatisticsType::Value).set_value(total);
        time.add_subkey("Propagation", StatisticsType::Value).set_value(solver_stat.time_propagate);
        time.add_subkey("Check", StatisticsType::Value).set_value(solver_stat.time_check);
        time.add_subkey("Undo", StatisticsType::Value).set_value(solver_stat.time_undo);

        thread.add_subkey("Refined reason", StatisticsType::Value).set_value(solver_stat.refined_reason);
        thread.add_subkey("Introduced reason", StatisticsType::Value).set_value(solver_stat.introduced_reason);
        thread.add_subkey("Literals introduced", StatisticsType::Value).set_value(solver_stat.literals);
    }
}

var_t Propagator::add_variable(Clingo::Symbol sym) {
    auto [it, ret] = sym_map_.emplace(sym, 0);

    if (ret) {
        it->second = master_().add_variable(config_.min_int, config_.max_int);
        var_map_.emplace(it->second, sym);
        ++stats_step_.num_variables;
    }

    return it->second;
}

void Propagator::show_variable(var_t var) {
    show_variable_.emplace(var);
}

void Propagator::show_signature(char const *name, size_t arity) {
    show_signature_.emplace(Clingo::Signature(name, arity));
}

bool Propagator::add_dom(AbstractClauseCreator &cc, lit_t lit, var_t var, IntervalSet<val_t> const &domain) {
    return master_().add_dom(cc, lit, var, domain);
}

bool Propagator::add_simple(AbstractClauseCreator &cc, lit_t clit, val_t co, var_t var, val_t rhs, bool strict) {
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

void Propagator::init(Clingo::PropagateInit &init) {
    init.set_check_mode(Clingo::PropagatorCheckMode::Partial);

    Timer timer{stats_step_.time_init};
    InitClauseCreator cc{init, stats_step_};

    // remove minimize constraint
    UniqueMinimizeConstraint minimize{remove_minimize()};

    // remove solve step local and fixed literals
    for (auto &solver : solvers_) {
        solver.update(cc);
    }

    // add constraints
    ConstraintBuilder builder{*this, cc, std::move(minimize)};
    if (!parse(builder, init.theory_atoms())) {
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
        solvers_.emplace_back(config_.solver_config(i), stats_step_.solver_stats(i));
    }
    while (solvers_.size() > n) {
        solvers_.pop_back();
    }
    master.shrink_to_fit();
    for (auto it = solvers_.begin() + 1, ie = solvers_.end(); it != ie; ++it) {
        it->copy_state(master);
    }
}

bool Propagator::simplify_(AbstractClauseCreator &cc) {
    Timer timer{stats_step_.time_simplify};
    struct Reset{ // NOLINT
        ~Reset() {
            master.statistics().time_propagate = 0;
            master.statistics().time_check = 0;
        }
        Solver &master;
    } reset{master_()};
    return master_().simplify(cc, config_.check_state);
}

bool Propagator::translate_(InitClauseCreator &cc, UniqueMinimizeConstraint minimize ) {
    Timer timer{stats_step_.time_translate};

    // add minimize constraint
    // Note: the minimize constraint is added after simplification to avoid
    // propagating tagged clauses, which is not supported at the moment.
    if (minimize != nullptr) {
        // Note: fail if translation was requested earlier
        if (translated_minimize_ && !config_.translate_minimize) {
            throw std::runtime_error("translation of minimize constraints is disabled but was enabled before");
        }
        add_minimize_(std::move(minimize));
    }

    // translate (simple enough) constraints
    cc.set_state(InitClauseCreator::StateTranslate);
    bool ret = master_().translate(cc, stats_step_, config_, constraints_);
    if (!ret) {
        return false;
    }
    cc.set_state(InitClauseCreator::StateInit);

    // mark minimize constraint as translated if necessary
    if (config_.translate_minimize && minimize_ != nullptr) {
        translated_minimize_ = true;
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

    if (minimize_ != nullptr && minimize_bound_.has_value()) {
        auto bound = *minimize_bound_ + minimize_->adjust();
        solver.update_minimize(*minimize_, dl, bound);
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

bool Propagator::shown(var_t var) {
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

    return
        sym->type() == Clingo::SymbolType::Function &&
        show_signature_.find(Clingo::Signature(sym->name(), sym->arguments().size())) != show_signature_.end();
}

std::optional<var_t> Propagator::get_index(Clingo::Symbol sym) const {
    auto it = sym_map_.find(sym);
    if (it != sym_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<Clingo::Symbol> Propagator::get_symbol(var_t var) const {
    auto it = var_map_.find(var);
    if (it != var_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

val_t Propagator::get_value(var_t var, uint32_t thread_id) const {
    return solver_(thread_id).get_value(var);
}

void Propagator::add_minimize_(UniqueMinimizeConstraint minimize) {
    assert(minimize_ == nullptr);
    minimize_ = minimize.get();
    add_constraint(std::move(minimize));
}

UniqueMinimizeConstraint Propagator::remove_minimize() {
    if (minimize_ == nullptr) {
        return nullptr;
    }

    --stats_step_.num_constraints;

    auto it = std::find_if(constraints_.begin(), constraints_.end(), [this](UniqueConstraint const &x) {
        return x.get() == minimize_;
    });
    assert(it != constraints_.end());

    UniqueMinimizeConstraint minimize{(it->release(), minimize_)};
    for (auto &solver : solvers_) {
        solver.remove_constraint(*minimize_);
    }
    constraints_.erase(it);
    minimize_ = nullptr;
    return minimize;
}

sum_t Propagator::get_minimize_value(uint32_t thread_id) {
    assert (has_minimize());
    auto &solver = solver_(thread_id);

    sum_t bound = 0;
    for (auto [co, var] : *minimize_) {
        bound += static_cast<sum_t>(co) * solver.get_value(var);
    }
    return bound - minimize_->adjust();
}

void Propagator::update_minimize(sum_t bound) {
    assert (has_minimize());
    minimize_bound_ = bound;
}

} // namespace Clingcon
