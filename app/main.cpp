// {{{ MIT License

// Copyright 2019 Roland Kaminski, Philipp Wanko, Max Ostrowski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#include <clingo.hh>
#include <clingcon.h>
#include <sstream>

using namespace Clingo;

#define CLINGO_CALL(x) Clingo::Detail::handle_error(x)

class ClingconApp : public Clingo::Application, private SolveEventHandler {
public:
    ClingconApp() {
        CLINGO_CALL(clingcon_create_propagator(&prop_));
    }
    ~ClingconApp() { clingcon_destroy_propagator(prop_); }
    char const *program_name() const noexcept override { return "clingcon"; }
    char const *version() const noexcept override { return CLINGCON_VERSION; }
    bool on_model(Model &model) override {
        CLINGO_CALL(clingcon_on_model(prop_, model.to_c()));
        return true;
    }

//    int get_bound(Model const &m) {
//        if (!bound_index_) {
//            if (!clingcon_lookup_symbol(prop_, bound_symbol.to_c(), &bound_index_)) {
//                throw std::runtime_error("variable to minimize not found");
//            }
//        }
//        if (!clingcon_assignment_has_value(prop_, m.thread_id(), bound_index_)) {
//            throw std::runtime_error("variable to minimize is unassigned");
//        }
//        clingcon_value_t value;
//        clingcon_assignment_get_value(prop_, m.thread_id(), bound_index_, &value);
//        // NOTE: minimizinig real values would require an epsilon
//        if (value.type != clingcon_value_type_int) {
//            throw std::runtime_error("only integer minimization is supported");
//        }
//        return value.int_number;
//
//    }
    void print_model(Model const &model, std::function<void()> default_printer) noexcept override {
        default_printer();
        // NOTE: this is printed as part of the model but not as part of the optimization value
//        if (minimize_) {
//            std::cout << "DL Optimization: " << get_bound(model) << "\n";
//        }
    }

    void add_stats(UserStatistics root) {
 //       if (found_bound_) {
//            UserStatistics diff = root.add_subkey("DifferenceLogic", StatisticsType::Map);
//            diff.add_subkey("Optimization", StatisticsType::Value).set_value(bound_value_);
//        }
    }

    void on_statistics(UserStatistics step, UserStatistics accu) override {
//        add_stats(step);
//        add_stats(accu);
//        CLINGO_CALL(clingcon_on_statistics(prop_, step.to_c(), accu.to_c()));
    }

    void main(Control &ctl, StringSpan files) override {
        CLINGO_CALL(clingcon_register_propagator(prop_, ctl.to_c()));
        for (auto &file : files) {
            ctl.load(file);
        }
        if (files.empty()) {
            ctl.load("-");
        }

        CLINGO_CALL(clingcon_pre_ground(prop_, ctl.to_c()));
        ctl.ground({{"base", {}}});
        CLINGO_CALL(clingcon_post_ground(prop_, ctl.to_c()));

//        if (!minimize_) {
            ctl.solve(Clingo::SymbolicLiteralSpan{}, this, false, false).get();
//        }
//        else {
//            // NOTE: with an API extension to implement a custom enumerator,
//            //       one could implement this more nicely
//            //       right now this implementation is restricted to the application
//            ctl.add("__bound", {"s", "b"}, "&diff { s-0 } <= b.");
//            do
//            {
//                if (has_bound_) {
//                    ctl.ground({{"__bound", {bound_symbol, Number(bound_value_ - 1)}}});
//                }
//                has_bound_ = false;
//                auto h = ctl.solve(Clingo::SymbolicLiteralSpan{}, this, false, true);
//                for (auto &&m : h) {
//                    bound_value_ = get_bound(m);
//                    has_bound_ = true;
//                    found_bound_ = true;
//                    break;
//                }
//            }
//            while (has_bound_);
//        }

    }

//    bool parse_bound(char const *value) {
//        std::ostringstream oss;
//        oss << "(" << value << ",)";
//        auto term = Clingo::parse_term(oss.str().c_str());
//        auto args = term.arguments();
//        auto size = args.size();
//        if (args.empty() || size > 2 || (size > 1 && args[1].type() != Clingo::SymbolType::Number)) {
//            return false;
//        }
//        minimize_ = true;
//        bound_symbol = args[0];
//        if (size > 1) {
//            has_bound_ = true;
//            bound_value_ = args[1].number();
//        }
//        return true;
//    }

    void register_options(ClingoOptions &options) override {
        CLINGO_CALL(clingcon_register_options(prop_, options.to_c()));
//        char const * group = "Clingo.DL Options";
//        options.add(group, "minimize-variable",
//            "Minimize the given variable\n"
//            "      <arg>   : <variable>[,<initial>]\n"
//            "      <variable>: the variable to minimize_\n"
//            "      <initial> : upper bound for the variable\n",
//            [this](char const *value) { return parse_bound(value); });
    }

    void validate_options() override {
        CLINGO_CALL(clingcon_validate_options(prop_));
    }
private:
    clingcon_propagator_t *prop_;
//    Clingo::Symbol bound_symbol;
//    size_t bound_index_ = 0;
//    int bound_value_ = 0;
//    bool minimize_ = false;
//    bool has_bound_ = false;
//    bool found_bound_ = false;
};

int main(int argc, char *argv[]) {
    ClingconApp app;
    return Clingo::clingo_main(app, {argv + 1, static_cast<size_t>(argc - 1)});
}


#undef CLINGO_CALL
