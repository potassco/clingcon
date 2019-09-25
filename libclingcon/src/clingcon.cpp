// {{{ MIT License
//
// // Copyright 2018 Roland Kaminski, Philipp Wanko, Max Ostrowski
//
// // Permission is hereby granted, free of charge, to any person obtaining a copy
// // of this software and associated documentation files (the "Software"), to
// // deal in the Software without restriction, including without limitation the
// // rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// // sell copies of the Software, and to permit persons to whom the Software is
// // furnished to do so, subject to the following conditions:
//
// // The above copyright notice and this permission notice shall be included in
// // all copies or substantial portions of the Software.
//
// // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// // FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// // IN THE SOFTWARE.
//
// // }}}

#include <clingcon.h>
#include <clingcon/clingconpropagator.h>

#include <sstream>

using namespace Clingo;
using namespace clingcon;

#define CLINGCON_TRY try
#define CLINGCON_CATCH                                                                             \
    catch (...)                                                                                    \
    {                                                                                              \
        Clingo::Detail::handle_cxx_error();                                                        \
        return false;                                                                              \
    }                                                                                              \
    return true

#define CLINGO_CALL(x) Clingo::Detail::handle_error(x)

bool init(clingo_propagate_init_t *i, void *data)
{
    CLINGCON_TRY
    {
        PropagateInit in(i);
        static_cast< ClingconPropagator * >(data)->init(in);
    }
    CLINGCON_CATCH;
}

bool propagate(clingo_propagate_control_t *i, const clingo_literal_t *changes, size_t size,
               void *data)
{
    CLINGCON_TRY
    {
        PropagateControl in(i);
        static_cast< ClingconPropagator * >(data)->propagate(in, {changes, size});
    }
    CLINGCON_CATCH;
}

bool undo(clingo_propagate_control_t const *i, const clingo_literal_t *changes, size_t size,
          void *data)
{
    CLINGCON_TRY
    {
        PropagateControl in(const_cast< clingo_propagate_control_t * >(i));
        static_cast< ClingconPropagator * >(data)->undo(in, {changes, size});
    }
    CLINGCON_CATCH;
}

bool check(clingo_propagate_control_t *i, void *data)
{
    CLINGCON_TRY
    {
        PropagateControl in(i);
        static_cast< ClingconPropagator * >(data)->check(in);
    }
    CLINGCON_CATCH;
}

struct PropagatorFacade
{
public:
    virtual ~PropagatorFacade() {}
    virtual bool prepare(clingo_control_t *ctl) = 0;
    virtual bool lookup_symbol(clingo_symbol_t name, size_t *index) = 0;
    virtual clingo_symbol_t get_symbol(size_t index) = 0;
    virtual bool has_value(uint32_t thread_id, size_t index) = 0;
    virtual void get_value(uint32_t thread_id, size_t index, int64_t *value) = 0;
    virtual bool next(uint32_t thread_id, size_t *current) = 0;
    virtual void extend_model(Model &m) = 0;
    virtual void on_statistics(UserStatistics &step, UserStatistics &accu) = 0;
};

template < typename T >
void set_value(clingcon_value_t *variant, T value);

template <>
void set_value< int >(clingcon_value_t *variant, int value)
{
    variant->type = clingcon_value_type_int;
    variant->int_number = value;
}

template <>
void set_value< double >(clingcon_value_t *variant, double value)
{
    variant->type = clingcon_value_type_int;
    variant->int_number = static_cast< int >(value);
}

