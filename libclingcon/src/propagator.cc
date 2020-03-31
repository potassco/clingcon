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
    void add_constraint(lit_t lit, CoVarVec const &elems, val_t rhs, bool strict) override {
        if (!strict && cc_.assignment().is_false(lit)) {
            return;
        }

        if (elems.size() == 1) {
            auto [co, var] = elems.front();
            propagator_.add_simple(cc_, lit, co, var, rhs, strict);
        }
        else {
            assert (!strict);
            propagator_.add_constraint(SumConstraint::create(lit, rhs, elems, propagator_.config().sort_constraints));
        }

    }
    void add_minimize(val_t co, var_t var) override {
        minimize_elems_.emplace_back(co, var);
    }

    //! Add a distinct constraint.
    //!
    //! Binary distinct constraints will be represented with a sum constraint.
    void add_distinct(lit_t lit, std::vector<std::pair<CoVarVec, val_t>> const &elems) override {
        static_cast<void>(lit);
        static_cast<void>(elems);
        throw std::runtime_error("implement me");
        /*
        if self.cc.assignment.is_false(literal):
            return

        if len(elems) > 2:
            self._propagator.add_constraint(self.cc, DistinctConstraint(literal, elems))
            return

        for i, (rhs_i, elems_i) in enumerate(elems):
            for rhs_j, elems_j in elems[i+1:]:
                rhs = rhs_i - rhs_j

                celems = []
                celems.extend(elems_i)
                celems.extend((-co_j, var_j) for co_j, var_j in elems_j)

                if not celems:
                    if rhs == 0:
                        self.cc.add_clause([-literal])
                        return
                    continue

                a = self.cc.add_literal()
                b = self.cc.add_literal()

                self.cc.add_clause([a, b, -literal])
                self.cc.add_clause([-a, -b])

                self.add_constraint(a, celems, rhs-1, False)
                self.add_constraint(b, [(-co, var) for co, var in celems], -rhs-1, False)
        */
    }
    void add_dom(lit_t lit, var_t var, IntervalSet<val_t> const &elems) override {
        if (!cc_.assignment().is_false(lit)) {
            propagator_.add_dom(cc_, lit, var, elems);
        }
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

        return nullptr;
    }
private:
    Propagator &propagator_;
    InitClauseCreator &cc_;
    UniqueMinimizeConstraint minimize_;
    CoVarVec minimize_elems_;
};

} // namespace

void Propagator::on_model(Clingo::Model const &model) { // NOLINT
    static_cast<void>(model);
    throw std::runtime_error("implement me!!!");
    /*
    shown = (var for var in self._var_map.items() if self.shown(var))
    assignment = self._state(model.thread_id).get_assignment(shown)
    model.extend(
        clingo.Function("__csp", [var, value])
        for var, value in assignment if self.shown(var))

    if self.has_minimize:
        bound = self.get_minimize_value(model.thread_id)
        model.extend([clingo.Function("__csp_cost", [bound])])
        if self._minimize_bound is None or bound-1 < self._minimize_bound:
            self.statistics.cost = bound
            self.update_minimize(bound-1)
    */
}

void Propagator::on_statistics(Clingo::UserStatistics &step, Clingo::UserStatistics &accu) { // NOLINT
    static_cast<void>(step);
    static_cast<void>(accu);
    throw std::runtime_error("implement me!!!");
    /*
    for s in self._states:
        self._stats_step.tstats.append(s.statistics)
    self._stats_accu.accu(self._stats_step)
    self.add_statistics(step, self._stats_step)
    self.add_statistics(accu, self._stats_accu)
    self._stats_step.reset()
    */
}

