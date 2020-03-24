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

#ifndef CLINGCON_ASTUTIL_H
#define CLINGCON_ASTUTIL_H

//! @file clingcon/astutil.hh
//! Utility functions to work with ASTs.
//!
//! @author Roland Kaminski

#include <clingo.hh>
#include <set>

//! @file clingcon/astutil.hh
//! Utility functions to work with ASTs.
//!
//! @author Roland Kaminski

namespace Clingcon {

//! Match the given term if it is a function with signature `name/arity`.
[[nodiscard]] bool match(Clingo::AST::TheoryTerm const &term, char const *name, size_t arity);

template <typename V, typename N>
void transform_ast(V&& v, N &node);
template <typename V, typename N>
void visit_ast(V&& v, N const &node);

template <typename N>
void collect_variables(std::set<char const *> &vars, N const &node);
template <typename N>
std::set<char const *> collect_variables(N const &node);

template <typename Seq, typename F>
inline void cross_product(Seq seq, F f);

} // namespace Clingcon

#endif // CLINGCON_ASTUTIL_H

#ifndef CLINGCON_ASTUTIL_IMPL_H
#include "clingcon/astutil_impl.hh"
#endif
