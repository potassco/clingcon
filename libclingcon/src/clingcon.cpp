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
#define CLINGCON_CATCH catch (...){ Clingo::Detail::handle_cxx_error(); return false; } return true

#define CLINGO_CALL(x) Clingo::Detail::handle_error(x)

template <typename T>
bool init(clingo_propagate_init_t* i, void* data)
{
    CLINGCON_TRY {
        PropagateInit in(i);
        static_cast<ClingconPropagator*>(data)->init(in);
    }
    CLINGCON_CATCH;
}

template <typename T>
bool propagate(clingo_propagate_control_t* i, const clingo_literal_t *changes, size_t size, void* data)
{
    CLINGCON_TRY {
        PropagateControl in(i);
        static_cast<ClingconPropagator*>(data)->propagate(in, {changes, size});
    }
    CLINGCON_CATCH;
}

template <typename T>
bool undo(clingo_propagate_control_t const* i, const clingo_literal_t *changes, size_t size, void* data)
{
    CLINGCON_TRY {
        PropagateControl in(const_cast<clingo_propagate_control_t *>(i));
        static_cast<ClingconPropagator*>(data)->undo(in, {changes, size});
    }
    CLINGCON_CATCH;
}

template <typename T>
bool check(clingo_propagate_control_t* i, void* data)
{
    CLINGCON_TRY {
        PropagateControl in(i);
        static_cast<ClingconPropagator*>(data)->check(in);
    }
    CLINGCON_CATCH;
}

struct PropagatorConfig {
// TODO: fill with option flags etc...
};

struct Stats {
// TODO: fill with stats information
};

struct PropagatorFacade {
public:
    virtual ~PropagatorFacade() {};
    virtual bool lookup_symbol(clingo_symbol_t name, size_t *index) = 0;
    virtual clingo_symbol_t get_symbol(size_t index) = 0;
    virtual bool has_value(uint32_t thread_id, size_t index) = 0;
    virtual void get_value(uint32_t thread_id, size_t index, clingcon_value_t *value) = 0;
    virtual bool next(uint32_t thread_id, size_t *current) = 0;
    virtual void extend_model(Model &m) = 0;
    virtual void on_statistics(UserStatistics& step, UserStatistics &accu) = 0;
};

template<typename T>
void set_value(clingcon_value_t *variant, T value);

template<>
void set_value<int>(clingcon_value_t *variant, int value) {
    variant->type = clingcon_value_type_int;
    variant->int_number = value;
}

template<>
void set_value<double>(clingcon_value_t *variant, double value) {
    variant->type = clingcon_value_type_int;
    variant->int_number = value;
}

