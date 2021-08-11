// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#ifndef _GRINGO_HASHABLE_HH
#define _GRINGO_HASHABLE_HH

#include <ostream>
#include <typeinfo>
#include <functional>
#include <gringo/utility.hh>

#define GRINGO_HASH(T) namespace std { template <> struct hash<T> : hash<Gringo::Hashable> { }; }
#define GRINGO_CALL_HASH(T) namespace std { template <> struct hash<T> { size_t operator()(T const &x) const { return x.hash(); } }; }

namespace Gringo {

// {{{ declaration of Hashable

class Hashable {
public:
    virtual size_t hash() const = 0;
    virtual ~Hashable() { }
};

// }}}
// {{{ definition of call_hash

template <class T>
struct call_hash {
    size_t operator()(T const &x) const { return x.hash(); }
};

// }}}

} // namespace Gringo

namespace std {

// {{{ definition of hash<Gringo::Hashable>

template <>
struct hash<Gringo::Hashable> {
    size_t operator()(Gringo::Hashable const &x) const { return x.hash(); }
};

// }}}

} // namespace std

#endif // _GRINGO_HASHABLE_HH

