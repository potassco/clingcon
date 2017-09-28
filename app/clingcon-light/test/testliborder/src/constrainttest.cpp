// {{{ GPL License

// This file is part of libcsp - a library for handling linear constraints.
// Copyright (C) 2016  Max Ostrowski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#include <catch.hpp>
#include <clingcon/constraint.h>
#include <clingcon/solver.h>
#include <clingcon/configs.h>
#include <test/testapp.h>
#include <iostream>

using namespace clingcon;


    void constrainttest1(Clingo::Control &ctl)
    {
        Grounder s(ctl.backend());
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

    }

    TEST_CASE("Linear Constraint normalize", "[lc]")
    {
            TestApp app(constrainttest1);
            char *argv[] =  {};
            Clingo::clingo_main(app, {argv, 0});
    }


