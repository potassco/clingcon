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

#include <clingo/core.hh>
#include <clingo/propagate.hh>

#include <math/wide_integer/uintwide_t.h>

#include <forward_list>
#include <optional>
#include <vector>

//! @file clingcon/base.hh
//! Basic data types.
//!
//! @author Roland Kaminski

namespace Clingcon {

using level_t = uint32_t;                    //!< type for decision levels
using lit_t = Clingo::SolverLiteral;         //!< type for solver and program literals
using var_t = uint32_t;                      //!< indexes of variables
using val_t = int32_t;                       //!< type for values of variables and coefficients
using sum_t = int64_t;                       //!< type for summing up values
using nsum_t = math::wide_integer::int128_t; //!< type for summing up values of nonlinear terms
using co_var_t = std::pair<val_t, var_t>;    //!< coefficient/variable pair
using CoVarVec = std::vector<co_var_t>;

enum class TruthValue : uint8_t {
    True,
    False,
    Free,
};

inline auto map_truth(std::optional<bool> tv) {
    if (tv) {
        return *tv ? TruthValue::True : TruthValue::False;
    }
    return TruthValue::Free;
}

// NOLINTNEXTLINE
enum class Heuristic : val_t { None, MaxChain };

//! The maximum value for variables/coefficients in clingcon.
//!
//! This is the largest supported integer value. It is chosen like this so that
//! MAX_VAL-MIN_VAL does not overflow and we can always add 1 even to the
//! difference.
constexpr val_t MAX_VAL = std::numeric_limits<val_t>::max() / 2;
//! The minimum value for variables/coefficients in clingcon.
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
inline auto is_valid_var(var_t var) -> bool {
    return var < INVALID_VAR;
}

template <class I> inline auto check_valid_value(I val) -> val_t {
    if (val < MIN_VAL) {
        throw std::underflow_error("value too small");
    }
    if (val > MAX_VAL) {
        throw std::underflow_error("value too large");
    }
    return val;
}

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

#define CLINGCON_THREAD_CONFIG(type, name, default_value)                                                              \
  private:                                                                                                             \
    type name##_{default_value};                                                                                       \
                                                                                                                       \
  public:                                                                                                              \
    [[nodiscard]] auto name(std::optional<clingo_id_t> thread_id = std::nullopt) const -> type {                       \
        if (thread_id && *thread_id < solver_configs_.size() && solver_configs_[*thread_id].name) {                    \
            return *solver_configs_[*thread_id].name; /* NOLINT */                                                     \
        }                                                                                                              \
        return name##_;                                                                                                \
    }                                                                                                                  \
    void set_##name(type value, std::optional<clingo_id_t> thread_id = std::nullopt) {                                 \
        if (thread_id) {                                                                                               \
            ensure_solver_config(*thread_id);                                                                          \
            solver_configs_[*thread_id].name = value;                                                                  \
        } else {                                                                                                       \
            name##_ = value;                                                                                           \
        }                                                                                                              \
    }

#define CLINGCON_CONFIG(type, name, default_value)                                                                     \
  private:                                                                                                             \
    type name##_{default_value};                                                                                       \
                                                                                                                       \
  public:                                                                                                              \
    [[nodiscard]] auto name() const -> type {                                                                          \
        return name##_;                                                                                                \
    }                                                                                                                  \
    void set_##name(type value) {                                                                                      \
        name##_ = value;                                                                                               \
    }

//! Global configuration.
class Config {
  public:
    CLINGCON_THREAD_CONFIG(Heuristic, heuristic, Heuristic::None)
    CLINGCON_THREAD_CONFIG(val_t, sign_value, DEFAULT_SIGN_VALUE)
    CLINGCON_THREAD_CONFIG(bool, split_all, DEFAULT_SPLIT_ALL)
    CLINGCON_THREAD_CONFIG(bool, propagate_chain, DEFAULT_PROPAGATE_CHAIN)
    CLINGCON_THREAD_CONFIG(bool, refine_reasons, DEFAULT_REFINE_REASONS)
    CLINGCON_THREAD_CONFIG(bool, refine_introduce, DEFAULT_REFINE_INTRODUCE)
    CLINGCON_CONFIG(double, weight_constraint_ratio, DEFAULT_WEIGHT_CONSTRAINT_RATIO)
    CLINGCON_CONFIG(uint64_t, clause_limit_total, DEFAULT_CLAUSE_LIMIT_TOTAL)
    CLINGCON_CONFIG(uint64_t, clause_limit, DEFAULT_CLAUSE_LIMIT)
    CLINGCON_CONFIG(uint32_t, distinct_limit, DEFAULT_DISTINCT_LIMIT)
    CLINGCON_CONFIG(uint32_t, translate_minimize, DEFAULT_TRANSLATE_MINIMIZE)
    CLINGCON_CONFIG(val_t, min_int, DEFAULT_MIN_INT)
    CLINGCON_CONFIG(val_t, max_int, DEFAULT_MAX_INT)

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    bool sort_constraints{DEFAULT_SORT_CONSTRAINTS};
    bool literals_only{DEFAULT_LITERALS_ONLY};
    bool add_order_clauses{DEFAULT_ADD_ORDER_CLAUSES};
    bool check_solution{DEFAULT_CHECK_SOLUTION};
    bool check_state{DEFAULT_CHECK_STATE};
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

