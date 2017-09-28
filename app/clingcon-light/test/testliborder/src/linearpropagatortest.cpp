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
#include "order/linearpropagator.h"
#include "test/mysolver.h"
#include "order/normalizer.h"
#include "order/configs.h"
#include <iostream>

using namespace order;



    TEST_CASE("TestUnique", "[linearPropagator]")
    {
        MySolver s;
        Normalizer n(s, translateConfig);


        {
            View v1 = n.createView(Domain(5,10));
            View v2 = n.createView(Domain(5,10));
            View v3 = n.createView(Domain(5,10));


            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(v1);
            l.add(v2);
            l.add(v3);
            l.addRhs(17);
            //std::cout << std::endl << l << std::endl;

            //REQUIRE(p.propagate_reified(p,l)==Propagator::value::UNKNOWN);
            //p.propagate_true(p,l);
            auto x= s.getNewLiteral(true);
            n.addConstraint(ReifiedLinearConstraint(std::move(l),x,Direction::FWD));
            LinearConstraint l2(LinearConstraint::Relation::LE);
            l2.add(v1);
            l2.add(v2);
            l2.add(v3);
            l2.addRhs(17);
            n.addConstraint(ReifiedLinearConstraint(std::move(l),x,Direction::EQ));

            n.prepare();
            //s.createNewLiterals(n.estimateVariables());


        }
    }

    TEST_CASE("TestPropagation1", "[linearPropagator]")
    {

        MySolver s;
        Normalizer n(s, translateConfig);
        Normalizer n2(s, translateConfig);

        {
            View v1 = n.createView(Domain(5,10));
            View v2 = n.createView(Domain(5,10));
            View v3 = n.createView(Domain(5,10));


            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(v1);
            l.add(v2);
            l.add(v3);
            l.addRhs(17);
            //std::cout << std::endl << l << std::endl;

            //REQUIRE(p.propagate_reified(p,l)==Propagator::value::UNKNOWN);
            //p.propagate_true(p,l);
            n.addConstraint(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
            n.prepare();
            //s.createNewLiterals(n.estimateVariables());

            //p.addImp(n.removeConstraints());
            //p.propagate();

            //std::cout << p << std::endl;

            REQUIRE(n.getVariableCreator().getViewDomain(v1).upper()==7);
            REQUIRE(n.getVariableCreator().getViewDomain(v2).upper()==7);
            REQUIRE(n.getVariableCreator().getViewDomain(v3).upper()==7);



            //REQUIRE(p.propagate_reified(p,l)==Propagator::value::UNKNOWN);

            //auto r = n.getVariableCreator().getViewDomain(v1);
            //n.getVariableCreator().constrainVariable(v1,Restrictor(r.begin()+2,r.end()));
            //r = n.getVariableCreator().getViewDomain(v2);
            //n.getVariableCreator().constrainVariable(v2,Restrictor(r.begin()+2,r.end()));

            //REQUIRE(p.propagate_reified(p,l)==Propagator::value::FALSE);

            //r = n.getVariableCreator().getViewDomain(v2);
            //n.getVariableCreator().constrainVariable(v2,Restrictor(r.begin()+1,r.end()));
            //REQUIRE(n.getVariableCreator().getViewDomain(v2).isEmpty());


            View v11 = n.createView(Domain(5,10));
            View v12 = n.createView(Domain(5,10));
            View v13 = n.createView(Domain(5,10));
            LinearConstraint l2(LinearConstraint::Relation::EQ);
            l2.add(v11);
            l2.add(v12);
            l2.add(v13);
            l2.addRhs(27);
            //std::cout << std::endl << l << std::endl;

            //REQUIRE(p.propagate_reified(p,l)==Propagator::value::UNKNOWN);
            //p.propagate_true(p,l2);
            n.addConstraint(ReifiedLinearConstraint(std::move(l2),s.trueLit(),Direction::EQ));
            n.prepare();
            //s.createNewLiterals(n.estimateVariables());

            REQUIRE(n.getVariableCreator().getViewDomain(v11).upper()==10);
            REQUIRE(n.getVariableCreator().getViewDomain(v12).upper()==10);
            REQUIRE(n.getVariableCreator().getViewDomain(v13).upper()==10);

            REQUIRE(n.getVariableCreator().getViewDomain(v11).lower()==7);
            REQUIRE(n.getVariableCreator().getViewDomain(v12).lower()==7);
            REQUIRE(n.getVariableCreator().getViewDomain(v13).lower()==7);

            //REQUIRE(p.propagate_reified(p,l2)==Propagator::value::UNKNOWN);
            //std::cout << p << std::endl;



            View v111 = n.createView(Domain(5,5));
            View v112 = n.createView(Domain(10,10));
            View v113 = n.createView(Domain(5,5));
            LinearConstraint l3(LinearConstraint::Relation::EQ);
            l3.add(v111);
            l3.add(v112*-1);
            l3.add(v113);
            l3.addRhs(0);
            //std::cout << std::endl << l << std::endl;


            //REQUIRE(p.propagate_reified(p,l3)==Propagator::value::TRUE);


            //Variable v114 = vc.createVariable(Domain(5,7));
            //l3.normalize();
            //l3.add(v114,-4);

            //REQUIRE(p.propagate_reified(p,l3)==Propagator::value::FALSE);

            View v1111 = n.createView(Domain(5,10));
            View v1112 = n.createView(Domain(5,10));
            View v1113 = n.createView(Domain(5,10));
            LinearConstraint l4(LinearConstraint::Relation::LE);
            l4.add(v1111);
            l4.add(v1112);
            l4.add(v1113);
            l4.addRhs(27);
            //std::cout << std::endl << l << std::endl;


            //REQUIRE(p.propagate_reified(p,l)==Propagator::value::UNKNOWN);
            n.addConstraint(ReifiedLinearConstraint(std::move(l4),s.trueLit(),Direction::EQ));
            //p.propagate_false(p,l4);
            //std::cout << p << std::endl;

            //REQUIRE(n.getVariableCreator().getViewDomain(v1111).getUpper()==10);
            //REQUIRE(n.getVariableCreator().getViewDomain(v1112).getUpper()==10);
            //REQUIRE(n.getVariableCreator().getViewDomain(v1113).getUpper()==10);

            //REQUIRE(n.getVariableCreator().getViewDomain(v1111).getLower()()==8);
            //REQUIRE(n.getVariableCreator().getViewDomain(v1112).getLower()()==8);
            //REQUIRE(n.getVariableCreator().getViewDomain(v1113).getLower()()==8);

        }

        // p2 and p2
        {
            View v1 = n2.createView(Domain(5,10));
            View v2 = n2.createView(Domain(5,10));
            View v3 = n2.createView(Domain(5,10));


            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(v1*-1);
            l.add(v2*-1);
            l.add(v3*-1);
            l.addRhs(-17-1);
            //std::cout << std::endl << l << std::endl;


            //REQUIRE(p2propagate_reified(p2,l)==Propagator::value::UNKNOWN);
            //p.propagate_true(p2,l);
            n2.addConstraint(ReifiedLinearConstraint(std::move(l),s.falseLit(),Direction::EQ));
            n2.prepare();
            //s.createNewLiterals(n2.estimateVariables());

            //std::cout << p2 << std::endl;

            REQUIRE(n2.getVariableCreator().getViewDomain(v1).upper()==7);
            REQUIRE(n2.getVariableCreator().getViewDomain(v2).upper()==7);
            REQUIRE(n2.getVariableCreator().getViewDomain(v3).upper()==7);



            //REQUIRE(p2.propagate_reified(p2,l)==Propagator::value::UNKNOWN);

            //auto r = n2.getVariableCreator().getViewDomain(v1);
            //n2.getVariableCreator().constrainVariable(v1,Restrictor(r.begin()+2,r.end()));
            //r = n2.getVariableCreator().getViewDomain(v2);
            //n2.getVariableCreator().constrainVariable(v2,Restrictor(r.begin()+2,r.end()));

            //REQUIRE(p2.propagate_reified(p2,l)==Propagator::value::FALSE);

            //r = n2.getVariableCreator().getViewDomain(v2);
            //n2.getVariableCreator().constrainVariable(v2,Restrictor(r.begin()+1,r.end()));
            //REQUIRE(n2.getVariableCreator().getViewDomain(v2).isEmpty());


            View v11 = n2.createView(Domain(5,10));
            View v12 = n2.createView(Domain(5,10));
            View v13 = n2.createView(Domain(5,10));
            LinearConstraint l2(LinearConstraint::Relation::EQ);
            l2.add(v11);
            l2.add(v12);
            l2.add(v13);
            l2.addRhs(27);
            //std::cout << std::endl << l << std::endl;


            //REQUIRE(p2.propagate_reified(p2,l)==Propagator::value::UNKNOWN);
            //p2.propagate_true(p2,l2);
            n2.addConstraint(ReifiedLinearConstraint(std::move(l2),s.trueLit(),Direction::EQ));
            n2.prepare();
            //s.createNewLiterals(n2.estimateVariables());

            REQUIRE(n2.getVariableCreator().getViewDomain(v11).upper()==10);
            REQUIRE(n2.getVariableCreator().getViewDomain(v12).upper()==10);
            REQUIRE(n2.getVariableCreator().getViewDomain(v13).upper()==10);

            REQUIRE(n2.getVariableCreator().getViewDomain(v11).lower()==7);
            REQUIRE(n2.getVariableCreator().getViewDomain(v12).lower()==7);
            REQUIRE(n2.getVariableCreator().getViewDomain(v13).lower()==7);

            //REQUIRE(p2.propagate_reified(p2,l2)==Propagator::value::UNKNOWN);
            //std::cout << p2 << std::endl;



            View v111 = n2.createView(Domain(5,5));
            View v112 = n2.createView(Domain(10,10));
            View v113 = n2.createView(Domain(5,5));
            LinearConstraint l3(LinearConstraint::Relation::EQ);
            l3.add(v111);
            l3.add(v112*-1);
            l3.add(v113);
            l3.addRhs(0);
            //std::cout << std::endl << l << std::endl;

            //REQUIRE(p2.propagate_reified(p2,l3)==Propagator::value::TRUE);


            //View v114 = vc2.createView(Domain(5,7));
            //l3.normalize();

            //REQUIRE(p2.propagate_reified(p2,l3)==Propagator::value::FALSE);

            View v1111 = n2.createView(Domain(5,10));
            View v1112 = n2.createView(Domain(5,10));
            View v1113 = n2.createView(Domain(5,10));
            LinearConstraint l4(LinearConstraint::Relation::LE);
            l4.add(v1111*-1);
            l4.add(v1112*-1);
            l4.add(v1113*-1);
            l4.addRhs(-27-1);
            //std::cout << std::endl << l << std::endl;

            //REQUIRE(p2.propagate_reified(p2,l)==Propagator::value::UNKNOWN);
            n2.addConstraint(ReifiedLinearConstraint(std::move(l4),s.falseLit(),Direction::EQ));
            n2.prepare();
            //s.createNewLiterals(n2.estimateVariables());

            //p2.propagate_false(p2,l4);
            //std::cout << p2 << std::endl;

            //REQUIRE(n2.getVariableCreator().getViewDomain(v1111).getUpper()==10);
            //REQUIRE(n2.getVariableCreator().getViewDomain(v1112).getUpper()==10);
            //REQUIRE(n2.getVariableCreator().getViewDomain(v1113).getUpper()==10);

            //REQUIRE(n2.getVariableCreator().getViewDomain(v1111).getLower()()==8);
            //REQUIRE(n2.getVariableCreator().getViewDomain(v1112).getLower()()==8);
            //REQUIRE(n2.getVariableCreator().getViewDomain(v1113).getLower()()==8);



        }

        for (int i = 0; i < 12; ++i)
        {
            auto r1 = n.getVariableCreator().getViewDomain(i);
            auto r2 = n2.getVariableCreator().getViewDomain(i);

            REQUIRE(r1.empty()==r2.empty());
            if (!r1.empty())
            {
                REQUIRE(r1.lower()==r2.lower());
                REQUIRE(r1.upper()==r2.upper());
            }
        }



        {
            View v11111 = n2.createView(Domain(0,100));
            View v11112 = n2.createView(Domain(0,100));
            LinearConstraint l5(LinearConstraint::Relation::LE);
            l5.add(v11111*-1);
            l5.add(v11112);
            l5.addRhs(1-1);
            //std::cout << std::endl << l << std::endl;

            LinearConstraint l6(LinearConstraint::Relation::LE);
            l6.add(v11111);
            l6.add(v11112*-1);
            l6.addRhs(1-1);
            //std::cout << std::endl << l << std::endl;

            /*Normalizer newn(s,n2.getVariableCreator());
        //REQUIRE(p2.propagate_reified(p2,l)==Propagator::value::UNKNOWN);
        newn.addConstraint(ReifiedLinearConstraint(std::move(l5),s.falseLit()));
        newn.addConstraint(ReifiedLinearConstraint(std::move(l6),s.falseLit()));
        REQUIRE(newn.createClauses());*/
        }

    }


    TEST_CASE("TestPropagation2", "[linearPropagator]")
    {

        MySolver s;
        Normalizer n(s, translateConfig);


        View v1 = n.createView(Domain(0,4));
        View v2 = n.createView(Domain(0,4));
        View v3 = n.createView(Domain(0,4));

        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(v1);
        l.add(v2);
        l.add(v3);
        l.addRhs(10);
        //std::cout << std::endl << l << std::endl;

        //REQUIRE(p.propagate_reified(p,l)==Propagator::value::UNKNOWN);
        //p.propagate_true(p,l);
        n.addConstraint(ReifiedLinearConstraint(std::move(l),s.falseLit(),Direction::EQ));
        n.prepare();
        //s.createNewLiterals(n.estimateVariables());

        //p.addImp(n.removeConstraints());
        //p.propagate();

        //std::cout << p << std::endl;

        REQUIRE(n.getVariableCreator().getViewDomain(v1).lower()==3);
        REQUIRE(n.getVariableCreator().getViewDomain(v2).lower()==3);
        REQUIRE(n.getVariableCreator().getViewDomain(v3).lower()==3);
    }

    TEST_CASE("TestPropagation3", "[linearPropagator]")
    {

        MySolver s;
        Normalizer n(s, translateConfig);


        View v1 = n.createView(Domain(-4,0));
        View v2 = n.createView(Domain(-4,0));
        View v3 = n.createView(Domain(-4,0));

        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(v1*-1);
        l.add(v2*-1);
        l.add(v3*-1);
        l.addRhs(10);
        //std::cout << std::endl << l << std::endl;

        //REQUIRE(p.propagate_reified(p,l)==Propagator::value::UNKNOWN);
        //p.propagate_true(p,l);
        n.addConstraint(ReifiedLinearConstraint(std::move(l),s.falseLit(),Direction::EQ));
        n.prepare();
        //s.createNewLiterals(n.estimateVariables());

        //p.addImp(n.removeConstraints());
        //p.propagate();

        //std::cout << p << std::endl;

        REQUIRE(n.getVariableCreator().getViewDomain(v1).upper()==-3);
        REQUIRE(n.getVariableCreator().getViewDomain(v2).upper()==-3);
        REQUIRE(n.getVariableCreator().getViewDomain(v3).upper()==-3);
    }

