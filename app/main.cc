#include <clingo.hh>
#include <clingcon.h>

using namespace Clingo;

class ClingconApp : public Application {
public:
    ClingconApp() { }

    char const *program_name() const noexcept override {
        return "clingcon";
    }

    char const *version() const noexcept override {
        return CLINGCON_VERSION;
    }

    void main(Control &ctl, StringSpan files) override {
        // load files into the control object
        for (auto &file : files) { ctl.load(file); }
        // if no files are given read from stdin
        if (files.empty()) { ctl.load("-"); }

        // ground & solve
        ctl.ground({{ "base", {} }});
        ctl.solve(LiteralSpan{}, nullptr, false, false).get();
    }
};

int main(int argc, char const **argv) {
    ClingconApp app;
    return clingo_main(app, {argv+1, static_cast<size_t>(argc)-1});
}
