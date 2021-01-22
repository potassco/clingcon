from cffi import FFI
import re
import os.path
ffibuilder = FFI()

path = os.path.dirname(os.path.abspath(__file__))

cnt = []
with open(os.path.join(path, '..', 'libclingcon', 'clingcon.h')) as f:
    for line in f:
        if not re.match(r' *(#|//|extern *"C" *{|}$|$)', line):
            cnt.append(re.sub(r'[A-Z_]+_VISIBILITY_DEFAULT ', '', line).strip())
cnt.append('extern "Python" bool pyclingcon_rewrite(clingo_ast_t *ast, void *data);')
code = '\n'.join(cnt)

ffibuilder.cdef(f'''\
typedef uint64_t clingo_symbol_t;
typedef struct clingo_ast_statement clingo_ast_statement_t;
typedef struct clingo_ast clingo_ast_t;
typedef struct clingo_control clingo_control_t;
typedef struct clingo_options clingo_options_t;
typedef struct clingo_model clingo_model_t;
typedef struct clingo_statistics clingo_statistics_t;
{code}
''')

ffibuilder.set_source("_clingcon", """\
#include "clingcon.h"
""")

ffibuilder.emit_c_code('_clingcon.c')
