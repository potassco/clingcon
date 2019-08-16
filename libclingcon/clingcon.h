// {{{ MIT License
//
// // Copyright 2019 Roland Kaminski, Philipp Wanko, Max Ostrowski
//
// // Permission is hereby granted, free of charge, to any person obtaining a copy
// // of this software and associated documentation files (the "Software"), to
// // deal in the Software without restriction, including without limitation the
// // rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// // sell copies of the Software, and to permit persons to whom the Software is
// // furnished to do so, subject to the following conditions:
//
// // The above copyright notice and this permission notice shall be included in
// // all copies or substantial portions of the Software.
//
// // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// // FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// // IN THE SOFTWARE.
//
// // }}}

#ifndef CLINGCON_H
#define CLINGCON_H

//! Major version number.
#define CLINGCON_VERSION_MAJOR 4
//! Minor version number.
#define CLINGCON_VERSION_MINOR 0
//! Revision number.
#define CLINGCON_VERSION_REVISION 0
//! String representation of version.
#define CLINGCON_VERSION "4.0.0"

#ifdef __cplusplus
extern "C" {
#endif

#if defined _WIN32 || defined __CYGWIN__
#   define CLINGCON_WIN
#endif
#ifdef CLINGCON_NO_VISIBILITY
#   define CLINGCON_VISIBILITY_DEFAULT
#   define CLINGCON_VISIBILITY_PRIVATE
#else
#   ifdef CLINGCON_WIN
#       ifdef CLINGCON_BUILD_LIBRARY
#           define CLINGCON_VISIBILITY_DEFAULT __declspec (dllexport)
#       else
#           define CLINGCON_VISIBILITY_DEFAULT __declspec (dllimport)
#       endif
#       define CLINGCON_VISIBILITY_PRIVATE
#   else
#       if __GNUC__ >= 4
#           define CLINGCON_VISIBILITY_DEFAULT  __attribute__ ((visibility ("default")))
#           define CLINGCON_VISIBILITY_PRIVATE __attribute__ ((visibility ("hidden")))
#       else
#           define CLINGCON_VISIBILITY_DEFAULT
#           define CLINGCON_VISIBILITY_PRIVATE
#       endif
#   endif
#endif

#include <clingo.h>

enum clingcon_value_type {
    clingcon_value_type_int = 0,
    clingcon_value_type_symbol = 2
};
typedef int clingcon_value_type_t;

typedef struct clingcon_value {
    clingcon_value_type type;
    union {
        int int_number;
        clingo_symbol_t symbol;
    };
} clingcon_value_t;

typedef struct clingcon_propagator clingcon_propagator_t;

//! creates the propagator
CLINGCON_VISIBILITY_DEFAULT bool clingcon_create_propagator(clingcon_propagator_t **propagator);

//! registers the propagator with the control
CLINGCON_VISIBILITY_DEFAULT bool clingcon_register_propagator(clingcon_propagator_t *propagator, clingo_control_t* control);

//! destroys the propagator, currently no way to unregister a propagator
CLINGCON_VISIBILITY_DEFAULT bool clingcon_destroy_propagator(clingcon_propagator_t *propagator);

//! configure propagator manually (without using clingo's options facility)
//! Note that the propagator has to be configured before registering it and cannot be reconfigured.
CLINGCON_VISIBILITY_DEFAULT bool clingcon_configure_propagator(clingcon_propagator_t *prop, char const *key, char const *value);

//! add options for your theory
CLINGCON_VISIBILITY_DEFAULT bool clingcon_register_options(clingcon_propagator_t *propagator, clingo_options_t* options);

//! validate options for your theory
CLINGCON_VISIBILITY_DEFAULT bool clingcon_validate_options(clingcon_propagator_t *propagator);

//! do pre ground work in the propagator
CLINGCON_VISIBILITY_DEFAULT bool clingcon_pre_ground(clingcon_propagator_t *propagator, clingo_control_t* control);

//! do post ground work in the propagator
CLINGCON_VISIBILITY_DEFAULT bool clingcon_post_ground(clingcon_propagator_t *propagator, clingo_control_t* control);

//! callback on every model
CLINGCON_VISIBILITY_DEFAULT bool clingcon_on_model(clingcon_propagator_t *propagator, clingo_model_t* model);

//! obtain a symbol index which can be used to get the value of a symbol
//! returns true if the symbol exists
//! does not throw
CLINGCON_VISIBILITY_DEFAULT bool clingcon_lookup_symbol(clingcon_propagator_t *propagator, clingo_symbol_t symbol, size_t *index);

//! obtain the symbol at the given index
//! does not throw
CLINGCON_VISIBILITY_DEFAULT clingo_symbol_t clingcon_get_symbol(clingcon_propagator_t *propagator, size_t index);

//! initialize index so that it can be used with clingcon_assignment_next
//! does not throw
CLINGCON_VISIBILITY_DEFAULT void clingcon_assignment_begin(clingcon_propagator_t *propagator, uint32_t thread_id, size_t *index);

//! move to the next index that has a value
//! returns true if the updated index is valid
//! does not throw
CLINGCON_VISIBILITY_DEFAULT bool clingcon_assignment_next(clingcon_propagator_t *propagator, uint32_t thread_id, size_t *index);

//! check if the symbol at the given index has a value
//! does not throw
CLINGCON_VISIBILITY_DEFAULT bool clingcon_assignment_has_value(clingcon_propagator_t *propagator, uint32_t thread_id, size_t index);

//! get the symbol and it's value at the given index
//! does not throw
CLINGCON_VISIBILITY_DEFAULT void clingcon_assignment_get_value(clingcon_propagator_t *propagator, uint32_t thread_id, size_t index, clingcon_value_t *value);

//! callback on statistic updates
/// please add a subkey with the name of your propagator
CLINGCON_VISIBILITY_DEFAULT bool clingcon_on_statistics(clingcon_propagator_t *propagator, clingo_statistics_t* step, clingo_statistics_t* accu);

#ifdef __cplusplus
}
#endif

#endif
