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

#ifndef CLINGCON_BASE_H
#define CLINGCON_BASE_H

#include <clingo.hh>
#include <forward_list>
#include <math/wide_integer/uintwide_t.h>
#include <optional>

//! @file clingcon/base.hh
//! Basic data types.
//!
//! @author Roland Kaminski

namespace Clingcon {

using level_t = uint32_t;                    //!< type for decision levels
using lit_t = Clingo::literal_t;             //!< type for solver and program literals
using var_t = uint32_t;                      //!< indexes of variables
using val_t = int32_t;                       //!< type for values of variables and coefficients
using sum_t = int64_t;                       //!< type for summing up values
using nsum_t = math::wide_integer::int128_t; //!< type for summing up values of nonlinear terms
using co_var_t = std::pair<val_t, var_t>;    //!< coeffcient/variable pair
using CoVarVec = std::vector<co_var_t>;

enum class Heuristic : val_t { None, MaxChain };

//! The maximum value for variables/coefficients in clingcon.
//!
//! This is the largest supported integer value. It is chosen like this so that
//! MAX_VAL-MIN_VAL does not overflow and we can always add 1 even to the
//! difference.
constexpr val_t MAX_VAL = std::numeric_limits<val_t>::max() / 2;
//! The minimum value for variables/coefficionts in clingcon.
//!
//! The minimum is chosen so that the product of two values will always fit
//! into Clingcon::sum_t.
constexpr val_t MIN_VAL = -MAX_VAL;

// defaults for solver config
constexpr val_t DEFAULT_SIGN_VALUE{0};
constexpr bool DEFAULT_SPLIT_ALL{false};
constexpr bool DEFAULT_PROPAGATE_CHAIN{true};
constexpr bool DEFAULT_REFINE_REASONS{true};
constexpr bool DEFAULT_REFINE_INTRODUCE{true};

// defaults for global config
constexpr val_t DEFAULT_MAX_INT{MAX_VAL};
constexpr val_t DEFAULT_MIN_INT{MIN_VAL};
constexpr bool DEFAULT_SORT_CONSTRAINTS{true};
constexpr uint64_t DEFAULT_CLAUSE_LIMIT_TOTAL{1000000};
constexpr uint32_t DEFAULT_CLAUSE_LIMIT{1000};
constexpr bool DEFAULT_LITERALS_ONLY{false};
constexpr double DEFAULT_WEIGHT_CONSTRAINT_RATIO{1.0};
constexpr uint32_t DEFAULT_DISTINCT_LIMIT{1000};
constexpr uint32_t DEFAULT_TRANSLATE_MINIMIZE{0};
constexpr bool DEFAULT_CHECK_SOLUTION{true};
constexpr bool DEFAULT_CHECK_STATE{false};
constexpr bool DEFAULT_ADD_ORDER_CLAUSES{false};

constexpr lit_t TRUE_LIT{1}; //!< The true literal.
constexpr var_t INVALID_VAR{std::numeric_limits<var_t>::max()};

//! Test whether a variable is valid.
inline auto is_valid_var(var_t var) -> bool { return var < INVALID_VAR; }

template <class I> inline auto check_valid_value(I val) -> val_t {
    if (val < MIN_VAL) {
        throw std::underflow_error("value too small");
    }
    if (val > MAX_VAL) {
        throw std::underflow_error("value too large");
    }
    return val;
}
static_assert(std::is_same<Clingo::weight_t, val_t>::value);

//! Solver specific statistics.
struct SolverStatistics {
    //! Reset all statistics to their starting values.
    void reset() { *this = SolverStatistics(); }

    //! Accumulate statistics in `stats`.
    void accu(SolverStatistics const &stats) {
        time_propagate += stats.time_propagate;
        time_check += stats.time_check;
        time_undo += stats.time_undo;
        refined_reason += stats.refined_reason;
        introduced_reason += stats.introduced_reason;
        literals += stats.literals;
    }

    double time_propagate{0};
    double time_check{0};
    double time_undo{0};
    uint64_t refined_reason{0};
    uint64_t introduced_reason{0};
    uint64_t literals{0};
};

//! Propagator specific statistics.
struct Statistics {
    //! Reset all statistics to their starting values.
    void reset() {
        time_init = 0;
        time_translate = 0;
        time_simplify = 0;
        num_variables = 0;
        num_constraints = 0;
        num_clauses = 0;
        num_literals = 0;
        translate_removed = 0;
        translate_added = 0;
        translate_clauses = 0;
        translate_wcs = 0;
        translate_literals = 0;
        cost.reset();
        for (auto &s : solver_statistics) {
            s.reset();
        }
    }

    //! Accumulate statistics in `stat`.
    void accu(Statistics &stat) {
        time_init += stat.time_init;
        time_translate += stat.time_translate;
        time_simplify += stat.time_simplify;
        num_variables += stat.num_variables;
        num_constraints += stat.num_constraints;
        num_clauses += stat.num_clauses;
        num_literals += stat.num_literals;
        translate_removed += stat.translate_removed;
        translate_added += stat.translate_added;
        translate_clauses += stat.translate_clauses;
        translate_wcs += stat.translate_wcs;
        translate_literals += stat.translate_literals;
        cost = stat.cost;

        auto it = solver_statistics.before_begin();
        for (auto &solver_stat : stat.solver_statistics) {
            auto jt = it++;
            if (it != solver_statistics.end()) {
                it->accu(solver_stat);
            } else {
                it = solver_statistics.emplace_after(jt, solver_stat);
            }
        }
    }

