#include <clingcon.h>

#include <clingo/app.hh>
#include <clingo/theory.hh>

#include <iostream>
#include <utility>

#ifdef CLINGCON_PROFILE
#include <gperftools/profiler.h>
#endif

namespace Clingcon {

//! Application class to run clingcon.
class App : public Clingo::App, private Clingo::SolveEventHandler {
  public:
    App(Clingo::Library lib) : lib_{std::move(lib)} {}
    //! Set program name to clingcon.
    auto do_program_name() noexcept -> std::string_view override { return "clingcon"; }
    //! Set the version.
    auto do_version() noexcept -> std::string_view override { return CLINGCON_VERSION; }
    //! Pass models to the theory.
    auto do_model(Clingo::Model model) -> bool override {
        theory_.model(model);
        return true;
    }
    //! Pass statistics to the theory.
    void do_stats(Clingo::Stats step, Clingo::Stats accu) override { theory_.stats(step, accu); }
    //! Run main solving function.
    void do_main(Clingo::Control const &ctl, Clingo::StringSpan files) override { // NOLINT
        theory_.register_theory(ctl);
        theory_.rewrite(lib_, ctl, files);
        if (ctl.mode() == Clingo::ControlMode::solve) {
            ctl.ground();
#ifdef CLINGCON_PROFILE
            ProfilerStart("clingcon.solve.prof");
#endif
            theory_.prepare(ctl);
            std::ignore = ctl.solve(*this).get();
#ifdef CLINGCON_PROFILE
            ProfilerStop();
#endif
        } else {
            ctl.main();
        }
    }
    void do_print_model(Clingo::ConstModel model, Clingo::ModelPrinter const &default_printer) override {
        static_cast<void>(default_printer);
        try {
            auto symbols = model.symbols(Clingo::ShowFlags::shown);

            // print model
            bool comma = false;
            std::ranges::sort(symbols);
            for (auto &sym : symbols) {
                if (!sym.match("__csp", 2) && !sym.match("__csp_cost", 1)) {
                    std::cout << (comma ? " " : "") << sym;
                    comma = true;
                }
            }

            // print assignment
            std::cout << "\nAssignment:\n";
            comma = false;
            auto cost = std::optional<Clingo::Symbol>{};
            for (auto &sym : symbols) {
                if (sym.match("__csp", 2)) {
                    auto arguments = sym.arguments();
                    std::cout << (comma ? " " : "") << arguments[0] << "=" << arguments[1];
                    comma = true;

                } else if (sym.match("__csp_cost", 1)) {
                    auto arguments = sym.arguments();
                    cost = arguments[0];
                }
            }
            std::cout << "\n";

            // print cost
            if (cost) {
                if (cost->type() == Clingo::SymbolType::string) {
                    std::cout << "Cost: " << cost->string() << "\n";
                } else {
                    std::cout << "Cost: " << *cost << "\n";
                }
            }

            std::cerr.flush();
        } catch (...) {
            fprintf(stderr, "panic: printing model failed\n");
            std::terminate();
        }
    }

    //! Register options of the theory and optimization related options.
    void do_register_options(Clingo::Options options) override {
        using namespace std::string_view_literals;
        theory_.register_options(options);
    }
    //! Validate options of the theory.
    void do_validate_options() override { theory_.validate_options(); }

  private:
    Clingo::Library lib_;
    Clingo::Theory theory_{lib_, clingcon_create};
    std::vector<Clingo::Symbol> symvec_;
};

} // namespace Clingcon

//! Run the clingcon application.
auto main(int argc, char *argv[]) -> int { // NOLINT(bugprone-exception-escape)
    Clingo::Library lib;
    Clingcon::App app{lib};
    auto args = std::vector<std::string_view>{argv + 1, argv + argc};
    return Clingo::main(lib, args, &app);
}