class CSPPropagatorFacade : public PropagatorFacade
{
public:
    CSPPropagatorFacade(clingo_control_t *ctl, Config const &conf)
        : config_(conf)
        , control_(ctl, false)
        , grounder_(control_, step_)
        , normalizer_(grounder_, config_)
    {
        grounder_.deactivate();
        CLINGO_CALL(clingo_control_add(ctl, "base", nullptr, 0, R"(#theory csp {
    linear_term {
    + : 5, unary;
    - : 5, unary;
    * : 4, binary, left;
    + : 3, binary, left;
    - : 3, binary, left
    };
    dom_term {
    + : 5, unary;
    - : 5, unary;
    .. : 1, binary, left;
    * : 4, binary, left;
    + : 3, binary, left;
    - : 3, binary, left
    };
    show_term {
    / : 1, binary, left
    };
    minimize_term {
    + : 5, unary;
    - : 5, unary;
    * : 4, binary, left;
    + : 3, binary, left;
    - : 3, binary, left;
    @ : 0, binary, left
    };
    &dom/0 : dom_term, {=}, linear_term, any;
    &sum/0 : linear_term, {<=,=,>=,<,>,!=}, linear_term, any;
    &show/0 : show_term, directive;
    &distinct/0 : linear_term, any;
    &minimize/0 : minimize_term, directive
}.)"));
    }

    bool prepare(clingo_control_t *ctl) override
    {

        grounder_.activate();
        clingcon::TheoryParser tp(grounder_, normalizer_, control_.theory_atoms());
        {
            Timer tinit(step_.time_init);
            if (!tp.readConstraints())
                throw std::runtime_error(std::string("Something went wrong"));
            // merge names shown in logic program
            auto names = tp.postProcess();
            for (auto i : names)
            {
                auto it = names_.find(i.first);
                if (it != names_.end())
                    it->second.second.insert(it->second.second.begin(), i.second.second.begin(),
                                             i.second.second.end());
                else
                    names_[i.first] = i.second;
            }
            auto symbols = tp.getSymbols();
            symbols_.insert(symbols.begin(), symbols.end());

            for (unsigned int level = 0; level < tp.minimize().size(); ++level)
                for (auto i : tp.minimize()[level])
                {
                    std::vector< clingcon::View > mini;
                    mini.emplace_back(i.second);
                    for (auto &j : mini) normalizer_.addMinimize(j, level);
                }
            bool conflict = false;
            conflict = !normalizer_.prepare();

            if (!conflict) conflict = !normalizer_.propagate();

            if (!conflict) conflict = !normalizer_.finalize();

            if (conflict) grounder_.createClause({grounder_.falseLit()});
            uint64_t numlits = normalizer_.getVariableCreator().numEqualLits();
            for (size_t i = 0; i < normalizer_.getVariableCreator().numVariables(); ++i)
            {
                numlits += normalizer_.getVariableCreator().numOrderLits(Variable(i));
            }
            step_.num_lits = numlits;

            grounder_.deactivate();
        }

        std::vector< clingcon::Variable > lowerBounds, upperBounds;
        normalizer_.variablesWithoutBounds(lowerBounds, upperBounds);
        for (auto i : lowerBounds)
            std::cerr << "Warning: Variable " << tp.getName(i)
                      << " has unrestricted lower bound, set to " << clingcon::Domain::min
                      << std::endl;

        for (auto i : upperBounds)
            std::cerr << "Warning: Variable " << tp.getName(i)
                      << " has unrestricted upper bound, set to " << clingcon::Domain::max
                      << std::endl;

        prop_ = std::make_unique< ClingconPropagator >(
            step_, grounder_.trueLit(), normalizer_.getVariableCreator(), normalizer_.getConfig(),
            &names_, &symbols_, normalizer_.constraints());
        static clingo_propagator_t prop = {init, propagate, undo, check, nullptr};
        CLINGO_CALL(clingo_control_register_propagator(ctl, &prop, prop_.get(), false));
        return true;
    }

    bool lookup_symbol(clingo_symbol_t name, size_t *index) override
    {
        *index = prop_->lookup(name) + 1;
        return *index <= prop_->num_variables();
    }

    clingo_symbol_t get_symbol(size_t index) override
    {
        return prop_->symbol(Var(index - 1)).to_c();
    }

    bool has_value(uint32_t thread_id, size_t index)
    {
        return prop_->has_unique_value(thread_id, Var(index - 1));
    }
    void get_value(uint32_t thread_id, size_t index, int64_t *value)
    {
        assert(index > 0 && index <= prop_->num_variables());
        *value = static_cast< int64_t >(prop_->value(thread_id, Var(index - 1)));
    }

