#include <clingo.hh>
#include <clingcon.h>
#include <fstream>

#define CLINGO_CALL(x) Clingo::Detail::handle_error(x)


class Rewriter {
public:
    Rewriter(clingcon_theory_t *theory, clingo_program_builder_t *builder)
    : theory_{theory}
    , builder_{builder} {
    }

    void load(char const *file) {
        std::string program;
        if (strcmp(file, "-") == 0) {
            program.assign(std::istreambuf_iterator<char>{std::cin}, std::istreambuf_iterator<char>{});
        }
        else {
            std::ifstream ifs{file};
            program.assign(std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{});
        }

        // TODO: parsing from file would be nice
        CLINGO_CALL(clingo_parse_program(program.c_str(), rewrite_, this, nullptr, nullptr, 0));
    }

private:
    static bool add_(clingo_ast_statement_t const *stm, void *data) {
        Rewriter *self = static_cast<Rewriter*>(data);
        return clingo_program_builder_add(self->builder_, stm);
    }

    static bool rewrite_(clingo_ast_statement_t const *stm, void *data) {
        Rewriter *self = static_cast<Rewriter*>(data);
        return clingcon_rewrite_statement(self->theory_, stm, add_, self);
    }

private:
    clingcon_theory_t *theory_;
    clingo_program_builder_t *builder_;
};


class ClingconApp : public Clingo::Application, private Clingo::SolveEventHandler {
public:
    ClingconApp()
    : theory_{nullptr} {
        CLINGO_CALL(clingcon_create(&theory_));
    }

    ~ClingconApp() {
        if (theory_) {
            clingcon_destroy(theory_);
        }
    }

    char const *program_name() const noexcept override {
        return "clingcon";
    }

    char const *version() const noexcept override {
        return CLINGCON_VERSION;
    }

    void register_options(Clingo::ClingoOptions &options) override {
        CLINGO_CALL(clingcon_register_options(theory_, options.to_c()));
    }

    void validate_options() override {
        CLINGO_CALL(clingcon_validate_options(theory_));
    }

    bool on_model(Clingo::Model &model) override {
        CLINGO_CALL(clingcon_on_model(theory_, model.to_c()));
        return true;
    }

    void on_statistics(Clingo::UserStatistics step, Clingo::UserStatistics accu) override {
        CLINGO_CALL(clingcon_on_statistics(theory_, step.to_c(), accu.to_c()));
    }

    void main(Clingo::Control &control, Clingo::StringSpan files) override {
        CLINGO_CALL(clingcon_register(theory_, control.to_c()));

        parse_(control, files);
        control.ground({{"base", {}}});
        CLINGO_CALL(clingcon_prepare(theory_, control.to_c()));
        control.solve(Clingo::SymbolicLiteralSpan{}, this, false, false).get();
    }

private:
    void parse_(Clingo::Control &control, Clingo::StringSpan files) {
        control.with_builder([&](Clingo::ProgramBuilder &builder) {
            Rewriter rewriter{theory_, builder.to_c()};
            for (auto &file : files) {
                rewriter.load(file);
            }
            if (files.empty()) {
                rewriter.load("-");
            }
        });
    }

private:
    clingcon_theory_t *theory_;
};


int main(int argc, char *argv[]) {
    ClingconApp app;
    return Clingo::clingo_main(app, {argv + 1, static_cast<size_t>(argc - 1)});
}
