// {{{ MIT License

// Copyright 2018 Max Ostrowski

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

#pragma once

#include <clingo.hh>

namespace clingcon
{


#define REQUIRE(x)                                                                                 \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            return false;                                                                          \
        }                                                                                          \
    }
#define REGISTER(x) Foo test_##x(x)

typedef bool (*TestCall)(Clingo::Control &ctl);

class Adder
{
private:
    Adder() {}
    std::vector< TestCall > calls;

public:
    void add(TestCall c) { calls.push_back(c); }

    TestCall get(size_t i)
    {
        assert(calls.size() > i);
        return calls[i];
    }

    size_t size() { return calls.size(); }

public:
    static Adder &singleton()
    {
        static Adder foo;
        return foo;
    }
};

class Foo
{
public:
    Foo(TestCall c) { clingcon::Adder::singleton().add(c); }
};

class TestApp : public Clingo::Application
{
public:
    TestApp(size_t num)
        : call(Adder::singleton().get(num))
        , ret_(false)
    {
    }
    bool retValue() const { return ret_; }
    char const *program_name() const noexcept override { return "clingcon"; }
    void main(Clingo::Control &ctl, Clingo::StringSpan) override
    {
        ctl.add("base", {}, R"(
#theory csp {
    linear_term {
    + : 5, unary;
    - : 5, unary;
    * : 4, binary, left;
    + : 3, binary, left;
    - : 3, binary, left
    };
    dom_term {
    + : 5, unary;
    - : 5, unary;
    .. : 1, binary, left;
    * : 4, binary, left;
    + : 3, binary, left;
    - : 3, binary, left
    };
    show_term {
    / : 1, binary, left
    };
    minimize_term {
    + : 5, unary;
    - : 5, unary;
    * : 4, binary, left;
    + : 3, binary, left;
    - : 3, binary, left;
    @ : 0, binary, left
    };

    &dom/0 : dom_term, {=}, linear_term, any;
    &sum/0 : linear_term, {<=,=,>=,<,>,!=}, linear_term, any;
    &show/0 : show_term, directive;
    &distinct/0 : linear_term, any;
    &minimize/0 : minimize_term, directive
}.
)");

        ret_ = call(ctl);
    }

private:
    TestCall call;
    bool ret_;
};


} // namespace clingcon
