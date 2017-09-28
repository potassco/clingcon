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

#include "catch.hpp"
#include "test/mysolver.h"
#include "order/normalizer.h"
#include "clasp/clasp_facade.h"
#include "order/equality.h"
#include "order/configs.h"
#include <iostream>

using namespace order;

    TEST_CASE("EqualityProcessor1", "1")
    {
        MySolver s;
        Normalizer n(s,lazySolveConfigProp4);
        EqualityProcessor p(s,n.getVariableCreator());

        std::vector<ReifiedLinearConstraint> lc;
        View a = n.createView();
        View b = n.createView();
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a);
            l.add(b*-1);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }
        REQUIRE(p.process(lc));
        REQUIRE(p.getEqualities(a.v)==p.getEqualities(b.v));
        REQUIRE(lc.size()==0);
    }

    TEST_CASE("EqualityProcessor2", "2")
    {
        MySolver s;
        Normalizer n(s,lazySolveConfigProp4);
        EqualityProcessor p(s,n.getVariableCreator());

        std::vector<ReifiedLinearConstraint> lc;
        View a = n.createView();

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }
        REQUIRE(p.process(lc));
        REQUIRE(lc.size()==0);
    }


    TEST_CASE("EqualityProcessor3", "3")
    {
        MySolver s;
        Normalizer n(s,lazySolveConfigProp4);
        EqualityProcessor p(s,n.getVariableCreator());

        std::vector<ReifiedLinearConstraint> lc;
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.addRhs(3);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }
        REQUIRE(!p.process(lc));
    }

    TEST_CASE("EqualityProcessor4", "4")
    {
        MySolver s;
        Normalizer n(s,lazySolveConfigProp4);
        EqualityProcessor p(s,n.getVariableCreator());

        View a = n.createView();
        View b = n.createView();

        std::vector<ReifiedLinearConstraint> lc;
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a);
            l.add(b*-1);
            l.addRhs(3);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }
        REQUIRE(p.process(lc));

        REQUIRE(p.getEqualities(a.v)==p.getEqualities(b.v));
        REQUIRE(p.getEqualities(a.v)->top()==a.v);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(b.v).constant==-3);
    }


    TEST_CASE("EqualityProcessor5", "5")
    {
        MySolver s;
        Normalizer n(s,lazySolveConfigProp4);
        EqualityProcessor p(s,n.getVariableCreator());

        View a = n.createView();
        View b = n.createView();
        View c = n.createView();
        View d = n.createView();

        std::vector<ReifiedLinearConstraint> lc;
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a);
            l.add(b);
            l.add(c);
            l.add(d*-1);
            l.addRhs(-3);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a);
            l.add(b);
            l.add(c*-1);
            l.addRhs(-2);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a);
            l.add(b*-1);
            l.addRhs(1);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        REQUIRE(p.process(lc));

        REQUIRE(p.getEqualities(a.v)==p.getEqualities(b.v));
        REQUIRE(p.getEqualities(b.v)==p.getEqualities(c.v));
        REQUIRE(p.getEqualities(c.v)==p.getEqualities(d.v));
        REQUIRE(p.getEqualities(a.v)->top()==a.v);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(b.v).firstCoef==1);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(b.v).secondCoef==1);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(b.v).constant==-1);

        REQUIRE(p.getEqualities(a.v)->getConstraints().at(c.v).firstCoef==1);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(c.v).secondCoef==2);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(c.v).constant==1);

        REQUIRE(p.getEqualities(a.v)->getConstraints().at(d.v).firstCoef==1);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(d.v).secondCoef==4);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(d.v).constant==3);
    }


    TEST_CASE("EqualityProcessor6", "6")
    {
        /// 2 times a = b + 3
        MySolver s;
        Normalizer n(s,lazySolveConfigProp4);
        EqualityProcessor p(s,n.getVariableCreator());

        View a = n.createView();
        View b = n.createView();

        std::vector<ReifiedLinearConstraint> lc;
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a);
            l.add(b*-1);
            l.addRhs(3);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a);
            l.add(b*-1);
            l.addRhs(3);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }
        REQUIRE(p.process(lc));

        REQUIRE(p.getEqualities(a.v)==p.getEqualities(b.v));
        REQUIRE(p.getEqualities(a.v)->top()==a.v);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(b.v).constant==-3);
    }

    TEST_CASE("EqualityProcessor7", "7")
    {
        // a = b + 3
        // a = b + 4
        // fail
        MySolver s;
        Normalizer n(s,lazySolveConfigProp4);
        EqualityProcessor p(s,n.getVariableCreator());

        View a = n.createView();
        View b = n.createView();

        std::vector<ReifiedLinearConstraint> lc;
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a);
            l.add(b*-1);
            l.addRhs(3);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a);
            l.add(b*-1);
            l.addRhs(4);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }
        REQUIRE(!p.process(lc));
    }


    TEST_CASE("EqualityProcessor8", "8")
    {
        // 3*a = 2*b
        // 3*a = 4*b
        MySolver s;
        Normalizer n(s,lazySolveConfigProp4);
        EqualityProcessor p(s,n.getVariableCreator());

        View a = n.createView();
        View b = n.createView();

        std::vector<ReifiedLinearConstraint> lc;
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a*3);
            l.add(b*-2);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a*3);
            l.add(b*-4);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        REQUIRE(p.process(lc));
        REQUIRE(lc.size()==0);

        REQUIRE(p.isUnary(a.v));
        REQUIRE(p.isUnary(b.v));
        REQUIRE(p.getUnary(a.v)==0);
        REQUIRE(p.getUnary(b.v)==0);
    }

            // 7*a = 18*b

    TEST_CASE("EqualityProcessor9", "9")
    {
        // 3*a = 2*b
        // 3*a = 4*b
        // 7*a = 18*b
        MySolver s;
        Normalizer n(s,lazySolveConfigProp4);
        EqualityProcessor p(s,n.getVariableCreator());

        View a = n.createView();
        View b = n.createView();

        std::vector<ReifiedLinearConstraint> lc;
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a*3);
            l.add(b*-2);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a*3);
            l.add(b*-4);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a*7);
            l.add(b*-18);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        REQUIRE(p.process(lc));
        REQUIRE(lc.size()==0);

        REQUIRE(p.isUnary(a.v));
        REQUIRE(p.isUnary(b.v));
        REQUIRE(p.getUnary(a.v)==0);
        REQUIRE(p.getUnary(b.v)==0);
    }


    TEST_CASE("EqualityProcessor10", "10")
    {
        // a = 2*b
        // b = 2*c

        // d = 2*e
        // e = 2*f

        ///merge both
        // c = 2*d



        /// a = 2b
        /// a = 4c
        /// a = 8d
        /// a = 16e
        /// a = 32f

        MySolver s;
        Normalizer n(s,lazySolveConfigProp4);
        EqualityProcessor p(s,n.getVariableCreator());

        View a = n.createView();
        View b = n.createView();
        View c = n.createView();
        View d = n.createView();
        View e = n.createView();
        View f = n.createView();

        std::vector<ReifiedLinearConstraint> lc;
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a);
            l.add(b*-2);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(b);
            l.add(c*-2);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(d);
            l.add(e*-2);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(e);
            l.add(f*-2);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(c);
            l.add(d*-2);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }


        /// further linear constraints

        View g = n.createView();
        {
        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(a);
        l.add(d*14); // this is 1.75 a's
        l.add(f*-3); /// is is only a fraction of a
        l.add(b); /// this is 0.5a
        l.add(g);
        l.addRhs(0);
        l.normalize();
        lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        REQUIRE(p.process(lc));
        REQUIRE(lc.size()==1);

        REQUIRE(p.getEqualities(a.v)==p.getEqualities(b.v));
        REQUIRE(p.getEqualities(b.v)==p.getEqualities(c.v));
        REQUIRE(p.getEqualities(c.v)==p.getEqualities(d.v));
        REQUIRE(p.getEqualities(d.v)==p.getEqualities(e.v));
        REQUIRE(p.getEqualities(e.v)==p.getEqualities(f.v));
        REQUIRE(p.getEqualities(a.v)->top()==a.v);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(b.v).constant==0);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(c.v).constant==0);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(d.v).constant==0);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(e.v).constant==0);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(f.v).constant==0);

        REQUIRE(p.getEqualities(a.v)->getConstraints().at(b.v).firstCoef==2);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(b.v).secondCoef==1);

        REQUIRE(p.getEqualities(a.v)->getConstraints().at(c.v).firstCoef==4);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(c.v).secondCoef==1);

        REQUIRE(p.getEqualities(a.v)->getConstraints().at(d.v).firstCoef==8);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(d.v).secondCoef==1);

        REQUIRE(p.getEqualities(a.v)->getConstraints().at(e.v).firstCoef==16);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(e.v).secondCoef==1);

        REQUIRE(p.getEqualities(a.v)->getConstraints().at(f.v).firstCoef==32);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(f.v).secondCoef==1);

        for (auto& i : lc)
            p.substitute(i.l);

        const LinearConstraint& l = lc.back().l;
        REQUIRE(l.getViews()[0].v==a);
        REQUIRE(l.getViews()[0].a==101);
        REQUIRE(l.getViews()[1].v==g);
        REQUIRE(l.getViews()[1].a==32);
        REQUIRE(l.getViews().size()==2);

        ReifiedAllDistinct rd({a*3,b+7,c,d,e,f,g},Literal(0,false),Direction::EQ);
        p.substitute(rd);
    }


    TEST_CASE("EqualityProcessor11", "11")
    {
        // 3*a = 2*b
        // 8*c = 4*b +20
        // b = 15
        // d = 2*c+13
        MySolver s;
        Normalizer n(s,lazySolveConfigProp4);
        EqualityProcessor p(s,n.getVariableCreator());

        View a = n.createView();
        View b = n.createView();
        View c = n.createView();
        View d = n.createView();

        std::vector<ReifiedLinearConstraint> lc;
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a*3);
            l.add(b*-2);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(c*8);
            l.add(b*-4);
            l.addRhs(20);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }


        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(b*1);
            l.addRhs(15);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(View(c.v,2,13));
            l.add(d*-1);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        REQUIRE(p.process(lc));
        REQUIRE(lc.size()==0);

        REQUIRE(p.isUnary(a.v));
        REQUIRE(p.isUnary(b.v));
        REQUIRE(p.isUnary(c.v));
        REQUIRE(p.isUnary(d.v));

        REQUIRE(p.getUnary(a.v)==10);
        REQUIRE(p.getUnary(b.v)==15);
        REQUIRE(p.getUnary(c.v)==10);
        REQUIRE(p.getUnary(d.v)==33);
    }


    TEST_CASE("EqualityProcessor12", "12")
    {
        // a = 2*b
        // b = 2*c
        // c = 2*d
        // d = 2*e
        // e = 2*f

        ///merge both
        // a +14d -3f +b <= -g



        /// a = 2b
        /// a = 4c
        /// a = 8d
        /// a = 16e
        /// a = 32f

        MySolver s;
        Normalizer n(s,lazySolveConfigProp4);
        EqualityProcessor p(s,n.getVariableCreator());

        View a = n.createView(Domain(-4096,4096));
        View b = n.createView(Domain(-4096,4096));
        View c = n.createView(Domain(-4096,4096));
        View d = n.createView(Domain(-4096,4096));
        View e = n.createView(Domain(-4096,4096));
        View f = n.createView(Domain(-4096,4096));
        View g = n.createView(Domain(-4096,4096));

        std::vector<ReifiedLinearConstraint> lc;
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a);
            l.add(b*-2);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(b);
            l.add(c*-2);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(c);
            l.add(d*-2);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(d);
            l.add(e*-2);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(e);
            l.add(f*-2);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(a);
            l.add(d*14);
            l.add(f*-3);
            l.add(b);
            l.add(g);
            l.addRhs(0);
            l.normalize();
            lc.emplace_back(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
        }

        REQUIRE(p.process(lc));
        REQUIRE(lc.size()==1);

        REQUIRE(p.getEqualities(a.v)==p.getEqualities(b.v));
        REQUIRE(p.getEqualities(b.v)==p.getEqualities(c.v));
        REQUIRE(p.getEqualities(c.v)==p.getEqualities(d.v));
        REQUIRE(p.getEqualities(d.v)==p.getEqualities(e.v));
        REQUIRE(p.getEqualities(e.v)==p.getEqualities(f.v));
        REQUIRE(p.getEqualities(a.v)->top()==a.v);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(b.v).constant==0);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(c.v).constant==0);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(d.v).constant==0);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(e.v).constant==0);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(f.v).constant==0);

        REQUIRE(p.getEqualities(a.v)->getConstraints().at(b.v).firstCoef==2);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(b.v).secondCoef==1);

        REQUIRE(p.getEqualities(a.v)->getConstraints().at(c.v).firstCoef==4);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(c.v).secondCoef==1);

        REQUIRE(p.getEqualities(a.v)->getConstraints().at(d.v).firstCoef==8);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(d.v).secondCoef==1);

        REQUIRE(p.getEqualities(a.v)->getConstraints().at(e.v).firstCoef==16);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(e.v).secondCoef==1);

        REQUIRE(p.getEqualities(a.v)->getConstraints().at(f.v).firstCoef==32);
        REQUIRE(p.getEqualities(a.v)->getConstraints().at(f.v).secondCoef==1);

        for (auto& i : lc)
            p.substitute(i.l);

        const LinearConstraint& l = lc.back().l;
        REQUIRE(l.getViews()[0].v==a);
        REQUIRE(l.getViews()[0].a==101);
        REQUIRE(l.getViews()[1].v==g);
        REQUIRE(l.getViews()[1].a==32);
        REQUIRE(l.getViews().size()==2);

        ReifiedAllDistinct rd({a*3,b+7,c,d,e,f,g},Literal(0,false),Direction::EQ);
        p.substitute(rd);
    }





