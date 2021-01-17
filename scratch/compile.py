from cffi import FFI
ffibuilder = FFI()

ffibuilder.cdef("""

typedef uint64_t clingo_symbol_t;
typedef struct clingo_ast_statement clingo_ast_statement_t;
typedef struct clingo_ast clingo_ast_t;
typedef struct clingo_control clingo_control_t;
typedef struct clingo_options clingo_options_t;
typedef struct clingo_model clingo_model_t;
typedef struct clingo_statistics clingo_statistics_t;

enum clingcon_value_type {
    clingcon_value_type_int = 0,
    clingcon_value_type_double = 1,
    clingcon_value_type_symbol = 2
};
typedef int clingcon_value_type_t;

typedef struct clingcon_value {
    clingcon_value_type_t type;
    union {
        int int_number;
        double double_number;
        clingo_symbol_t symbol;
    };
} clingcon_value_t;

typedef struct clingcon_theory clingcon_theory_t;

typedef bool (*clingcon_rewrite_callback_t)(clingo_ast_statement_t const *statement, void *data);

typedef bool (*clingcon_ast_callback_t)(clingo_ast_t *ast, void *data);

bool clingcon_create(clingcon_theory_t **theory);

bool clingcon_register(clingcon_theory_t *theory, clingo_control_t* control);

bool clingcon_rewrite_statement(clingcon_theory_t *theory, clingo_ast_statement_t const *stm, clingcon_rewrite_callback_t add, void *data);

bool clingcon_rewrite_ast(clingcon_theory_t *theory, clingo_ast_t *ast, clingcon_ast_callback_t add, void *data);

bool clingcon_prepare(clingcon_theory_t *theory, clingo_control_t* control);

bool clingcon_destroy(clingcon_theory_t *theory);

bool clingcon_configure(clingcon_theory_t *theory, char const *key, char const *value);

bool clingcon_register_options(clingcon_theory_t *theory, clingo_options_t* options);

bool clingcon_validate_options(clingcon_theory_t *theory);

bool clingcon_on_model(clingcon_theory_t *theory, clingo_model_t* model);

bool clingcon_lookup_symbol(clingcon_theory_t *theory, clingo_symbol_t symbol, size_t *index);

clingo_symbol_t clingcon_get_symbol(clingcon_theory_t *theory, size_t index);

void clingcon_assignment_begin(clingcon_theory_t *theory, uint32_t thread_id, size_t *index);

bool clingcon_assignment_next(clingcon_theory_t *theory, uint32_t thread_id, size_t *index);

bool clingcon_assignment_has_value(clingcon_theory_t *theory, uint32_t thread_id, size_t index);

void clingcon_assignment_get_value(clingcon_theory_t *theory, uint32_t thread_id, size_t index, clingcon_value_t *value);

bool clingcon_on_statistics(clingcon_theory_t *theory, clingo_statistics_t* step, clingo_statistics_t* accu);

extern "Python" bool pyclingcon_rewrite(clingo_ast_t *ast, void *data);
""")

pip_path = '/home/kaminski/.local//lib/python3.8/site-packages/clingo'
lib_path = '/home/kaminski/git/potassco/clingcon/build/pip/bin'
inc_path = '/home/kaminski/git/potassco/clingcon/libclingcon'


ffibuilder.set_source("_clingcon", """\
#include "clingcon.h"
""", libraries=['clingcon'], \
     include_dirs=[pip_path, inc_path],
     library_dirs=[lib_path],
     extra_link_args=[f'-Wl,-rpath={lib_path}'])

ffibuilder.compile(verbose=True)