class CSPPropagatorFacade : public PropagatorFacade {
public:
    CSPPropagatorFacade(clingo_control_t *ctl, PropagatorConfig const &conf)
    // here i need a backend, but maybe I'm not allowed to get one but only the back_end_t via the clingo function clingo_control_backend
    : grounder_(ctl->backend()),
      normalizer_(grounder_,nullptr)
    {

        CLINGO_CALL(clingo_control_add(ctl,"base", nullptr, 0, R"(#theory csp {
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

    bool pre_ground(clingo_control_t *ctl) {
        return true;
    };

    bool post_ground(clingo_control_t *ctl) {
        tp_ = std::make_unique<clingcon::TheoryParser>(grounder_,
                                                       normalizer_,
                                                       ctl->theory_atoms());
        if (!tp->readConstraints()) throw std::runtime_error(std::string("Something went wrong"));
        names_ = tp->postProcess();


        for (unsigned int level = 0; level < tp->minimize().size(); ++level)
            for (auto i : tp->minimize()[level])
            {
                std::vector<clingcon::View> mini;
                mini.emplace_back(i.second);
                for (auto& j : mini)
                   normalizer_.addMinimize(j,level);

            }
        bool conflict = false;
        conflict = !normalizer_.prepare();
    
        if (!conflict) conflict = !normalizer_.propagate();
    
        if (!conflict) conflict = !normalizer_.finalize();
    
        if (conflict) ctl.backend().rule(false, {}, {});
    
        std::vector< clingcon::Variable > lowerBounds, upperBounds;
        normalizer_.variablesWithoutBounds(lowerBounds, upperBounds);
        for (auto i : lowerBounds)
            std::cerr << "Warning: Variable " << tp->getName(i)
                      << " has unrestricted lower bound, set to " << clingcon::Domain::min << std::endl;
    
        for (auto i : upperBounds)
            std::cerr << "Warning: Variable " << tp->getName(i)
                      << " has unrestricted upper bound, set to " << clingcon::Domain::max << std::endl;
    



        prop_ = std::make_unique<ClingconPropagator>(grounder_.trueLit(),
                                                     normalizer_.getVariableCreator(),
                                                     normalizer_.getConfig(),
                                                     &names_,
                                                     normalizer_.constraints());
        static clingo_propagator_t prop = {
            init,
            propagate,
            undo,
            check,
            nullptr
        };
        CLINGO_CALL(clingo_control_register_propagator(ctl, &prop, &prop_, false));
    }

    bool lookup_symbol(clingo_symbol_t name, size_t *index) override {
        *index = prop_.lookup(name) + 1;
        return *index <= prop_.num_vertices();
    }

    clingo_symbol_t get_symbol(size_t index) override {
        return prop_.symbol(index - 1).to_c();
    }

    bool has_value(uint32_t thread_id, size_t index) override {
        return prop_.has_lower_bound(thread_id, index - 1);
    }
    void get_value(uint32_t thread_id, size_t index, clingcon_value_t *value) override {
        assert(index > 0 && index <= prop_.num_vertices());
        set_value(value, prop_.lower_bound(thread_id, index - 1));
    }

    bool next(uint32_t thread_id, size_t *current) override {
        for (++*current; *current <= prop_.num_vertices(); ++*current) {
            if (prop_.has_lower_bound(thread_id, *current - 1)) {
                return true;
            }
        }
        return false;
    }
    void extend_model(Model &m) override {
        prop_.extend_model(m);
    }
    void on_statistics(UserStatistics& step, UserStatistics &accu) override {
        accu_.accu(step_);
        add_statistics(step, step_);
        add_statistics(accu, accu_);
        step_.reset();
    }

    void add_statistics(UserStatistics& root, Stats const &stats) {
        //UserStatistics diff = root.add_subkey("DifferenceLogic", StatisticsType::Map);
        //diff.add_subkey("Time init(s)", StatisticsType::Value).set_value(stats.time_init.count());
//        diff.add_subkey("Mutexes", StatisticsType::Value).set_value(stats.mutexes);
//        UserStatistics threads = diff.add_subkey("Thread", StatisticsType::Array);
//        threads.ensure_size(stats.dl_stats.size(), StatisticsType::Map);
//        auto it = threads.begin();
//        for (DLStats const& stat : stats.dl_stats) {
//            auto thread = *it++;
//            thread.add_subkey("Propagation(s)", StatisticsType::Value).set_value(stat.time_propagate.count());
//            thread.add_subkey("Dijkstra(s)", StatisticsType::Value).set_value(stat.time_dijkstra.count());
//            thread.add_subkey("Undo(s)", StatisticsType::Value).set_value(stat.time_undo.count());
//            thread.add_subkey("True edges", StatisticsType::Value).set_value(stat.true_edges);
//            thread.add_subkey("False edges", StatisticsType::Value).set_value(stat.false_edges);
//            thread.add_subkey("False edges (inverse)", StatisticsType::Value).set_value(stat.false_edges_trivial);
//            thread.add_subkey("False edges (partial)", StatisticsType::Value).set_value(stat.false_edges_weak);
//            thread.add_subkey("False edges (partial+)", StatisticsType::Value).set_value(stat.false_edges_weak_plus);
//            thread.add_subkey("Edges added", StatisticsType::Value).set_value(stat.edges_added);
//            thread.add_subkey("Edges skipped", StatisticsType::Value).set_value(stat.edges_skipped);
//            thread.add_subkey("Edges propagated", StatisticsType::Value).set_value(stat.edges_propagated);
//            thread.add_subkey("Cost consistency", StatisticsType::Value).set_value(stat.propagate_cost_add);
//            thread.add_subkey("Cost forward", StatisticsType::Value).set_value(stat.propagate_cost_from);
//            thread.add_subkey("Cost backward", StatisticsType::Value).set_value(stat.propagate_cost_to);
//        }
    }

private:
    Stats step_;
    Stats accu_;
    std::unique_ptr<ClingconPropagator> prop_{nullptr};
    clingcon::Grounder grounder_;
    clingcon::Normalizer normalizer_;
    std::unique_ptr<clingcon::TheoryParser> tp_;
    clingcon::NameList names_;
};

struct clingcon_propagator {
    std::unique_ptr<PropagatorFacade> clingcon{nullptr};
    bool rdl;
    PropagatorConfig config;
};

extern "C" bool clingcon_create_propagator(clingcon_propagator_t **prop) {
    CLINGCON_TRY { *prop = new clingcon_propagator{}; }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_register_propagator(clingcon_propagator_t *prop, clingo_control_t* ctl) {
    CLINGCON_TRY {
        if (!prop->rdl) {
            prop->clingcon = std::make_unique<CSPPropagatorFacade<int>>(ctl, prop->config);
        }
        else {
            prop->clingcon = std::make_unique<CSPPropagatorFacade<double>>(ctl, prop->config);
        }
    }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_destroy_propagator(clingcon_propagator_t *prop) {
    CLINGCON_TRY { delete prop; }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_pre_ground(clingcon_propagator_t *prop, clingo_control_t* ctl) {
    CLINGCON_TRY { return prop->pre_ground(ctl); }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_post_ground(clingcon_propagator_t *prop, clingo_control_t* ctl) {
    CLINGCON_TRY { return prop->post_ground(ctl); }
    CLINGCON_CATCH;
}

static char const *iequals_pre(char const *a, char const *b) {
    for (; *a && *b; ++a, ++b) {
        if (tolower(*a) != tolower(*b)) { return nullptr; }
    }
    return *b ? nullptr : a;
}
static bool iequals(char const *a, char const *b) {
    a = iequals_pre(a, b);
    return a && !*a;
}
static char const *parse_uint64_pre(const char *value, void *data) {
    auto &res = *static_cast<uint64_t*>(data);
    char const *it = value;
    res = 0;

    for (; *it; ++it) {
        if ('0' <= *it && *it <= '9') {
            auto tmp = res;
            res *= 10;
            res += *it - '0';
            if (res < tmp) { return nullptr; }
        }
        else { break; }
    }

    return value != it ? it : nullptr;
}
static bool parse_uint64(const char *value, void *data) {
    value = parse_uint64_pre(value, data);
    return value && !*value;
}

template <typename F, typename G>
bool set_config(char const *value, void *data, F f, G g) {
    try {
        auto &config = *static_cast<PropagatorConfig*>(data);
        uint64_t id = 0;
        if (*value == '\0') {
            f(config);
            return true;
        }
        else if (*value == ',' && parse_uint64(value + 1, &id) && id < 64) {
            g(config.ensure(id));
            return true;
        }
    }
    catch (...) { }
    return false;
}

//static bool parse_root(const char *value, void *data) {
//    uint64_t x = 0;
//    return (value = parse_uint64_pre(value, &x)) && set_config(value, data,
//        [x](PropagatorConfig &config) { config.propagate_root = x; },
//        [x](ThreadConfig &config) { config.propagate_root = {true, x}; });
//}
//static bool parse_budget(const char *value, void *data) {
//    uint64_t x = 0;
//    return (value = parse_uint64_pre(value, &x)) && set_config(value, data,
//        [x](PropagatorConfig &config) { config.propagate_budget = x; },
//        [x](ThreadConfig &config) { config.propagate_budget = {true, x}; });
//}
//static bool parse_mutex(const char *value, void *data) {
//    auto &pc = *static_cast<PropagatorConfig*>(data);
//    uint64_t x = 0;
//    if (!(value = parse_uint64_pre(value, &x))) { return false; }
//    pc.mutex_size = x;
//    if (*value == '\0') {
//        pc.mutex_cutoff = 10 * x;
//        return true;
//    }
//    if (*value == ',') {
//        if (!parse_uint64(value+1, &x)) { return false; }
//        pc.mutex_cutoff = x;
//    }
//    return true;
//}
//static bool parse_mode(const char *value, void *data) {
//    PropagationMode mode = PropagationMode::Check;
//    char const *rem = nullptr;
//    if ((rem = iequals_pre(value, "no"))) {
//        mode = PropagationMode::Check;
//    }
//    else if ((rem = iequals_pre(value, "inverse"))) {
//        mode = PropagationMode::Trivial;
//    }
//    else if ((rem = iequals_pre(value, "partial+"))) {
//        mode = PropagationMode::WeakPlus;
//    }
//    else if ((rem = iequals_pre(value, "partial"))) {
//        mode = PropagationMode::Weak;
//    }
//    else if ((rem = iequals_pre(value, "full"))) {
//        mode = PropagationMode::Strong;
//    }
//    return rem && set_config(rem, data,
//        [mode](PropagatorConfig &config) { config.mode = mode; },
//        [mode](ThreadConfig &config) { config.mode = {true, mode}; });
//}
//static bool parse_bool(const char *value, void *data) {
//    auto &result = *static_cast<bool*>(data);
//    if (iequals(value, "no") || iequals(value, "off") || iequals(value, "0")) {
//        result = false;
//        return true;
//    }
//    if (iequals(value, "yes") || iequals(value, "on") || iequals(value, "1")) {
//        result = true;
//        return true;
//    }
//    return false;
//}
//
static bool check_parse(char const *key, bool ret) {
    if (!ret) {
        std::ostringstream msg;
        msg << "invalid value for '" << key << "'";
        clingo_set_error(clingo_error_runtime, msg.str().c_str());
    }
    return ret;
}

extern "C" bool clingcon_configure_propagator(clingcon_propagator_t *prop, char const *key, char const *value) {
    CLINGCON_TRY {
//        if (strcmp(key, "propagate") == 0) {
//            return check_parse("propagate", parse_mode(value, &prop->config));
//        }
//        if (strcmp(key, "propagate-root") == 0) {
//            return check_parse("propagate-root", parse_root(value, &prop->config));
//        }
//        if (strcmp(key, "propagate-budget") == 0) {
//            return check_parse("propgate-budget", parse_budget(value, &prop->config));
//        }
//        if (strcmp(key, "add-mutexes") == 0) {
//            return check_parse("add-mutexes", parse_mutex(value, &prop->config));
//        }
//        if (strcmp(key, "rdl") == 0) {
//            return check_parse("rdl", parse_bool(value, &prop->rdl));
//        }
//        if (strcmp(key, "strict") == 0) {
//            return check_parse("strict", parse_bool(value, &prop->config));
//        }
        std::ostringstream msg;
        msg << "invalid configuration key '" << key << "'";
        clingo_set_error(clingo_error_runtime, msg.str().c_str());
        return false;
    }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_register_options(clingcon_propagator_t *prop, clingo_options_t* options) {
    CLINGCON_TRY {
//        char const * group = "Clingo.DL Options";
//        CLINGO_CALL(clingo_options_add(options, group, "propagate",
//            "Set propagation mode [no]\n"
//            "      <mode>  : {no,inverse,partial,partial+,full}[,<thread>]\n"
//            "        no      : No propagation; only detect conflicts\n"
//            "        inverse : Check inverse constraints\n"
//            "        partial : Detect some conflicting constraints\n"
//            "        partial+: Detect some more conflicting constraints\n"
//            "        full    : Detect all conflicting constraints\n"
//            "      <thread>: Restrict to thread",
//            &parse_mode, &prop->config, true, "<mode>"));
//        CLINGO_CALL(clingo_options_add(options, group, "propagate-root",
//            "Enable full propagation below decision level [0]\n"
//            "      <arg>   : <n>[,<thread>]\n"
//            "      <n>     : Upper bound for decision level\n"
//            "      <thread>: Restrict to thread",
//            &parse_root, &prop->config, true, "<arg>"));
//        CLINGO_CALL(clingo_options_add(options, group, "propagate-budget",
//            "Enable full propagation limiting to budget [0]\n"
//            "      <arg>   : <n>[,<thread>]\n"
//            "      <n>     : Budget roughly corresponding to cost of consistency checks\n"
//            "                (if possible use with --propagate-root greater 0)\n"
//            "      <thread>: Restrict to thread",
//            &parse_budget, &prop->config, true, "<arg>"));
//        CLINGO_CALL(clingo_options_add(options, group, "add-mutexes",
//            "Add mutexes in a preprocessing step [0]\n"
//            "      <arg>   : <max>[,<cut>]\n"
//            "      <max>   : Maximum size of mutexes to add\n"
//            "      <cut>   : Limit costs to calculate mutexes\n",
//            &parse_mutex, &prop->config, true, "<arg>"));
//        CLINGO_CALL(clingo_options_add_flag(options, group, "rdl", "Enable support for real numbers", &prop->rdl));
//        CLINGO_CALL(clingo_options_add_flag(options, group, "strict", "Enable strict mode", &prop->config.strict));
    }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_validate_options(clingcon_propagator_t *prop) {
    CLINGCON_TRY {
//        if (prop->config.strict && prop->rdl) {
//            throw std::runtime_error("real difference logic not available with strict semantics");
//        }
    }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_on_model(clingcon_propagator_t *prop, clingo_model_t* model) {
    CLINGCON_TRY {
        Model m(model);
        prop->clingcon->extend_model(m);
    }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_lookup_symbol(clingcon_propagator_t *prop_, clingo_symbol_t symbol, size_t *index) {
    return prop_->clingcon->lookup_symbol(symbol, index);
}

extern "C" clingo_symbol_t clingcon_get_symbol(clingcon_propagator_t *prop, size_t index) {
    return prop->clingcon->get_symbol(index);
}

extern "C" void clingcon_assignment_begin(clingcon_propagator_t *, uint32_t, size_t *current) {
    *current = 0;
}

extern "C" bool clingcon_assignment_next(clingcon_propagator_t *prop, uint32_t thread_id, size_t *index) {
    return prop->clingcon->next(thread_id, index);
}

extern "C" bool clingcon_assignment_has_value(clingcon_propagator_t *prop, uint32_t thread_id, size_t index) {
    return prop->clingcon->has_value(thread_id, index);
}

extern "C" void clingcon_assignment_get_value(clingcon_propagator_t *prop, uint32_t thread_id, size_t index, clingcon_value_t *value) {
    prop->clingcon->get_value(thread_id, index, value);
}

extern "C" bool clingcon_on_statistics(clingcon_propagator_t *prop, clingo_statistics_t* step, clingo_statistics_t* accu) {
    CLINGCON_TRY {
//        uint64_t root_s, root_a;
//        CLINGO_CALL(clingo_statistics_root(step, &root_s));
//        CLINGO_CALL(clingo_statistics_root(accu, &root_a));
//        UserStatistics s(step, root_s);
//        UserStatistics a(accu, root_a);
//        prop->clingcon->on_statistics(s, a);
    }
    CLINGCON_CATCH;
}

#undef CLINGCON_TRY
#undef CLINGCON_CATCH
#undef CLINGO_CALL
