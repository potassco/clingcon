// {{{ MIT License
//
// Copyright 2020 Roland Kaminski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
// }}}

#include "clingcon.h"
#include <stdexcept>

extern "C" bool clingcon_create(clingcon_theory_t **theory) {
    static_cast<void>(theory);
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_register(clingcon_theory_t *theory, clingo_control_t* control) {
    static_cast<void>(theory);
    static_cast<void>(control);
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_rewrite_statement(clingcon_theory_t *theory, clingo_ast_statement_t const *stm, clingcon_rewrite_callback_t add, void *data) {
    static_cast<void>(theory);
    static_cast<void>(stm);
    static_cast<void>(add);
    static_cast<void>(data);
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_prepare(clingcon_theory_t *theory, clingo_control_t* control) {
    static_cast<void>(theory);
    static_cast<void>(control);
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_destroy(clingcon_theory_t *theory) {
    static_cast<void>(theory);
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_configure(clingcon_theory_t *theory, char const *key, char const *value) {
    static_cast<void>(theory);
    static_cast<void>(key);
    static_cast<void>(value);
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_register_options(clingcon_theory_t *theory, clingo_options_t* options) {
    static_cast<void>(theory);
    static_cast<void>(options);
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_validate_options(clingcon_theory_t *theory) {
    static_cast<void>(theory);
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_on_model(clingcon_theory_t *theory, clingo_model_t* model) {
    static_cast<void>(theory);
    static_cast<void>(model);
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_lookup_symbol(clingcon_theory_t *theory, clingo_symbol_t symbol, size_t *index) {
    static_cast<void>(theory);
    static_cast<void>(symbol);
    *index = 0;
    throw std::runtime_error("implement me!!!");
}

extern "C" clingo_symbol_t clingcon_get_symbol(clingcon_theory_t *theory, size_t index) {
    static_cast<void>(theory);
    static_cast<void>(index);
    throw std::runtime_error("implement me!!!");
}

extern "C" void clingcon_assignment_begin(clingcon_theory_t *theory, uint32_t thread_id, size_t *index) {
    static_cast<void>(theory);
    static_cast<void>(thread_id);
    *index = 0;
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_assignment_next(clingcon_theory_t *theory, uint32_t thread_id, size_t *index) {
    static_cast<void>(theory);
    static_cast<void>(thread_id);
    *index = 0;
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_assignment_has_value(clingcon_theory_t *theory, uint32_t thread_id, size_t index) {
    static_cast<void>(theory);
    static_cast<void>(thread_id);
    static_cast<void>(index);
    throw std::runtime_error("implement me!!!");
}

extern "C" void clingcon_assignment_get_value(clingcon_theory_t *theory, uint32_t thread_id, size_t index, clingcon_value_t *value) {
    static_cast<void>(theory);
    static_cast<void>(thread_id);
    static_cast<void>(index);
    static_cast<void>(value);
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_on_statistics(clingcon_theory_t *theory, clingo_statistics_t* step, clingo_statistics_t* accu) {
    static_cast<void>(theory);
    static_cast<void>(step);
    static_cast<void>(accu);
    throw std::runtime_error("implement me!!!");
}