    auto solver_stats(uint32_t thread_id) -> SolverStatistics & {
        auto it = solver_statistics.before_begin();

        for (uint32_t i = 0; i <= thread_id; ++i) {
            auto jt = it++;
            if (it == solver_statistics.end()) {
                it = solver_statistics.emplace_after(jt);
            }
        }

        return *it;
    }

    double time_init = 0;
    double time_translate = 0;
    double time_simplify = 0;
    uint64_t num_variables = 0;
    uint64_t num_constraints = 0;
    uint64_t num_clauses = 0;
    uint64_t num_literals = 0;
    uint64_t translate_removed = 0;
    uint64_t translate_added = 0;
    uint64_t translate_clauses = 0;
    uint64_t translate_wcs = 0;
    uint64_t translate_literals = 0;
    std::optional<sum_t> cost;
    std::forward_list<SolverStatistics> solver_statistics;
};

//! Per solver configuration.
struct SolverConfig {
    Heuristic heuristic{Heuristic::None};
    val_t sign_value{DEFAULT_SIGN_VALUE};
    bool split_all{DEFAULT_SPLIT_ALL};
    bool propagate_chain{DEFAULT_PROPAGATE_CHAIN};
    bool refine_reasons{DEFAULT_REFINE_REASONS};
    bool refine_introduce{DEFAULT_REFINE_INTRODUCE};
};

//! Global configuration.
struct Config {
    //! Get solver specific configuration.
    auto solver_config(uint32_t thread_id) -> SolverConfig & {
        auto it = solver_configs.before_begin();

        for (uint32_t i = 0; i <= thread_id; ++i) {
            auto jt = it++;
            if (it == solver_configs.end()) {
                it = solver_configs.emplace_after(jt, default_solver_config);
            }
        }

        return *it;
    }

    std::forward_list<SolverConfig> solver_configs;
    SolverConfig default_solver_config;
    double weight_constraint_ratio{DEFAULT_WEIGHT_CONSTRAINT_RATIO};
    uint64_t clause_limit_total{DEFAULT_CLAUSE_LIMIT_TOTAL};
    uint32_t clause_limit{DEFAULT_CLAUSE_LIMIT};
    uint32_t distinct_limit{DEFAULT_DISTINCT_LIMIT};
    uint32_t translate_minimize{DEFAULT_TRANSLATE_MINIMIZE};
    val_t min_int{DEFAULT_MIN_INT};
    val_t max_int{DEFAULT_MAX_INT};
    bool sort_constraints{DEFAULT_SORT_CONSTRAINTS};
    bool literals_only{DEFAULT_LITERALS_ONLY};
    bool add_order_clauses{DEFAULT_ADD_ORDER_CLAUSES};
    bool check_solution{DEFAULT_CHECK_SOLUTION};
    bool check_state{DEFAULT_CHECK_STATE};
};

//! Class to add solver literals, create clauses, and access the current
//! assignment.
class AbstractClauseCreator {
  public:
    AbstractClauseCreator() = default;

    AbstractClauseCreator(AbstractClauseCreator &&) = delete;
    AbstractClauseCreator(AbstractClauseCreator const &) = delete;
    auto operator=(AbstractClauseCreator &&) -> AbstractClauseCreator & = delete;
    auto operator=(AbstractClauseCreator const &) -> AbstractClauseCreator & = delete;

    virtual ~AbstractClauseCreator() = default;

    //! Add a new solver literal.
    [[nodiscard]] virtual auto add_literal() -> lit_t = 0;

    //! Watch the given solver literal.
    virtual void add_watch(lit_t lit) = 0;

    //! Call unit propagation on the solver.
    virtual auto propagate() -> bool = 0;

    //! Add the given clause to the sovler.
    virtual auto add_clause(Clingo::LiteralSpan clause, Clingo::ClauseType type = Clingo::ClauseType::Learnt)
        -> bool = 0;

    //! Get the assignment.
    virtual auto assignment() -> Clingo::Assignment = 0;
};

enum class InitState { Init = 0, Translate = 1 };

//! Implement an `AbstractClauseCreator` using a `Clingo::PropagateInit`
//! object and extra functions.
class InitClauseCreator final : public AbstractClauseCreator {
  public:
    using Clause = std::vector<lit_t>;
    using WeightConstraint =
        std::tuple<lit_t, std::vector<Clingo::WeightedLiteral>, val_t, Clingo::WeightConstraintType>;
    using MinimizeLiteral = std::tuple<lit_t, val_t, int>;

    InitClauseCreator(Clingo::PropagateInit &init, Statistics &stats) : init_{init}, stats_{stats} {}

