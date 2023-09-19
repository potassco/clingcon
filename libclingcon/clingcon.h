// {{{ MIT License
//
// Copyright 2018 Roland Kaminski
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

#ifndef CLINGCON_H
#define CLINGCON_H

#include <clingo.h>

//! @file clingcon.h
//! Functions implementing the tefoli interface to register a propagator with a
//! clingo application or use it with a standalone control object.
//!
//! @author Roland Kaminski

//! Major version number.
#define CLINGCON_VERSION_MAJOR 5
//! Minor version number.
#define CLINGCON_VERSION_MINOR 2
//! Revision number.
#define CLINGCON_VERSION_REVISION 1
//! String representation of version.
#define CLINGCON_VERSION "5.2.1"

#ifdef __cplusplus
extern "C" {
#endif

#if defined _WIN32 || defined __CYGWIN__
#define CLINGCON_WIN
#endif
#ifdef CLINGCON_NO_VISIBILITY
#define CLINGCON_VISIBILITY_DEFAULT
#define CLINGCON_VISIBILITY_PRIVATE
#else
#ifdef CLINGCON_WIN
#ifdef CLINGCON_BUILD_LIBRARY
#define CLINGCON_VISIBILITY_DEFAULT __declspec(dllexport)
#else
#define CLINGCON_VISIBILITY_DEFAULT __declspec(dllimport)
#endif
#define CLINGCON_VISIBILITY_PRIVATE
#else
#if __GNUC__ >= 4
#define CLINGCON_VISIBILITY_DEFAULT __attribute__((visibility("default")))
#define CLINGCON_VISIBILITY_PRIVATE __attribute__((visibility("hidden")))
#else
#define CLINGCON_VISIBILITY_DEFAULT
#define CLINGCON_VISIBILITY_PRIVATE
#endif
#endif
#endif

// NOLINTBEGIN(modernize-use-using,modernize-use-trailing-return-type)

//! Value types that can be returned by a theory.
enum clingcon_value_type {
    clingcon_value_type_int = 0,
    clingcon_value_type_double = 1,
    clingcon_value_type_symbol = 2
};
//! Corresponding type to ::clingcon_value_type.
typedef int clingcon_value_type_t;

//! Struct to store values that can be returned by a theory.
typedef struct clingcon_value {
    clingcon_value_type_t type;
    union {
        int int_number;
        double double_number;
        clingo_symbol_t symbol;
    };
} clingcon_value_t;

//! The clingcon theory.
typedef struct clingcon_theory clingcon_theory_t;

//! Callback to rewrite statements (see ::clingcon_rewrite_ast).
typedef bool (*clingcon_ast_callback_t)(clingo_ast_t *ast, void *data);

//! Return the version of the theory.
CLINGCON_VISIBILITY_DEFAULT void clingcon_version(int *major, int *minor, int *patch);

//! Creates the theory.
CLINGCON_VISIBILITY_DEFAULT bool clingcon_create(clingcon_theory_t **theory);

//! Register the theory with a control object.
CLINGCON_VISIBILITY_DEFAULT bool clingcon_register(clingcon_theory_t *theory, clingo_control_t *control);

//! Rewrite asts before adding them via the given callback.
CLINGCON_VISIBILITY_DEFAULT bool clingcon_rewrite_ast(clingcon_theory_t *theory, clingo_ast_t *ast,
                                                      clingcon_ast_callback_t add, void *data);

//! Prepare the theory between grounding and solving.
CLINGCON_VISIBILITY_DEFAULT bool clingcon_prepare(clingcon_theory_t *theory, clingo_control_t *control);

//! Destroy the theory.
//!
//! Currently no way to unregister a theory.
CLINGCON_VISIBILITY_DEFAULT bool clingcon_destroy(clingcon_theory_t *theory);

//! Configure theory manually (without using clingo's options facility).
//!
//! Note that the theory has to be configured before registering it and cannot
//! be reconfigured.
CLINGCON_VISIBILITY_DEFAULT bool clingcon_configure(clingcon_theory_t *theory, char const *key, char const *value);

//! Register options of the theory.
CLINGCON_VISIBILITY_DEFAULT bool clingcon_register_options(clingcon_theory_t *theory, clingo_options_t *options);

//! Validate options of the theory.
CLINGCON_VISIBILITY_DEFAULT bool clingcon_validate_options(clingcon_theory_t *theory);

//! Callback for models.
CLINGCON_VISIBILITY_DEFAULT bool clingcon_on_model(clingcon_theory_t *theory, clingo_model_t *model);

//! Obtain a symbol index which can be used to get the value of a symbol.
//!
//! Returns true if the symbol exists.
//! Does not throw.
CLINGCON_VISIBILITY_DEFAULT bool clingcon_lookup_symbol(clingcon_theory_t *theory, clingo_symbol_t symbol,
                                                        size_t *index);

//! Obtain the symbol at the given index.
//!
//! Does not throw.
CLINGCON_VISIBILITY_DEFAULT clingo_symbol_t clingcon_get_symbol(clingcon_theory_t *theory, size_t index);

//! Initialize index so that it can be used with clingcon_assignment_next.
//!
//! Does not throw.
CLINGCON_VISIBILITY_DEFAULT void clingcon_assignment_begin(clingcon_theory_t *theory, uint32_t thread_id,
                                                           size_t *index);

//! Move to the next index that has a value.
//!
//! Returns true if the updated index is valid.
//! Does not throw.
CLINGCON_VISIBILITY_DEFAULT bool clingcon_assignment_next(clingcon_theory_t *theory, uint32_t thread_id, size_t *index);

//! Check if the symbol at the given index has a value.
//!
//! Does not throw.
CLINGCON_VISIBILITY_DEFAULT bool clingcon_assignment_has_value(clingcon_theory_t *theory, uint32_t thread_id,
                                                               size_t index);

//! Get the symbol and it's value at the given index.
//!
//! Does not throw.
CLINGCON_VISIBILITY_DEFAULT void clingcon_assignment_get_value(clingcon_theory_t *theory, uint32_t thread_id,
                                                               size_t index, clingcon_value_t *value);

//! Callback for statistic updates.
//!
//! Best add statistics under a subkey with the name of your theory.
CLINGCON_VISIBILITY_DEFAULT bool clingcon_on_statistics(clingcon_theory_t *theory, clingo_statistics_t *step,
                                                        clingo_statistics_t *accu);

// NOLINTEND(modernize-use-using,modernize-use-trailing-return-type)

#ifdef __cplusplus
}
#endif

#endif
