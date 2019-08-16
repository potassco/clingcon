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
#include <clingcon/config.h>
#include <clingcon/linearpropagator.h>
#include <cstddef>
#include <limits>


namespace clingcon
{

class Translator
{
public:
    Translator(Grounder &s, const Config &conf)
        : s_(s)
        , conf_(conf)
    {
    }

    /// translate the constraint
    /// returns false if addclause fails
    bool doTranslate(VariableCreator &vc, const ReifiedLinearConstraint &l);


private:
    bool doTranslateImplication(VariableCreator &vc, Literal l, const LinearConstraint &c);
    Grounder &s_;
    const Config &conf_;
};

inline bool translate(Grounder &s, VariableCreator &vc, std::vector< ReifiedLinearConstraint > &rl,
                      const Config &conf)
{
    Translator t(s, conf);

    uint64 size = conf.translateConstraints == -1 ? std::numeric_limits< uint64 >::max() :
                                                    conf.translateConstraints;


    unsigned int num = rl.size();
    for (unsigned int i = 0; i < num;)
    {
        if (rl[i].l.productOfDomainsExceptLastLEx(vc, size))
        {
            if (!t.doTranslate(vc, rl[i])) return false;
            std::swap(rl[i], *(rl.begin() + num - 1));
            --num;
        }
        else
            ++i;
    }
    rl.erase(rl.begin() + num, rl.end());
    return true;
}
}