    [[nodiscard]] auto size() const -> size_t { return solver_configs_.size(); }

    [[nodiscard]] auto has_heuristic() const -> bool {
        return heuristic_ != Heuristic::None || std::ranges::any_of(solver_configs_, [](auto const &cfg) {
                   return cfg.heuristic && cfg.heuristic != Heuristic::None;
               });
    }

  private:
    void ensure_solver_config(clingo_id_t thread_id) {
        while (thread_id >= solver_configs_.size()) {
            solver_configs_.emplace_back();
        }
    }

    //! Per solver configuration.
    struct SolverConfig {
        std::optional<Heuristic> heuristic;
        std::optional<val_t> sign_value;
        std::optional<bool> split_all;
        std::optional<bool> propagate_chain;
        std::optional<bool> refine_reasons;
        std::optional<bool> refine_introduce;
    };

    std::vector<SolverConfig> solver_configs_;
};

#undef CLINGCON_CONFIG
#undef CLINGCON_THREAD_CONFIG

//! Per solver configuration.
class SolverConfig {
  public:
    SolverConfig(Config const &config, clingo_id_t thread_id) : config_{&config}, thread_id_{thread_id} {}
    [[nodiscard]] auto heuristic() const -> Heuristic { return config_->heuristic(thread_id_); }
    [[nodiscard]] auto sign_value() const -> val_t { return config_->sign_value(thread_id_); }
    [[nodiscard]] auto split_all() const -> bool { return config_->split_all(thread_id_); }
    [[nodiscard]] auto propagate_chain() const -> bool { return config_->propagate_chain(thread_id_); }
    [[nodiscard]] auto refine_reasons() const -> bool { return config_->refine_reasons(thread_id_); }
    [[nodiscard]] auto refine_introduce() const -> bool { return config_->refine_introduce(thread_id_); }

  private:
    Config const *config_;
    clingo_id_t thread_id_;
};

//! Class to add solver literals, create clauses, and access the current
//! assignment.
class AbstractClauseCreator {
  public:
    AbstractClauseCreator(const Clingo::Assignment &assignment, Clingo::PropagateControl &control)
        : control_{control}, assignment_{assignment} {}

    AbstractClauseCreator(AbstractClauseCreator &&) = delete;
    AbstractClauseCreator(AbstractClauseCreator const &) = delete;
    auto operator=(AbstractClauseCreator &&) -> AbstractClauseCreator & = delete;
    auto operator=(AbstractClauseCreator const &) -> AbstractClauseCreator & = delete;

    virtual ~AbstractClauseCreator() = default;

    //! Add a new solver literal.
    [[nodiscard]] auto add_literal() -> lit_t { return new_literal(control_.add_literal()); }

    //! Watch the given solver literal.
    void add_watch(lit_t lit) { control_.add_watch(lit); }

    //! Call unit propagation on the solver.
    auto propagate() -> bool { return prepare_propagate() && control_.propagate(); }

    //! Add the given clause to the solver.
    auto add_clause(Clingo::SolverLiteralSpan clause, Clingo::ClauseFlags type = Clingo::ClauseFlags::none) -> bool {
        return !commit_clause(clause, type) || (control_.add_clause(clause, type) && propagate());
    }

    //! Get the assignment.
    [[nodiscard]] auto assignment() const -> Clingo::Assignment { return assignment_; }

  protected:
    template <std::derived_from<Clingo::PropagateControl> T> auto control() -> T & {
        return static_cast<T &>(control_);
    }

