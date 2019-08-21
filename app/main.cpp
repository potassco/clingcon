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

        ctl.solve(Clingo::SymbolicLiteralSpan{}, this, false, false).get();
    }

    void register_options(ClingoOptions &options) override {
        CLINGO_CALL(clingcon_register_options(prop_, options.to_c()));
   }

    void validate_options() override {
        CLINGO_CALL(clingcon_validate_options(prop_));
    }
private:
    clingcon_propagator_t *prop_;
};

int main(int argc, char *argv[]) {
    ClingconApp app;
    return Clingo::clingo_main(app, {argv + 1, static_cast<size_t>(argc - 1)});
}


#undef CLINGO_CALL
