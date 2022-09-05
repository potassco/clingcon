#include <clingo.hh>
#include <clingcon.h>
#include <sstream>
#include <fstream>
#include <optional>

#ifdef CLINGCON_PROFILE
#include <gperftools/profiler.h>
#endif

using Clingo::Detail::handle_error;


class Rewriter {
public:
    Rewriter(clingcon_theory_t *theory, clingo_program_builder_t *builder)
    : theory_{theory}
    , builder_{builder} {
    }

    void rewrite(Clingo::Control &control, Clingo::StringSpan files) {
        handle_error(clingo_ast_parse_files(files.begin(), files.size(), rewrite_, this, control.to_c(), nullptr, nullptr, 0));
    }

private:
    static bool add_(clingo_ast_t *stm, void *data) {
        auto *self = static_cast<Rewriter*>(data);
        return clingo_program_builder_add(self->builder_, stm);
    }

    static bool rewrite_(clingo_ast_t *stm, void *data) {
        auto *self = static_cast<Rewriter*>(data);
        return clingcon_rewrite_ast(self->theory_, stm, add_, self);
    }

    clingcon_theory_t *theory_;
    clingo_program_builder_t *builder_;
};


class ClingconApp final : public Clingo::Application, private Clingo::SolveEventHandler {
public:
    ClingconApp() {
        handle_error(clingcon_create(&theory_));
    }

    ClingconApp(ClingconApp const &) = delete;
    ClingconApp(ClingconApp &&) = delete;
    ClingconApp &operator=(ClingconApp const &) = delete;
    ClingconApp &operator=(ClingconApp &&) = delete;

    ~ClingconApp() override {
        if (theory_ != nullptr) {
            clingcon_destroy(theory_);
        }
    }

    [[nodiscard]] char const *program_name() const noexcept override {
        return "clingcon";
    }

    [[nodiscard]] char const *version() const noexcept override {
        return CLINGCON_VERSION;
    }

    void register_options(Clingo::ClingoOptions &options) override {
        handle_error(clingcon_register_options(theory_, options.to_c()));
    }

    void validate_options() override {
        handle_error(clingcon_validate_options(theory_));
    }

    bool on_model(Clingo::Model &model) override {
        handle_error(clingcon_on_model(theory_, model.to_c()));
        return true;
    }

    void print_model(Clingo::Model const &model, std::function<void()> default_printer) noexcept override {
        static_cast<void>(default_printer);
        try {
            // print model
            bool comma = false;
            auto symbols = model.symbols(Clingo::ShowType::Shown);
            symvec_.assign(symbols.begin(), symbols.end());
            std::sort(symbols.begin(), symbols.end());
            for (auto &sym : symbols) {
                std::cout << (comma ? " " : "") <<  sym;
                comma = true;
            }
            std::cout << "\n";

            // print assignment
            comma = false;
            symbols = model.symbols(Clingo::ShowType::Theory);
            symvec_.assign(symbols.begin(), symbols.end());
            std::sort(symbols.begin(), symbols.end());
            char const *cost = nullptr;
            std::cout << "Assignment:\n";
            for (auto &sym : symbols) {
                if (sym.match("__csp", 2)) {
                    auto arguments = sym.arguments();
                    std::cout << (comma ? " " : "") <<  arguments[0] << "=" << arguments[1];
                    comma = true;
                }
                else if (sym.match("__csp_cost", 1)) {
                    auto arguments = sym.arguments();
                    if (arguments[0].type() == Clingo::SymbolType::String) {
                        cost = arguments[0].string();
                    }
                }
            }
            std::cout << "\n";

            // print cost
            if (cost != nullptr) {
                std::cout << "Cost: " << cost << "\n";
            }

            std::cerr.flush();
        }
        catch(...) {
            std::terminate();
        }

    }
    void on_statistics(Clingo::UserStatistics step, Clingo::UserStatistics accu) override {
        handle_error(clingcon_on_statistics(theory_, step.to_c(), accu.to_c()));
    }

    void main(Clingo::Control &control, Clingo::StringSpan files) override { // NOLINT(bugprone-exception-escape)
        handle_error(clingcon_register(theory_, control.to_c()));

        Clingo::AST::with_builder(control, [&](Clingo::AST::ProgramBuilder &builder) {
            Rewriter rewriter{theory_, builder.to_c()};
            rewriter.rewrite(control, files);
        });
        control.ground({{"base", {}}});
        handle_error(clingcon_prepare(theory_, control.to_c()));

#ifdef CLINGCON_PROFILE
        ProfilerStart("clingcon.solve.prof");
#endif
        control.solve(Clingo::SymbolicLiteralSpan{}, this, false, false).get();
#ifdef CLINGCON_PROFILE
        ProfilerStop();
#endif
    }

private:
    clingcon_theory_t *theory_{nullptr};
    std::vector<Clingo::Symbol> symvec_;
};


int main(int argc, char *argv[]) {
    ClingconApp app;
    return Clingo::clingo_main(app, {argv + 1, static_cast<size_t>(argc - 1)});
}