void Propagator::add_statistics_(Clingo::UserStatistics &stats_map, Statistics &stats) { // NOLINT
    static_cast<void>(stats_map);
    static_cast<void>(stats);
    throw std::runtime_error("implement me!!!");
    /*
    def thread_stats(tstat):  # pylint: disable=missing-docstring
        p, c, u = tstat.time_propagate, tstat.time_check, tstat.time_undo
        return OrderedDict([
            ("Time in seconds", OrderedDict([
                ("Total", p+c+u),
                ("Propagation", p),
                ("Check", c),
                ("Undo", u)])),
            ("Refined reason", tstat.refined_reason),
            ("Introduced reason", tstat.introduced_reason),
            ("Literals introduced ", tstat.literals)])
    cost = []
    if stats.cost is not None:
        cost.append(("Cost", stats.cost))
    stats_map["Clingcon"] = OrderedDict(cost + [
        ("Init time in seconds", OrderedDict([
            ("Total", stats.time_init),
            ("Simplify", stats.time_simplify),
            ("Translate", stats.time_translate)])),
        ("Problem", OrderedDict([
            ("Constraints", stats.num_constraints),
            ("Variables", stats.num_variables),
            ("Clauses", stats.num_clauses),
            ("Literals", stats.num_literals)])),
        ("Translate", OrderedDict([
            ("Constraints removed", stats.translate_removed),
            ("Constraints added", stats.translate_added),
            ("Clauses", stats.translate_clauses),
            ("Weight constraints", stats.translate_wcs),
            ("Literals", stats.translate_literals)])),
        ("Thread", map(thread_stats, stats.tstats[:len(self._states)]))])
    */
}

var_t Propagator::add_variable(Clingo::Symbol sym) {
    auto [it, ret] = var_map_.emplace(sym, 0);

    if (ret) {
        it->second = master_().add_variable(config_.min_int, config_.max_int);
        ++stats_step_.num_variables;
    }

    return it->second;
}

void Propagator::show_variable(var_t var) {
    show_variable_.emplace(var);
}

void Propagator::show_signature(char const *name, size_t arity) {
    auto it = show_signature_.emplace(Clingo::Signature(name, arity));
    if (it.second) {
        show_offset_ = 0;
    }
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
    UniqueMinimizeConstraint minimize{remove_minimize_()};

    // remove solve step local and fixed literals
    for (auto &solver : solvers_) {
        solver.update(cc);
    }

    // add constraints
    ConstraintBuilder builder{*this, cc, std::move(minimize)};
    if (!parse(builder, init.theory_atoms())) {
        return;
    }

    // gather bounds of states in master
    auto &master = master_();
    for (auto it = solvers_.begin() + 1, ie = solvers_.end(); it != ie; ++it) {
        if (!master.update_bounds(cc, *it, config_.check_state)) {
            return;
        }
    }

    // propagate the newly added constraints
    if (!simplify_(cc)) {
        return;
    }

    // remove unnecessary literals after simplification
    if (!master.cleanup_literals(cc, config_.check_state)) {
        return;
    }

    // translate (simple enough) constraints
    if (!translate_(cc, builder.prepare_minimize())) {
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
    for (auto it = solvers_.begin() + 1, ie = solvers_.end(); it != ie; ++it) {
        it->copy_state(master);
    }

    // watch all the remaining constraints
    for (auto &constraint : constraints_) {
        cc.add_watch(constraint->literal());
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
    if (!show_) {
        return true;
    }

    if (var >= show_offset_) {
        for (auto sym_var : var_map_) {
            auto sym = sym_var.first;
            if (sym.type() == Clingo::SymbolType::Function && show_signature_.find(Clingo::Signature(sym.name(), sym.arguments().size())) != show_signature_.end()) {
                show_variable_.emplace(sym_var.second);
            }
        }
        show_offset_ = var_map_.size();
    }

    if (show_variable_.find(var) != show_variable_.end()) {
        return true;
    }

    return false;
}

val_t Propagator::get_value(var_t var, uint32_t thread_id) const {
    return solver_(thread_id).get_value(var);
}

uint32_t Propagator::num_variables() const {
    return var_map_.size();
}

void Propagator::add_minimize_(UniqueMinimizeConstraint minimize) {
    assert(minimize_ == nullptr);
    minimize_ = minimize.get();
    add_constraint(std::move(minimize));
}

UniqueMinimizeConstraint Propagator::remove_minimize_() {
    if (minimize_ == nullptr) {
        return nullptr;
    }

    --stats_step_.num_constraints;

    auto it = std::find_if(constraints_.begin(), constraints_.end(), [this](UniqueConstraint const &x) {
        return x.get() == minimize_;
    });
    assert(it != constraints_.end());

    UniqueMinimizeConstraint minimize{(it->release(), minimize_)};
    master_().remove_constraint(*minimize_);
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