    bool next(uint32_t thread_id, size_t *current)
    {
        for (++*current; *current <= prop_->num_variables(); ++*current)
        {
            if (prop_->has_unique_value(thread_id, Variable(*current - 1)))
            {
                return true;
            }
        }
        return false;
    }
    void extend_model(Model &m) override { prop_->extend_model(m); }
    void on_statistics(UserStatistics &step, UserStatistics &accu) override
    {
        accu_.accu(step_);
        add_statistics(step, step_);
        add_statistics(accu, accu_);
        step_.reset();
    }

    void add_statistics(UserStatistics &root, Stats const &stats)
    {
        UserStatistics clingcon = root.add_subkey("Clingcon", StatisticsType::Map);
        clingcon.add_subkey("Time init(s)", StatisticsType::Value)
            .set_value(stats.time_init.count());
        clingcon.add_subkey("Constraints", StatisticsType::Value)
            .set_value(static_cast< double >(stats.num_constraints));
        clingcon.add_subkey("Integer Variables", StatisticsType::Value)
            .set_value(static_cast< double >(stats.num_int_variables));
        clingcon.add_subkey("Preadded Literals", StatisticsType::Value)
            .set_value(static_cast< double >(stats.num_lits));
        clingcon.add_subkey("Preadded Clauses", StatisticsType::Value)
            .set_value(static_cast< double >(stats.num_clauses));
        UserStatistics threads = clingcon.add_subkey("Thread", StatisticsType::Array);
        threads.ensure_size(stats.clingcon_stats.size(), StatisticsType::Map);
        auto it = threads.begin();
        for (ClingconStats const &stat : stats.clingcon_stats)
        {
            auto thread = *it++;
            thread.add_subkey("Propagation(s)", StatisticsType::Value)
                .set_value(stat.time_propagate.count());
            thread.add_subkey("Undo(s)", StatisticsType::Value)
                .set_value(static_cast< double >(stat.time_undo.count()));
            thread.add_subkey("Order Literals", StatisticsType::Value)
                .set_value(static_cast< double >(stat.num_lits));
            thread.add_subkey("Added Clauses", StatisticsType::Value)
                .set_value(static_cast< double >(stat.num_clauses));
        }
    }

private:
    Stats step_;
    Stats accu_;
    const clingcon::Config &config_;
    std::unique_ptr< ClingconPropagator > prop_{nullptr};
    Clingo::Control control_;
    clingcon::Grounder grounder_;
    clingcon::Normalizer normalizer_;
    clingcon::NameList names_;
    clingcon::SymbolMap symbols_;
};

struct clingcon_theory
{
    std::unique_ptr< PropagatorFacade > clingcon{nullptr};
    bool rdl;
    clingcon::Config config;
};

