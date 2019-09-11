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

#include <test/testapp.h>
#include <clingcon/constraint.h>
#include <clingcon/solver.h>
#include <clingcon/configs.h>
#include <test/testapp.h>
#include <iostream>

using namespace clingcon;



    bool constrainttest1(Clingo::Control &ctl)
    {
        Stats stats;
        Grounder s(ctl, stats);
        VariableCreator vc(s, translateConfig);
        Variable v0 = vc.createVariable(Domain(1,10));
        Variable v1 = vc.createVariable(Domain(1,10));
        Variable v15 = vc.createVariable(Domain(1,10));
        Variable v2 = vc.createVariable(Domain(1,10));

        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(View(v0,1));
        l.add(View(v0,3));
        l.add(View(v0,-7));
        l.add(View(v1,17));
        l.add(View(v1,-4));
        l.add(View(v1,1));
        l.add(View(v15,17));
        l.add(View(v15,-18));
        l.add(View(v15,1));
        l.add(View(v2,1));
        l.add(View(v2,-1));
        l.addRhs(-45);
        //std::cout << std::endl << l << std::endl;
        l.normalize();

        REQUIRE(l.getViews()[0].v==v0);
        REQUIRE(l.getViews()[1].v==v1);
        //l.getViews()[2].v=v2;

        REQUIRE(l.getViews()[0].a==-3);
        REQUIRE(l.getViews()[1].a==14);
        //l.getViews()[0].a=0
        REQUIRE(l.getViews().size()==2);
        return true;
    }

    REGISTER(constrainttest1);

