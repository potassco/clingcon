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
#include <order/constraint.h>
#include <test/mysolver.h>
#include <order/configs.h>
#include <iostream>

using namespace order;


    TEST_CASE("TestAddition", "[split]")
    {
        Config conf = lazySolveConfigProp4;
        MySolver s;
        VariableCreator vc(s, translateConfig);
        Variable v0 = vc.createVariable(Domain(1,1));
        v0 += 0;
        Variable v1 = vc.createVariable(Domain(1,100));
        Variable v2 = vc.createVariable(Domain(1,200));
        Variable v3 = vc.createVariable(Domain(-100,100));
        Variable v4 = vc.createVariable(Domain(1,1));
        Variable v5 = vc.createVariable(Domain(-1,50));
        Variable v6 = vc.createVariable(Domain(1,10));

        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(View(v1,1));
        l.add(View(v2,5));
        l.add(View(v3,-1));
        l.add(View(v4,17));
        l.add(View(v1,-4));
        l.add(View(v3,1));
        l.add(View(v5,1));
        l.add(View(v6,1000));
        l.addRhs(-45);
        //std::cout << std::endl << l << std::endl;
        l.normalize();
        //std::cout << l << std::endl;
        std::stringstream ss;
        ss << l;
        REQUIRE(ss.str()=="v1 * -3	+	v2 * 5	+	v4 * 17	+	v5 * 1	+	v6 * 1000	<= -45");

        l.sort(vc,conf);
        ss.str("");
        ss << l;
        //std::cout << ss.str() << std::endl;
        REQUIRE(ss.str()=="v4 * 17	+	v6 * 1000	+	v5 * 1	+	v1 * -3	+	v2 * 5	<= -45");
        //conf.break_symmetries
        Config c2 = translateConfig;
        c2.break_symmetries=false;
        auto v = l.split(s, vc,c2, TruthValue::TRUE);
        std::vector<std::string> results;
        results.push_back("v5 * 1	+	v7 * 5	+	v8 * 1	<= -45");
        results.push_back("v4 * 17	+	v1 * -3	+	v8 * -1	== 0");
        results.push_back("v6 * 200	+	v2 * 1	+	v7 * -1	== 0");
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            ss.str("");
            ss << v[i];
            //std::cout << v[i] << std::endl;
            REQUIRE(ss.str()==results[i]);
        }
    }

    TEST_CASE("Linear Constraint normalize", "[lc]")
    {
        MySolver s;
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