    InitClauseCreator(InitClauseCreator &&) = delete;
    InitClauseCreator(InitClauseCreator const &) = delete;
    auto operator=(InitClauseCreator &&) -> InitClauseCreator & = delete;
    auto operator=(InitClauseCreator const &) -> InitClauseCreator & = delete;

    ~InitClauseCreator() override = default;

    //! Get the propagator statistics.
    [[nodiscard]] auto statistics() const -> Statistics const & { return stats_; }

    [[nodiscard]] auto add_literal() -> lit_t override {
        auto lit = init_.add_literal();
        ++stats_.num_literals;
        if (state_ == InitState::Translate) {
            ++stats_.translate_literals;
        }
        return lit;
    }

    void add_watch(lit_t lit) override { init_.add_watch(lit); }

    [[nodiscard]] auto propagate() -> bool override { return commit() && init_.propagate(); }

    [[nodiscard]] auto add_clause(Clingo::LiteralSpan clause, Clingo::ClauseType type = Clingo::ClauseType::Learnt)
        -> bool override {
        assert(type != Clingo::ClauseType::Volatile && type != Clingo::ClauseType::VolatileStatic);
        static_cast<void>(type);

        ++stats_.num_clauses;
        if (state_ == InitState::Translate) {
            ++stats_.translate_clauses;
        }

        for (auto lit : clause) {
            clauses_.emplace_back(lit);
        }
        clauses_.emplace_back(0);

        return true;
    }

    [[nodiscard]] auto assignment() -> Clingo::Assignment override { return init_.assignment(); }

    //! Set the state to log either init literals or additionally translation
    //! literals.
    void set_state(InitState state) { state_ = state; }

    //! Map the literal to a solver literal.
    [[nodiscard]] auto solver_literal(lit_t literal) -> lit_t { return init_.solver_literal(literal); }

    //! Add a weight constraint of form `lit == (wlits <= bound)`.
    [[nodiscard]] auto add_weight_constraint(lit_t lit, Clingo::WeightedLiteralSpan wlits, val_t bound,
                                             Clingo::WeightConstraintType type) -> bool {
        auto ass = assignment();
        if (ass.is_true(lit)) {
            if (type < 0) {
                return true;
            }
        } else if (ass.is_false(lit)) {
            if (type > 0) {
                return true;
            }
        }

        if (state_ == InitState::Translate) {
            ++stats_.translate_wcs;
        }
        weight_constraints_.emplace_back(lit, std::vector<Clingo::WeightedLiteral>{wlits.begin(), wlits.end()}, bound,
                                         type);
        return true;
    }

    //! Add a literal to the objective function.
    void add_minimize(lit_t lit, val_t weight, int level) { minimize_.emplace_back(lit, weight, level); }

    //! Commit accumulated constraints.
    [[nodiscard]] auto commit() -> bool {
        for (auto it = clauses_.begin(), ie = clauses_.end(); it != ie; ++it) {
            auto ib = it;
            while (*it != 0) {
                ++it;
            }
            if (!init_.add_clause(Clingo::LiteralSpan{&*ib, &*it})) {
                return false;
            }
        }
        clauses_ = Clause();

        for (auto const &[lit, wlits, bound, type] : weight_constraints_) {
            auto inv = static_cast<Clingo::WeightConstraintType>(-type);
            if (!init_.add_weight_constraint(-lit, wlits, bound + 1, inv)) {
                return false;
            }
        }
        weight_constraints_.clear();

        for (auto const &[lit, weight, level] : minimize_) {
            init_.add_minimize(lit, weight, level);
        }
        minimize_.clear();

        return true;
    }

  private:
    InitState state_{InitState::Init};
    Clingo::PropagateInit &init_;
    Statistics &stats_;
    Clause clauses_;
    std::vector<WeightConstraint> weight_constraints_;
    std::vector<MinimizeLiteral> minimize_;
};

//! Implement an `AbstractClauseCreator` using a `Clingo::PropagateControl`
//! object.
class ControlClauseCreator final : public AbstractClauseCreator {
  public:
    ControlClauseCreator(Clingo::PropagateControl &control, SolverStatistics &stats)
        : control_{control}, stats_{stats} {}

    auto add_literal() -> lit_t override {
        ++stats_.literals;
        return control_.add_literal();
    }

    void add_watch(lit_t lit) override { control_.add_watch(lit); }

    auto propagate() -> bool override { return control_.propagate(); }

    auto add_clause(Clingo::LiteralSpan clause, Clingo::ClauseType type = Clingo::ClauseType::Learnt) -> bool override {
        return control_.add_clause(clause, type) && propagate();
    }

    auto assignment() -> Clingo::Assignment override { return control_.assignment(); }

  private:
    Clingo::PropagateControl &control_;
    SolverStatistics &stats_;
};

} // namespace Clingcon

namespace math::wide_integer {

using Clingcon::nsum_t;

struct ndiv_t {
    nsum_t quot{0};
    nsum_t rem{0};
};

inline auto div(nsum_t a, nsum_t b) -> ndiv_t { return {a / b, a % b}; }

} // namespace math::wide_integer

#endif // CLINGCON_BASE_H