extern "C" bool clingcon_create(clingcon_theory_t **prop)
{
    CLINGCON_TRY { *prop = new clingcon_theory{}; }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_register(clingcon_theory_t *prop, clingo_control_t *ctl)
{
    CLINGCON_TRY { prop->clingcon = std::make_unique< CSPPropagatorFacade >(ctl, prop->config); }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_destroy(clingcon_theory_t *prop)
{
    CLINGCON_TRY { delete prop; }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_prepare(clingcon_theory_t *prop, clingo_control_t *ctl)
{
    CLINGCON_TRY { return prop->clingcon->prepare(ctl); }
    CLINGCON_CATCH;
}

static char const *iequals_pre(char const *a, char const *b)
{
    for (; *a && *b; ++a, ++b)
    {
        if (tolower(*a) != tolower(*b))
        {
            return nullptr;
        }
    }
    return *b ? nullptr : a;
}
static bool iequals(char const *a, char const *b)
{
    a = iequals_pre(a, b);
    return a && !*a;
}
static char const *parse_uint64_pre(const char *value, void *data)
{
    auto &res = *static_cast< uint64_t * >(data);
    char const *it = value;
    res = 0;

    for (; *it; ++it)
    {
        if ('0' <= *it && *it <= '9')
        {
            auto tmp = res;
            res *= 10;
            res += *it - '0';
            if (res < tmp)
            {
                return nullptr;
            }
        }
        else
        {
            break;
        }
    }

    return value != it ? it : nullptr;
}

static char const *parse_int64_pre(const char *value, void *data)
{
    auto &res = *static_cast< int64_t * >(data);
    std::size_t pos = 0;
    try
    {
        res = std::stoll(value, &pos);
    }
    catch (const std::logic_error &)
    {
        return value;
    }
    return value + pos;
}

static bool parse_uint64(const char *value, void *data)
{
    value = parse_uint64_pre(value, data);
    return value && !*value;
}

static bool parse_int64(const char *value, void *data)
{
    value = parse_int64_pre(value, data);
    return value && !*value;
}


template < typename F, typename G >
bool set_config(char const *value, void *data, F f, G g)
{
    try
    {
        auto &config = *static_cast< clingcon::Config * >(data);
        // uint64_t id = 0;
        if (*value == '\0')
        {
            f(config);
            return true;
        }
        // else if (*value == ',' && parse_uint64(value + 1, &id) && id < 64) {
        //    can be used for per thread options
        //    //g(config.ensure(id));
        //    return true;
        //}
    }
    catch (...)
    {
    }
    return false;
}

static bool parse_translate_constraints(const char *value, void *data)
{
    int64 x = 0;
    return (value = parse_int64_pre(value, &x)) && x >= -1ll &&
           set_config(value, data,
                      [x](clingcon::Config &config) { config.translateConstraints = x; },
                      [x](int a) {});
}

static bool parse_prop_strength(const char *value, void *data)
{
    uint64 x = 0;
    return (value = parse_uint64_pre(value, &x)) && 1ull <= x && x <= 4ull &&
           set_config(value, data, [x](clingcon::Config &config) { config.propStrength = x; },
                      [x](int a) {});
}

static bool parse_min_lits_per_var(const char *value, void *data)
{
    int64 x = 0;
    return (value = parse_int64_pre(value, &x)) && x >= -1ll &&
           set_config(value, data, [x](clingcon::Config &config) { config.minLitsPerVar = x; },
                      [x](int a) {});
}

static bool parse_domain_propagation(const char *value, void *data)
{
    int64 x = 0;
    return (value = parse_int64_pre(value, &x)) && x >= -1ll &&
           set_config(value, data, [x](clingcon::Config &config) { config.domSize = x; },
                      [x](int a) {});
}

static bool parse_bool(const char *value, void *data)
{
    auto &result = *static_cast< bool * >(data);
    if (iequals(value, "no") || iequals(value, "off") || iequals(value, "0"))
    {
        result = false;
        return true;
    }
    if (iequals(value, "yes") || iequals(value, "on") || iequals(value, "1"))
    {
        result = true;
        return true;
    }
    return false;
}

static bool check_parse(char const *key, bool ret)
{
    if (!ret)
    {
        std::ostringstream msg;
        msg << "invalid value for '" << key << "'";
        clingo_set_error(clingo_error_runtime, msg.str().c_str());
    }
    return ret;
}

extern "C" bool clingcon_configure_theory(clingcon_theory_t *prop, char const *key,
                                          char const *value)
{
    CLINGCON_TRY
    {
        if (strcmp(key, "translate-constraints") == 0)
        {
            return check_parse("translate-constraints",
                               parse_translate_constraints(value, &prop->config));
        }
        if (strcmp(key, "prop-strength") == 0)
        {
            return check_parse("prop-strength", parse_prop_strength(value, &prop->config));
        }
        if (strcmp(key, "min-lits-per-var") == 0)
        {
            return check_parse("min-lits-per-var", parse_min_lits_per_var(value, &prop->config));
        }
        if (strcmp(key, "domain-propagation") == 0)
        {
            return check_parse("domain-propagation",
                               parse_domain_propagation(value, &prop->config));
        }
        if (strcmp(key, "distinct-to-card") == 0)
        {
            return check_parse("distinct-to-card",
                               parse_bool(value, &prop->config.alldistinctCard));
        }
        std::ostringstream msg;
        msg << "invalid configuration key '" << key << "'";
        clingo_set_error(clingo_error_runtime, msg.str().c_str());
        return false;
    }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_register_options(clingcon_theory_t *prop, clingo_options_t *options)
{
    CLINGCON_TRY
    {
        char const *group = "Clingcon Options";
        prop->config.alldistinctCard = false;
        prop->config.domSize = 10000;
        prop->config.minLitsPerVar = 1000;
        prop->config.propStrength = 4;
        prop->config.translateConstraints = 10000;
        CLINGO_CALL(clingo_options_add(options, group, "translate-constraints",
                                       "Translate constraints with an estimated "
                                       "number of nogoods less than <n> (-1=all) (default: 10000).",
                                       &parse_translate_constraints, &(prop->config), false,
                                       "<n>"));
        CLINGO_CALL(
            clingo_options_add(options, group, "prop-strength",
                               "Propagation strength <n> {1=weak .. 4=strong} (default: 4) ",
                               &parse_prop_strength, &(prop->config), false, "<n>"));
        CLINGO_CALL(clingo_options_add(
            options, group, "min-lits-per-var",
            "Creates at least <n> literals per variable (-1=all) (default: 1000)",
            &parse_min_lits_per_var, &(prop->config), false, "<n>"));
        CLINGO_CALL(clingo_options_add(options, group, "domain-propagation",
                                       "Restrict the exponential runtime behaviour of "
                                       "domain propagation (-1=full propagation) "
                                       "(default: 10000)",
                                       &parse_domain_propagation, &(prop->config), false, "<n>"));
        CLINGO_CALL(clingo_options_add_flag(
            options, group, "distinct-to-card",
            "Translate distinct constraint using cardinality constraints (default: false)",
            &prop->config.alldistinctCard));
    }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_validate_options(clingcon_theory_t *)
{
    CLINGCON_TRY {}
    CLINGCON_CATCH;
}

extern "C" bool clingcon_on_model(clingcon_theory_t *prop, clingo_model_t *model)
{
    CLINGCON_TRY
    {
        Model m(model);
        prop->clingcon->extend_model(m);
    }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_lookup_symbol(clingcon_theory_t *prop_, clingo_symbol_t symbol,
                                       size_t *index)
{
    return prop_->clingcon->lookup_symbol(symbol, index);
}

extern "C" clingo_symbol_t clingcon_get_symbol(clingcon_theory_t *prop, size_t index)
{
    return prop->clingcon->get_symbol(index);
}

extern "C" void clingcon_assignment_begin(clingcon_theory_t *, uint32_t, size_t *current)
{
    *current = 0;
}

extern "C" bool clingcon_assignment_next(clingcon_theory_t *prop, uint32_t thread_id, size_t *index)
{
    return prop->clingcon->next(thread_id, index);
}

extern "C" bool clingcon_assignment_has_value(clingcon_theory_t *prop, uint32_t thread_id,
                                              size_t index)
{
    return prop->clingcon->has_value(thread_id, index);
}

extern "C" void clingcon_assignment_get_value(clingcon_theory_t *prop, uint32_t thread_id,
                                              size_t index, int64_t *value)
{
    prop->clingcon->get_value(thread_id, index, value);
}

extern "C" bool clingcon_on_statistics(clingcon_theory_t *prop, clingo_statistics_t *step,
                                       clingo_statistics_t *accu)
{
    CLINGCON_TRY
    {
        uint64_t root_s, root_a;
        CLINGO_CALL(clingo_statistics_root(step, &root_s));
        CLINGO_CALL(clingo_statistics_root(accu, &root_a));
        UserStatistics s(step, root_s);
        UserStatistics a(accu, root_a);
        prop->clingcon->on_statistics(s, a);
    }
    CLINGCON_CATCH;
}

#undef CLINGCON_TRY
#undef CLINGCON_CATCH
#undef CLINGO_CALL