  private:
    virtual auto new_literal(lit_t) -> lit_t = 0;
    virtual auto commit_clause(Clingo::SolverLiteralSpan clause, Clingo::ClauseFlags type) -> bool = 0;
    virtual auto prepare_propagate() -> bool = 0;

    Clingo::PropagateControl &control_;
    Clingo::Assignment assignment_;
};

enum class InitState : uint8_t { Init = 0, Translate = 1 };

//! Implement an `AbstractClauseCreator` using a `Clingo::PropagateInit`
//! object and extra functions.
class InitClauseCreator final : public AbstractClauseCreator {
  public:
    using Clause = std::vector<lit_t>;
    using WeightConstraint =
        std::tuple<lit_t, std::vector<Clingo::WeightedLiteral>, val_t, Clingo::WeightConstraintType>;
    using MinimizeLiteral = std::tuple<lit_t, val_t, int>;

    InitClauseCreator(const Clingo::Assignment &ass, Clingo::PropagateInit &init, Statistics &stats)
        : AbstractClauseCreator(ass, init), stats_{stats} {}

    //! Get the propagator statistics.
    [[nodiscard]] auto statistics() const -> Statistics const & { return stats_; }

    //! Set the state to log either init literals or additionally translation
    //! literals.
    void set_state(InitState state) { state_ = state; }

    //! Map the literal to a solver literal.
    [[nodiscard]] auto solver_literal(lit_t literal) -> lit_t {
        return control<Clingo::PropagateInit>().solver_literal(literal);
    }

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
        auto &init = control<Clingo::PropagateInit>();
        for (auto it = clauses_.begin(), ie = clauses_.end(); it != ie; ++it) {
            auto ib = it;
            while (*it != 0) {
                ++it;
            }
            if (!init.add_clause(Clingo::SolverLiteralSpan{&*ib, &*it})) {
                return false;
            }
        }
        clauses_ = Clause();

        for (auto const &[lit, wlits, bound, type] : weight_constraints_) {
            auto inv = type == Clingo::WeightConstraintType::implication_left
                           ? Clingo::WeightConstraintType::implication_right
                           : Clingo::WeightConstraintType::implication_left;
            if (!init.add_weight_constraint(-lit, wlits, bound + 1, inv)) {
                return false;
            }
        }
        weight_constraints_.clear();

        for (auto const &[lit, weight, level] : minimize_) {
            init.add_minimize(lit, weight, level);
        }
        minimize_.clear();

        return true;
    }

  private:
    auto new_literal(lit_t lit) -> lit_t override {
        ++stats_.num_literals;
        if (state_ == InitState::Translate) {
            ++stats_.translate_literals;
        }
        return lit;
    }
    auto commit_clause(Clingo::SolverLiteralSpan clause, [[maybe_unused]] Clingo::ClauseFlags type) -> bool override {
        assert(!intersects(type, Clingo::ClauseFlags::tag));

        ++stats_.num_clauses;
        if (state_ == InitState::Translate) {
            ++stats_.translate_clauses;
        }

        for (auto lit : clause) {
            clauses_.emplace_back(lit);
        }
        clauses_.emplace_back(0);

        return false;
    }
    auto prepare_propagate() -> bool override { return commit(); }

    InitState state_{InitState::Init};
    Statistics &stats_;
    Clause clauses_;
    std::vector<WeightConstraint> weight_constraints_;
    std::vector<MinimizeLiteral> minimize_;
};

//! Implement an `AbstractClauseCreator` using a `Clingo::PropagateControl`
//! object.
class ControlClauseCreator final : public AbstractClauseCreator {
  public:
    ControlClauseCreator(const Clingo::Assignment &assignment, Clingo::PropagateControl &control,
                         SolverStatistics &stats)
        : AbstractClauseCreator(assignment, control), stats_{stats} {}

  private:
    auto new_literal(lit_t lit) -> lit_t override {
        ++stats_.literals;
        return lit;
    }
    auto commit_clause([[maybe_unused]] Clingo::SolverLiteralSpan clause, [[maybe_unused]] Clingo::ClauseFlags type)
        -> bool override {
        return true;
    }
    auto prepare_propagate() -> bool override { return true; }
    SolverStatistics &stats_;
};

} // namespace Clingcon

namespace math::wide_integer {

using Clingcon::nsum_t;

struct ndiv_t {
    nsum_t quot{0};
    nsum_t rem{0};
};

inline auto div(nsum_t a, nsum_t b) -> ndiv_t {
    return {a / b, a % b};
}

} // namespace math::wide_integer

#endif // CLINGCON_BASE_H
