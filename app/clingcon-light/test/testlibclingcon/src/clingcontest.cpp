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
#include "clasp/clasp_facade.h"
#include "clasp/solver.h"
#include "clasp/cli/clasp_output.h"
#include "clasp/logic_program.h"
#include "clingcon/clingconorderpropagator.h"
#include "clingcon/clingcondlpropagator.h"
#include "order/solver.h"
#include "order/normalizer.h"
#include "order/configs.h"
#include <memory>
#include <sstream>


#include <ctime>
#include <chrono>
#include <algorithm>

class ClingconConfig : public Clasp::ClaspConfig {
public:
    ClingconConfig(Clasp::SharedContext& c, order::Config conf) : creator_(c), n_(creator_,conf), conf_(conf), cp_(0), s_(0)
    {
    }
    ~ClingconConfig()
    {
        if (cp_)
            s_->removePost(cp_);
        delete cp_;
    }

    virtual bool addPost(Clasp::Solver& s) const
    {
        s_ = &s;
//        if (conf_.dlprop==2)
//            if (!addDLProp(s,n_.constraints()))
//                return false;

//        std::vector<order::ReifiedLinearConstraint> constraints;
//        if (conf_.dlprop==1)
//        {
//            constraints = n_.constraints();
//        }

        ///solver takes ownership of propagator
        cp_ = new clingcon::ClingconOrderPropagator(s, n_.getVariableCreator(), conf_,
                                                                                      std::move(n_.constraints()),n_.getEqualities(),0);
        if (!s.addPost(cp_))
            return false;

//        if (conf_.dlprop==1)
//            if (!addDLProp(s, constraints))
//                return false;
        return ClaspConfig::addPost(s);
    }

//    bool addDLProp(Clasp::Solver& s, const std::vector<order::ReifiedLinearConstraint>& constraints) const
//    {
//        clingcon::ClingconDLPropagator* dlp = new clingcon::ClingconDLPropagator(s, conf_);
//        for (const auto&i : constraints)
//        {
//            if (dlp->isValidConstraint(i))
//                dlp->addValidConstraint(i);
//        }
//        if (!s.addPost(dlp))
//            return false;
//        return true;
//    }

    //clingcon::ClingconPropagator* prop_;///prop_ = new clingcon::ClingconPropagator(new MySolver(&s));

    //std::vector<std::unique_ptr<MySolver> > solvers_;
    //std::vector<std::unique_ptr<clingcon::ClingconPropagator> > props_;
    MySharedContext creator_;
    mutable order::Normalizer n_;
    order::Config conf_;
    mutable clingcon::ClingconOrderPropagator* cp_;
    mutable Clasp::Solver* s_;
};


using namespace order;



    TEST_CASE("Empty", "1")
    {
        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, lazySolveConfigProp4);
        conf.solve.numModels = 0;
        //conf.solve.project =  1;

        f.startAsp(conf);
        //auto False = lp.newAtom();

        //lp.setCompute(False, false);

        f.prepare();

        //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
        f.solve();
    }


    TEST_CASE("Simple logic", "1")
    {

        //std::cout << "start test 0 " << std::endl;
        Clasp::ClaspFacade f;
        auto myconf = lazySolveConfigProp4;
        myconf.dlprop=0;
        ClingconConfig conf(f.ctx, myconf);
        conf.solve.numModels = 150;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View s = conf.n_.createView(Domain(0,9));
            View e = conf.n_.createView(Domain(0,9));
            //       View n = conf.n_.createView(Domain(0,9));
            //      View d = conf.n_.createView(Domain(0,9));

            //       View m = conf.n_.createView(Domain(0,0));
            /*       View o = conf.n_.createView(Domain(0,9));
        View r = conf.n_.createView(Domain(0,9));*/
            /*
        View y = conf.n_.createView(Domain(0,9));
*/
            /*
        {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(s*1);
        l.addRhs(0);
        //std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
        }

        {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(m*1);
        l.addRhs(0);
        //std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
        }*/

            {
                LinearConstraint l(LinearConstraint::Relation::LE);
                l.add(s);
                l.add(e);
                l.addRhs(12);
                //std::cout << std::endl << l << std::endl;
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());

            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;
                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="s"; break;
                    case 1: varname="e"; break;
                    case 2: varname="n"; break;
                    case 3: varname="d"; break;
                    case 4: varname="m"; break;
                    case 5: varname="o"; break;
                    case 6: varname="r"; break;
                    case 7: varname="y"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        //st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            f.solve();
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==79);
            //TODO: compare with translation based approach



        }
    }

    void test1aux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        /// {a,b}.
        /// a :- b.
        /// :- a, b, not a+b+c <= 17.
        /// :- not b.
        auto a = lp.newAtom();
        auto b = lp.newAtom();
        lp.addOutput("a",a);
        lp.addOutput("b",b);
        {
            Potassco::RuleBuilder rb;
            rb.start(Potassco::Head_t::Choice);
            rb.addHead(a).addHead(b).end();
            lp.addRule(rb.rule());
        }

        {
            Potassco::RuleBuilder rb;
            rb.start().addHead(a).addGoal(b).end();
            lp.addRule(rb.rule());
        }


        auto constraint1 = lp.newAtom();
        lp.addOutput("a+b+c<=17",constraint1);
        {
            Potassco::RuleBuilder rb;
            rb.start().addGoal(a).addGoal(b).addGoal(Potassco::neg(constraint1))/*.addHead(lp.falseAtom())*/.end();
            lp.addRule(rb.rule());
        }

        {
            //// ADDITIONAL STUFF
            Potassco::RuleBuilder rb;
            rb.start(Potassco::Head_t::Choice).addHead(constraint1).end();
            lp.addRule(rb.rule());
        }




        REQUIRE(lp.end()); /// UNSAT

        auto ia = conf.n_.createView(order::Domain(5,10));
        auto ib = conf.n_.createView(order::Domain(5,10));
        auto ic = conf.n_.createView(order::Domain(5,10));
        order::LinearConstraint l(order::LinearConstraint::Relation::LE);
        l.addRhs(17);
        l.add(ia);
        l.add(ib);
        l.add(ic);
        //// at least getting the literal from the View has to take place after lp.end() has been called
        linearConstraints.emplace_back(order::ReifiedLinearConstraint(std::move(l),toOrderFormat(lp.getLiteral(constraint1)),Direction::EQ));
        ////
        //f.ctx.unfreeze();
        for (auto &i : linearConstraints)
            conf.n_.addConstraint(std::move(i));

        REQUIRE(conf.n_.prepare());

        REQUIRE(conf.n_.finalize());

        //f.ctx.startAddConstraints(1000);
        //REQUIRE(conf.n_.createClauses()); /// UNSAT
        f.prepare();

        //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
        //f.solve(&to); /// 442 solutions
        f.solve();
        REQUIRE(f.summary().numEnum==442);
        //f.solve();

        /// first thing in next incremental step is
        /*
         *r.reset();
        r.setType(Clasp::Asp::BASICRULE);
        r.addToBody(atomId, false);
        r.addHead(False);
        lp.addRule(r);
         *
         */
    }

    TEST_CASE("Simple ASP", "1")
    {
        for (auto i : conf1)
            test1aux(i);
    }

    void test3aux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View s = conf.n_.createView(Domain(7,9));
            View e = conf.n_.createView(Domain(0,9));
            //       View n = conf.n_.createView(Domain(0,9));
            //      View d = conf.n_.createView(Domain(0,9));

            View m = conf.n_.createView(Domain(1,2));
            /*       View o = conf.n_.createView(Domain(0,9));
        View r = conf.n_.createView(Domain(0,9));*/
            /*
        View y = conf.n_.createView(Domain(0,9));
*/
            /*
        {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(s,1);
        l.addRhs(0);
        //std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
        }

        {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(m,1);
        l.addRhs(0);
        //std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
        }*/

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1000);
                l.add(e*100);
                //        l.add(n,10);
                //      l.add(d,1);
                l.add(m*1000);
                /*    l.add(o,100);
        l.add(r,10);
        l.add(e,1);*//*
        l.add(m*-10000);
        l.add(o*-1000);
        l.add(n*-100);
        l.add(e*-10);
        l.add(y*-1);*/
                l.addRhs(9500);
                //std::cout << std::endl << l << std::endl;
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());



            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="s"; break;
                    case 1: varname="e"; break;
                    case 2: varname="n"; break;
                    case 3: varname="d"; break;
                    case 4: varname="m"; break;
                    case 5: varname="o"; break;
                    case 6: varname="r"; break;
                    case 7: varname="y"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==2);
            //TODO: compare with translation based approach



        }
    }

    TEST_CASE("Equality Translation", "3")
    {
        for (auto i : conf1)
            test3aux(i);
    }

    void test4aux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            //       View s = conf.n_.createView(Domain(0,9));
            //       View e = conf.n_.createView(Domain(0,9));

            View d = conf.n_.createView(Domain(0,4));
            //        View n = conf.n_.createView(Domain(0,5));

            //       View m = conf.n_.createView(Domain(0,9));
            //       View o = conf.n_.createView(Domain(0,9));
            //       View r = conf.n_.createView(Domain(0,9));

            //       View y = conf.n_.createView(Domain(0,9));


            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(d*1);
                l.addRhs(3);
                //std::cout << std::endl << l << std::endl;

                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(d*1);
                l.addRhs(1);
                //std::cout << std::endl << l << std::endl;

                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }


            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());



            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="s"; break;
                    case 1: varname="e"; break;
                    case 2: varname="n"; break;
                    case 3: varname="d"; break;
                    case 4: varname="m"; break;
                    case 5: varname="o"; break;
                    case 6: varname="r"; break;
                    case 7: varname="y"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            f.solve();
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==3);
            //TODO: compare with translation based approach



        }
    }

    TEST_CASE("Domain Constraint Propagation", "4")
    {
        for (auto i : conf1)
            test4aux(i);
    }


    void test5aux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View s = conf.n_.createView(Domain(0,5));
            View e = conf.n_.createView(Domain(0,5));


            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(e*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());



            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="s"; break;
                    case 1: varname="e"; break;
                    case 2: varname="n"; break;
                    case 3: varname="d"; break;
                    case 4: varname="m"; break;
                    case 5: varname="o"; break;
                    case 6: varname="r"; break;
                    case 7: varname="y"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            f.solve();
            //f.solve();
            // std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==30);
            //TODO: compare with translation based approach



        }
    }


    TEST_CASE("Eq Propagation", "5")
    {
        for (auto i : conf1)
            test5aux(i);
    }


    void test6aux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View s = conf.n_.createView(Domain(0,9));
            View e = conf.n_.createView(Domain(0,9));
            View n = conf.n_.createView(Domain(0,9));
            View d = conf.n_.createView(Domain(0,9));

            //      View m = conf.n_.createView(Domain(0,9));
            //      View o = conf.n_.createView(Domain(0,9));
            //      View r = conf.n_.createView(Domain(0,9));

            //      View y = conf.n_.createView(Domain(0,9));

            /*
        {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(s,1);
        l.addRhs(0);
        //std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
        }

        {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(m,1);
        l.addRhs(0);
        //std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
        }
*/
            /*        {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(s,1000);
        l.add(e,100);
        l.add(n,10);
        l.add(d,1);
        l.add(m,1000);
        l.add(o,100);
        l.add(r,10);
        l.add(e,1);
        l.add(m*-10000);
        l.add(o*-1000);
        l.add(n*-100);
        l.add(e*-10);
        l.add(y*-1);
        l.addRhs(0);
        //std::cout << std::endl << l << std::endl;
        linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
        }
*/

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(e*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(n*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(d*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1);
                l.add(n*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1);
                l.add(d*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(n*1);
                l.add(d*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());



            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="s"; break;
                    case 1: varname="e"; break;
                    case 2: varname="n"; break;
                    case 3: varname="d"; break;
                    case 4: varname="m"; break;
                    case 5: varname="o"; break;
                    case 6: varname="r"; break;
                    case 7: varname="y"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==5040);
            //TODO: compare with translation based approach



        }
    }


    TEST_CASE("Send", "6")
    {
        for (auto i : conf1)
            test6aux(i);
    }


    void sendMoreMoneyaux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View s = conf.n_.createView(Domain(0,9));
            View e = conf.n_.createView(Domain(0,9));
            View n = conf.n_.createView(Domain(0,9));
            View d = conf.n_.createView(Domain(0,9));

            View m = conf.n_.createView(Domain(0,9));
            View o = conf.n_.createView(Domain(0,9));
            View r = conf.n_.createView(Domain(0,9));

            View y = conf.n_.createView(Domain(0,9));


            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;

                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(m);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;

                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1000);
                l.add(e*100);
                l.add(n*10);
                l.add(d*1);
                l.add(m*1000);
                l.add(o*100);
                l.add(r*10);
                l.add(e*1);
                l.add(m*-10000);
                l.add(o*-1000);
                l.add(n*-100);
                l.add(e*-10);
                l.add(y*-1);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }


            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(e*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(n*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(d*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(m*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(o*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(r*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(y*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1);
                l.add(n*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1);
                l.add(d*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1);
                l.add(m*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1);
                l.add(o*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1);
                l.add(r*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1);
                l.add(y*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(n*1);
                l.add(d*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(n*1);
                l.add(m*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(n*1);
                l.add(o*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(n*1);
                l.add(r*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(n*1);
                l.add(y*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(d*1);
                l.add(m*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(d*1);
                l.add(o*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(d*1);
                l.add(r*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(d*1);
                l.add(y*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(m*1);
                l.add(o*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(m*1);
                l.add(r*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(m*1);
                l.add(y*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(o*1);
                l.add(r*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(o*1);
                l.add(y*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());



            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="s"; break;
                    case 1: varname="e"; break;
                    case 2: varname="n"; break;
                    case 3: varname="d"; break;
                    case 4: varname="m"; break;
                    case 5: varname="o"; break;
                    case 6: varname="r"; break;
                    case 7: varname="y"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==1);
            //TODO: compare with translation based approach



        }
    }

    TEST_CASE("SendMoreMoney1", "1")
    {
        for (auto i : conf1)
            sendMoreMoneyaux(i);
    }


    void sendMoreMoney2aux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;

        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View s = conf.n_.createView(Domain(0,9));
            View e = conf.n_.createView(Domain(0,9));
            View n = conf.n_.createView(Domain(0,9));
            View d = conf.n_.createView(Domain(0,9));

            View m = conf.n_.createView(Domain(0,9));
            View o = conf.n_.createView(Domain(0,9));
            View r = conf.n_.createView(Domain(0,9));

            View y = conf.n_.createView(Domain(0,9));


            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;

                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(m);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;

                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1000);
                l.add(e*100);
                l.add(n*10);
                l.add(d*1);
                l.add(m*1000);
                l.add(o*100);
                l.add(r*10);
                l.add(e*1);
                l.add(m*-10000);
                l.add(o*-1000);
                l.add(n*-100);
                l.add(e*-10);
                l.add(y*-1);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }


            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(e*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(n*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(d*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(m*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(o*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(r*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.add(y*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1);
                l.add(n*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1);
                l.add(d*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1);
                l.add(m*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1);
                l.add(o*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1);
                l.add(r*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1);
                l.add(y*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(n*1);
                l.add(d*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(n*1);
                l.add(m*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(n*1);
                l.add(o*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(n*1);
                l.add(r*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(n*1);
                l.add(y*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(d*1);
                l.add(m*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(d*1);
                l.add(o*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(d*1);
                l.add(r*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(d*1);
                l.add(y*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(m*1);
                l.add(o*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(m*1);
                l.add(r*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(m*1);
                l.add(y*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(o*1);
                l.add(r*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(o*1);
                l.add(y*-1);
                l.addRhs(0);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="s"; break;
                    case 1: varname="e"; break;
                    case 2: varname="n"; break;
                    case 3: varname="d"; break;
                    case 4: varname="m"; break;
                    case 5: varname="o"; break;
                    case 6: varname="r"; break;
                    case 7: varname="y"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==1);
            //TODO: compare with translation based approach



        }
    }

    TEST_CASE("SendMoreMoney2", "2")
    {
        for (auto i : conf1)
            sendMoreMoney2aux(i);
    }

    void nQueensaux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;
        conf.n_.getConfig().disjoint2distinct = false;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;


            View q[10];
            for (unsigned int i = 0; i < 10; ++i)
                q[i] = conf.n_.createView(Domain(1,10));

            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> vars;

                for (unsigned int i = 0; i < 10; ++i)
                {
                    std::vector<std::vector<order::Literal>> l;
                    l.push_back(std::vector<order::Literal>());

                    std::vector<std::pair<View,ReifiedDNF>> one;
                    one.push_back(std::make_pair(q[i],ReifiedDNF(std::move(l))));
                    vars.emplace_back(one);

                }
                conf.n_.addConstraint(ReifiedDisjoint(std::move(vars),solver.trueLit(),Direction::EQ));
            }

            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> vars;

                for (unsigned int i = 0; i < 10; ++i)
                {
                    std::vector<std::vector<order::Literal>> l;
                    l.push_back(std::vector<order::Literal>());

                    std::vector<std::pair<View,ReifiedDNF>> one;

                    order::LinearConstraint lin(order::LinearConstraint::Relation::EQ);
                    lin.addRhs(-i-1);
                    lin.add(q[i]*1);
                    View b = conf.n_.createView();
                    lin.add(b*-1);
                    linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(lin),solver.trueLit(),Direction::EQ));


                    one.push_back(std::make_pair(b,ReifiedDNF(std::move(l))));
                    vars.emplace_back(one);
                }
                conf.n_.addConstraint(ReifiedDisjoint(std::move(vars),solver.trueLit(),Direction::EQ));
            }


            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> vars;
                for (unsigned int i = 0; i < 10; ++i)
                {
                    std::vector<std::vector<order::Literal>> l;
                    l.push_back(std::vector<order::Literal>());

                    std::vector<std::pair<View,ReifiedDNF>> one;

                    order::LinearConstraint lin(order::LinearConstraint::Relation::EQ);
                    lin.addRhs(i+1);
                    lin.add(q[i]*1);
                    View b = conf.n_.createView();
                    lin.add(b*-1);
                    linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(lin),solver.trueLit(),Direction::EQ));


                    one.push_back(std::make_pair(b,ReifiedDNF(std::move(l))));
                    vars.emplace_back(one);
                }
                conf.n_.addConstraint(ReifiedDisjoint(std::move(vars),solver.trueLit(),Direction::EQ));
            }
            //conf.n_.addConstraint(ReifiedDisjoint({s,e,n,d,m,o,r,e,m,o,n,e,y},solver.trueLit(),Direction::EQ));

        }


        for (auto &i : linearConstraints)
            conf.n_.addConstraint(std::move(i));

        REQUIRE(conf.n_.prepare());

        REQUIRE(conf.n_.finalize());


        if (conf.n_.getConfig().minLitsPerVar == -1)
        {
            ///name the order lits
            /// i just use free id's for this.
            /// in the next incremental step these id's need to be
            /// made false variables

            Clasp::OutputTable& st = f.ctx.output;

            for (std::size_t i = 0; i < 10; ++i)
            {
                std::string varname;
                switch(i)
                {
                case 0: varname="q[0]"; break;
                case 1: varname="q[1]"; break;
                case 2: varname="q[2]"; break;
                case 3: varname="q[3]"; break;
                case 4: varname="q[4]"; break;
                case 5: varname="q[5]"; break;
                case 6: varname="q[6]"; break;
                case 7: varname="q[7]"; break;
                case 8: varname="q[8]"; break;
                case 9: varname="q[9]"; break;
                }

                auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                {
                    std::stringstream ss;
                    ss << varname << "<=" << *litresit;
                    st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                }
            }


        }
        //f.ctx.startAddConstraints(1000);
        //REQUIRE(conf.n_.createClauses()); /// UNSAT
        f.prepare();
        //std::cout << "constraints: " << f.ctx.stats().numConstraints() << std::endl;
        //std::cout << "variables:   " << f.ctx.stats().vars << std::endl;

        //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
        //f.solve(&to);
        f.solve();
        //std::cout << "conflicts: " << f.ctx.master()->stats.conflicts << std::endl;
        //std::cout << "choices: " << f.ctx.master()->stats.choices << std::endl;
        //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
        REQUIRE(f.summary().numEnum==724);
        //TODO: compare with translation based approach

    }


    TEST_CASE("nQueens", "1")
    {
        for (auto i : conf1)
            nQueensaux(i);
    }


    void nQueensExaux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;
        conf.n_.getConfig().disjoint2distinct = false;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;


            View q[10];
            for (unsigned int i = 0; i < 10; ++i)
                q[i] = conf.n_.createView(Domain(1,10));

            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> vars;

                for (unsigned int i = 0; i < 10; ++i)
                {
                    std::vector<std::vector<order::Literal>> l;
                    l.push_back(std::vector<order::Literal>());

                    std::vector<std::pair<View,ReifiedDNF>> one;
                    one.push_back(std::make_pair(q[i],ReifiedDNF(std::move(l))));
                    vars.emplace_back(one);

                }
                conf.n_.addConstraint(ReifiedDisjoint(std::move(vars),solver.trueLit(),Direction::EQ));
            }

            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> vars;

                for (unsigned int i = 0; i < 10; ++i)
                {
                    std::vector<std::vector<order::Literal>> l;
                    l.push_back(std::vector<order::Literal>());

                    std::vector<std::pair<View,ReifiedDNF>> one;

                    /*order::LinearConstraint lin(order::LinearConstraint::Relation::EQ);
                lin.addRhs(-i-1);
                lin.add(q[i]*1);
                View b = conf.n_.createView();
                lin.add(b*-1);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(lin),solver.trueLit(),Direction::EQ));*/

                    View b(q[i].v,1,i+1);

                    one.push_back(std::make_pair(b,ReifiedDNF(std::move(l))));
                    vars.emplace_back(one);
                }
                conf.n_.addConstraint(ReifiedDisjoint(std::move(vars),solver.trueLit(),Direction::EQ));
            }


            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> vars;
                for (unsigned int i = 0; i < 10; ++i)
                {
                    std::vector<std::vector<order::Literal>> l;
                    l.push_back(std::vector<order::Literal>());

                    std::vector<std::pair<View,ReifiedDNF>> one;

                    /*order::LinearConstraint lin(order::LinearConstraint::Relation::EQ);
                lin.addRhs(i+1);
                lin.add(q[i]*1);
                View b = conf.n_.createView();
                lin.add(b*-1);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(lin),solver.trueLit(),Direction::EQ));
                */
                    View b(q[i].v,1,-i-1);

                    one.push_back(std::make_pair(b,ReifiedDNF(std::move(l))));
                    vars.emplace_back(one);
                }
                conf.n_.addConstraint(ReifiedDisjoint(std::move(vars),solver.trueLit(),Direction::EQ));
            }
            //conf.n_.addConstraint(ReifiedDisjoint({s,e,n,d,m,o,r,e,m,o,n,e,y},solver.trueLit(),Direction::EQ));

        }


        for (auto &i : linearConstraints)
            conf.n_.addConstraint(std::move(i));

        REQUIRE(conf.n_.prepare());

        REQUIRE(conf.n_.finalize());


        if (conf.n_.getConfig().minLitsPerVar == -1)
        {
            ///name the order lits
            /// i just use free id's for this.
            /// in the next incremental step these id's need to be
            /// made false variables

            Clasp::OutputTable& st = f.ctx.output;

            for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
            {
                std::string varname;
                switch(i)
                {
                case 0: varname="q[0]"; break;
                case 1: varname="q[1]"; break;
                case 2: varname="q[2]"; break;
                case 3: varname="q[3]"; break;
                case 4: varname="q[4]"; break;
                case 5: varname="q[5]"; break;
                case 6: varname="q[6]"; break;
                case 7: varname="q[7]"; break;
                case 8: varname="q[8]"; break;
                case 9: varname="q[9]"; break;
                }

                auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                {
                    std::stringstream ss;
                    ss << varname << "<=" << *litresit;
                    st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                }
            }


        }
        //f.ctx.startAddConstraints(1000);
        //REQUIRE(conf.n_.createClauses()); /// UNSAT
        f.prepare();
        //std::cout << "constraints: " << f.ctx.stats().numConstraints() << std::endl;
        //std::cout << "variables:   " << f.ctx.stats().vars << std::endl;

        //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
        //f.solve(&to);
        f.solve();
        //std::cout << "conflicts: " << f.ctx.master()->stats.conflicts << std::endl;
        //std::cout << "choices: " << f.ctx.master()->stats.choices << std::endl;
        //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
        REQUIRE(f.summary().numEnum==724);
        //TODO: compare with translation based approach

    }



    TEST_CASE("nQueensEx", "1")
    {
        for (auto i : conf1)
            nQueensExaux(i);
    }


    void crypt112aux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;
        conf.n_.getConfig().disjoint2distinct = false;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT


        MySharedContext& solver = conf.creator_;


        {

            View e = conf.n_.createView(Domain(0,9));
            View i = conf.n_.createView(Domain(0,9));
            View n = conf.n_.createView(Domain(0,9));
            View s = conf.n_.createView(Domain(0,9));

            View z = conf.n_.createView(Domain(0,9));
            View w = conf.n_.createView(Domain(0,9));


            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1000);
                l.add(i*100);
                l.add(n*10);
                l.add(s*1);
                l.add(e*1000);
                l.add(i*100);
                l.add(n*10);
                l.add(s*1);
                l.add(z*-1000);
                l.add(w*-100);
                l.add(e*-10);
                l.add(i*-1);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            conf.n_.addConstraint(ReifiedAllDistinct({e,i,n,s,z,w},solver.trueLit(),Direction::EQ));

        }


        for (auto &i : linearConstraints)
            conf.n_.addConstraint(std::move(i));

        REQUIRE(conf.n_.prepare());

        REQUIRE(conf.n_.finalize());


        if (conf.n_.getConfig().minLitsPerVar == -1)
        {
            ///name the order lits
            /// i just use free id's for this.
            /// in the next incremental step these id's need to be
            /// made false variables

            Clasp::OutputTable& st = f.ctx.output;

            for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
            {
                std::string varname;
                switch(i)
                {
                case 0: varname="q[0]"; break;
                case 1: varname="q[1]"; break;
                case 2: varname="q[2]"; break;
                case 3: varname="q[3]"; break;
                case 4: varname="q[4]"; break;
                case 5: varname="q[5]"; break;
                case 6: varname="q[6]"; break;
                case 7: varname="q[7]"; break;
                case 8: varname="q[8]"; break;
                case 9: varname="q[9]"; break;
                }

                auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                {
                    std::stringstream ss;
                    ss << varname << "<=" << *litresit;
                    st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                }
            }


        }
        //f.ctx.startAddConstraints(1000);
        //REQUIRE(conf.n_.createClauses()); /// UNSAT
        f.prepare();

        //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
        //f.solve(&to);
        f.solve();
        //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
        REQUIRE(f.summary().numEnum==12);
        //TODO: compare with translation based approach

    }


    TEST_CASE("Crypt112", "1")
    {
        for (auto i : conf1)
            crypt112aux(i);
        //crypt112aux(nonlazySolveConfig);
    }


    void crypt224aux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;
        conf.n_.getConfig().disjoint2distinct = false;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT


        MySharedContext& solver = conf.creator_;


        {

            View z = conf.n_.createView(Domain(0,9));
            View w = conf.n_.createView(Domain(0,9));
            View e = conf.n_.createView(Domain(0,9));
            View i = conf.n_.createView(Domain(0,9));

            View v = conf.n_.createView(Domain(0,9));
            View r = conf.n_.createView(Domain(0,9));


            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(z*1000);
                l.add(w*100);
                l.add(e*10);
                l.add(i*1);
                l.add(z*1000);
                l.add(w*100);
                l.add(e*10);
                l.add(i*1);
                l.add(v*-1000);
                l.add(i*-100);
                l.add(e*-10);
                l.add(r*-1);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(z*1);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            conf.n_.addConstraint(ReifiedAllDistinct({z,w,e,i,v,r},solver.trueLit(),Direction::EQ));

        }





        for (auto &i : linearConstraints)
            conf.n_.addConstraint(std::move(i));

        REQUIRE(conf.n_.prepare());

        REQUIRE(conf.n_.finalize());


        if (conf.n_.getConfig().minLitsPerVar == -1)
        {
            ///name the order lits
            /// i just use free id's for this.
            /// in the next incremental step these id's need to be
            /// made false variables

            Clasp::OutputTable& st = f.ctx.output;

            for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
            {
                std::string varname;
                switch(i)
                {
                case 0: varname="q[0]"; break;
                case 1: varname="q[1]"; break;
                case 2: varname="q[2]"; break;
                case 3: varname="q[3]"; break;
                case 4: varname="q[4]"; break;
                case 5: varname="q[5]"; break;
                case 6: varname="q[6]"; break;
                case 7: varname="q[7]"; break;
                case 8: varname="q[8]"; break;
                case 9: varname="q[9]"; break;
                }

                auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                {
                    std::stringstream ss;
                    ss << varname << "<=" << *litresit;
                    st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                }
            }


        }
        //f.ctx.startAddConstraints(1000);
        //REQUIRE(conf.n_.createClauses()); /// UNSAT
        f.prepare();

        //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
        //f.solve(&to);
        f.solve();
        //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
        REQUIRE(f.summary().numEnum==12);
        //TODO: compare with translation based approach

    }


    TEST_CASE("Crypt224", "1")
    {
        for (auto i : conf1)
            crypt224aux(i);
    }

    void crypt145aux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;
        conf.n_.getConfig().disjoint2distinct = false;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT


        MySharedContext& solver = conf.creator_;

        //conf.creator_.createNewLiterals(1);

        {

            View e = conf.n_.createView();
            View i = conf.n_.createView(Domain(0,9));
            View n = conf.n_.createView(Domain(0,9));
            View s = conf.n_.createView(Domain(0,9));

            View v = conf.n_.createView(Domain(0,9));
            View r = conf.n_.createView(Domain(0,9));

            View f = conf.n_.createView(Domain(0,9));
            View u = conf.n_.createView(Domain(0,9));


            conf.n_.addConstraint(ReifiedDomainConstraint(e,Domain(0,9),solver.trueLit(),Direction::EQ));
            {
                LinearConstraint l(LinearConstraint::Relation::GE);
                l.add(e*1);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(e*1000);
                l.add(i*100);
                l.add(n*10);
                l.add(s*1);
                l.add(v*1000);
                l.add(i*100);
                l.add(e*10);
                l.add(r*1);
                l.add(f*-10000);
                l.add(u*-1000);
                l.add(e*-100);
                l.add(n*-10);
                l.add(f*-1);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            conf.n_.addConstraint(ReifiedAllDistinct({e,i,n,s,v,r,f,u},solver.trueLit(),Direction::EQ));

            {
                LinearConstraint l(LinearConstraint::Relation::GE);
                l.add(e*1);
                l.addRhs(4);
                //std::cout << std::endl << l << std::endl;
                solver.createNewLiterals(1);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.getNewLiteral(false),Direction::EQ));
            }

        }


        for (auto &i : linearConstraints)
            conf.n_.addConstraint(std::move(i));

        REQUIRE(conf.n_.prepare());

        REQUIRE(conf.n_.finalize());


        if (conf.n_.getConfig().minLitsPerVar == -1)
        {
            ///name the order lits
            /// i just use free id's for this.
            /// in the next incremental step these id's need to be
            /// made false variables

            Clasp::OutputTable& st = f.ctx.output;

            for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
            {
                std::string varname;
                switch(i)
                {
                case 0: varname="e"; break;
                case 1: varname="i"; break;
                case 2: varname="n"; break;
                case 3: varname="s"; break;
                case 4: varname="v"; break;
                case 5: varname="r"; break;
                case 6: varname="f"; break;
                case 7: varname="u"; break;
                }

                auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                {
                    std::stringstream ss;
                    ss << varname << "<=" << *litresit;
                    st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                }
            }


        }
        //f.ctx.startAddConstraints(1000);
        //REQUIRE(conf.n_.createClauses()); /// UNSAT
        f.prepare();

        //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
        //f.solve(&to);
        f.solve();
        //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
        REQUIRE(f.summary().numEnum==24);
        //TODO: compare with translation based approach

    }



   TEST_CASE("Crypt145", "1")
    {
        for (auto i : conf1)
            crypt145aux(i);
    }

   void allDiff1aux(order::Config c)
   {

       Clasp::ClaspFacade f;
       ClingconConfig conf(f.ctx, c);
       conf.solve.numModels = 0;
       //conf.solve.project =  1;
       //conf.n_.getConfig().pidgeon=false;

       Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
       //lp.start(f.ctx);

       std::vector<order::ReifiedLinearConstraint> linearConstraints;



       REQUIRE(lp.end()); /// UNSAT
       {

           View s = conf.n_.createView(Domain(0,2));
           View e = conf.n_.createView(Domain(0,1));

           conf.n_.addConstraint(ReifiedAllDistinct({s,e},conf.creator_.trueLit(),Direction::EQ));


           for (auto &i : linearConstraints)
               conf.n_.addConstraint(std::move(i));

           REQUIRE(conf.n_.prepare());


           REQUIRE(conf.n_.finalize());

           if (conf.n_.getConfig().minLitsPerVar == -1)
           {
               ///name the order lits
               /// i just use free id's for this.
               /// in the next incremental step these id's need to be
               /// made false variables

               Clasp::OutputTable& st = f.ctx.output;

               for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
               {
                   std::string varname;
                   switch(i)
                   {
                   case 0: varname="s"; break;
                   case 1: varname="e"; break;
                   case 2: varname="n"; break;
                   case 3: varname="d"; break;
                   case 4: varname="m"; break;
                   case 5: varname="o"; break;
                   case 6: varname="r"; break;
                   case 7: varname="y"; break;
                   }

                   auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                   for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                   {
                       std::stringstream ss;
                       ss << varname << "<=" << *litresit;
                       st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                   }
               }


           }

           //f.ctx.startAddConstraints(1000);
           //REQUIRE(conf.n_.createClauses()); /// UNSAT
           f.prepare();

           Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
           //f.solve(&to);
           f.solve();
           //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
           REQUIRE(f.summary().numEnum==4);
           //TODO: compare with translation based approach



       }
   }



    TEST_CASE("AllDiff1", "1")
    {
        for (auto i : conf1)
            allDiff1aux(i);
    }


    void allDiff2aux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT
        {



            View s = conf.n_.createView(Domain(0,2));
            View e = conf.n_.createView(Domain(0,1));
            View n = conf.n_.createView(Domain(0,1));
            //View d = conf.n_.createView(Domain(0,1));

            conf.n_.addConstraint(ReifiedAllDistinct({s,e,n},conf.creator_.trueLit(),Direction::EQ));


            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="s"; break;
                    case 1: varname="e"; break;
                    case 2: varname="n"; break;
                    case 3: varname="d"; break;
                    case 4: varname="m"; break;
                    case 5: varname="o"; break;
                    case 6: varname="r"; break;
                    case 7: varname="y"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==2);
            //TODO: compare with translation based approach



        }
    }



    TEST_CASE("AllDiff2", "2")
    {
        //allDiff2aux(lazySolveConfig);
        allDiff2aux(nonlazySolveConfig);
    }

    void allDiff3aux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;
        //conf.n_.getConfig().pidgeon=false;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT
        {

            View s = conf.n_.createView(Domain(0,1));
            View e = conf.n_.createView(Domain(0,1));
            View n = conf.n_.createView(Domain(0,1));
            //View d = conf.n_.createView(Domain(0,1));

            conf.n_.addConstraint(ReifiedAllDistinct({s,e,n},conf.creator_.trueLit(),Direction::EQ));


            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE_FALSE(conf.n_.prepare());
        }
    }


    TEST_CASE("AllDiff3", "3")
    {
        for (auto i : conf1)
            allDiff3aux(i);
    }

    void allDiff4aux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View s = conf.n_.createView(Domain(0,2));
            View e = conf.n_.createView(Domain(0,2));
            View n = conf.n_.createView(Domain(0,2));
            //View d = conf.n_.createView(Domain(0,1));

            conf.n_.addConstraint(ReifiedAllDistinct({s,e,n},solver.trueLit(),Direction::EQ));


            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="s"; break;
                    case 1: varname="e"; break;
                    case 2: varname="n"; break;
                    case 3: varname="d"; break;
                    case 4: varname="m"; break;
                    case 5: varname="o"; break;
                    case 6: varname="r"; break;
                    case 7: varname="y"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==6);
            //TODO: compare with translation based approach



        }
    }



    TEST_CASE("AllDiff4", "4")
    {
        for (auto i : conf1)
            allDiff4aux(i);
    }

    void sendMoreMoney3aux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 666;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View s = conf.n_.createView(Domain(0,9));
            View e = conf.n_.createView(Domain(0,9));
            View n = conf.n_.createView(Domain(0,9));
            View d = conf.n_.createView(Domain(0,9));

            View m = conf.n_.createView(Domain(0,9));
            View o = conf.n_.createView(Domain(0,9));
            View r = conf.n_.createView(Domain(0,9));

            View y = conf.n_.createView(Domain(0,9));


            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;

                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(m*1);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;

                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1000);
                l.add(e*100);
                l.add(n*10);
                l.add(d*1);
                l.add(m*1000);
                l.add(o*100);
                l.add(r*10);
                l.add(e*1);
                l.add(m*-10000);
                l.add(o*-1000);
                l.add(n*-100);
                l.add(e*-10);
                l.add(y*-1);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }


            conf.n_.addConstraint(ReifiedAllDistinct({s,e,n,d,m,o,r,e,m,o,n,e,y},solver.trueLit(),Direction::EQ));


            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="s"; break;
                    case 1: varname="e"; break;
                    case 2: varname="n"; break;
                    case 3: varname="d"; break;
                    case 4: varname="m"; break;
                    case 5: varname="o"; break;
                    case 6: varname="r"; break;
                    case 7: varname="y"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==1);
            //TODO: compare with translation based approach



        }
    }



    TEST_CASE("SendMoreMoney3", "3")
    {
        for (auto i : conf1)
            sendMoreMoney3aux(i);
    }


    void sendMoreMoney4aux(order::Config c)
    {

        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 100000;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View s = conf.n_.createView(Domain(9,9));
            View e = conf.n_.createView(Domain(2,9));
            View n = conf.n_.createView(Domain(3,9));
            View d = conf.n_.createView(Domain(5,9));

            View m = conf.n_.createView(Domain(1,1));
            View o = conf.n_.createView(Domain(0,0));
            View r = conf.n_.createView(Domain(8,9));
            View y = conf.n_.createView(Domain(2,7));


            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;

                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(m*1);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;

                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1000);
                l.add(e*100);
                l.add(n*10);
                l.add(d*1);
                l.add(m*1000);
                l.add(o*100);
                l.add(r*10);
                l.add(e*1);
                l.add(m*-10000);
                l.add(o*-1000);
                l.add(n*-100);
                l.add(e*-10);
                l.add(y*-1);
                l.addRhs(0);
                //std::cout << std::endl << l << std::endl;
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }


            conf.n_.addConstraint(ReifiedAllDistinct({s,e,n,d,m,o,r,e,m,o,n,e,y},solver.trueLit(),Direction::EQ));


            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="s"; break;
                    case 1: varname="e"; break;
                    case 2: varname="n"; break;
                    case 3: varname="d"; break;
                    case 4: varname="m"; break;
                    case 5: varname="o"; break;
                    case 6: varname="r"; break;
                    case 7: varname="y"; break;
                    default:
                    {
                        std::stringstream ss;
                        ss << i;
                        varname="V" + ss.str();
                        break;
                    }
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();


            Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was Test4 " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==1);
        }
    }


    TEST_CASE("SendMoreMoney4", "1")
    {
        for (auto i : conf1)
            sendMoreMoney4aux(i);
    }


    void unsat1aux(order::Config c)
    {
        {
            //std::cout << "start SMM3 " << std::endl;
            Clasp::ClaspFacade f;
            auto myconf =  c;
            myconf.dlprop = 2; // dlprop comes after
            ClingconConfig conf(f.ctx, myconf);
            conf.solve.numModels = 999;
            //conf.solve.project =  1;

            Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
            //lp.start(f.ctx);




            REQUIRE(lp.end()); /// UNSAT

            MySharedContext& solver = conf.creator_;


            View v10 = conf.n_.createView(Domain(1,10));
            View v1 =  conf.n_.createView(Domain(1,10));
            View v0 =  conf.n_.createView(Domain(1,10));
            View v11 = conf.n_.createView(Domain(1,10));

            {
                LinearConstraint l(LinearConstraint::Relation::LE);
                l.add(v10*1);
                l.add(v0*-1);
                l.addRhs(1);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::LE);
                l.add(v0*1);
                l.add(v1*-1);
                l.addRhs(0);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::LE);
                l.add(v1*1);
                l.add(v11*-1);
                l.addRhs(-2);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::LE);
                l.add(v11*1);
                l.add(v10*-1);
                l.addRhs(0);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }



            REQUIRE_FALSE(conf.n_.prepare());
        }
    }


    TEST_CASE("unsat1", "1")
    {
        for (auto i : conf1)
            unsat1aux(i);
    }


    void unsat2aux(order::Config c)
    {
        {
            //std::cout << "start SMM3 " << std::endl;
            Clasp::ClaspFacade f;
            auto myconf =  c;
            myconf.dlprop = 0; // dlprop comes after
            ClingconConfig conf(f.ctx, myconf);
            conf.solve.numModels = 999;
            //conf.solve.project =  1;

            Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
            //lp.start(f.ctx);




            REQUIRE(lp.end()); /// UNSAT

            MySharedContext& solver = conf.creator_;

            View v10 = conf.n_.createView(Domain(1,10));
            View v1 =  conf.n_.createView(Domain(1,10));
            View v0 =  conf.n_.createView(Domain(1,10));
            View v11 = conf.n_.createView(Domain(1,10));

            {
                LinearConstraint l(LinearConstraint::Relation::LE);
                l.add(v10*1);
                l.add(v0*-1);
                l.addRhs(1);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::LE);
                l.add(v0*1);
                l.add(v1*-1);
                l.addRhs(0);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::LE);
                l.add(v1*1);
                l.add(v11*-1);
                l.addRhs(-2);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::LE);
                l.add(v11*1);
                l.add(v10*-1);
                l.addRhs(0);
                solver.createNewLiterals(1);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.getNewLiteral(true),Direction::EQ));
            }



            solver.makeRestFalse();
            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="q(1)"; break;
                    case 1: varname="q(2)"; break;
                    case 2: varname="q(3)"; break;
                    case 3: varname="q(4)"; break;
                    case 4: varname="q(5)"; break;
                    case 5: varname="q(6)"; break;
                    case 6: varname="q(7)"; break;
                    case 7: varname="q(8)"; break;
                    case 8: varname="q(9)"; break;
                    case 9: varname="q(10)"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==450);
            //TODO: compare with translation based approach
        }
    }

    TEST_CASE("Unsat2", "1")
    {
        for (auto i : conf1)
            unsat2aux(i);
    }




//    void disjoint0()
//    {
//        for (auto i : conf1)
//            disjoint0aux(i);
//    }

//    void disjoint0aux(order::Config c)
//    {
//        {
//            //std::cout << "start SMM3 " << std::endl;
//            Clasp::ClaspFacade f;
//            auto myconf =  c;
//            myconf.dlprop = 2; // dlprop comes after
//            ClingconConfig conf(f.ctx, myconf);
//            unsigned int numM = 999;
//            //std::cout << "dl prop "<<myconf.dlprop << ",   " << numM << " models " << std::endl;
//            conf.solve.numModels = numM;
//            //conf.solve.project =  1;

//            Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
//            //lp.start(f.ctx);




//            REQUIRE(lp.end()); /// UNSAT

//            MySharedContext& solver = conf.creator_;


//            const int n = 10;

//            std::vector<View> views;
//            for (int i = 1; i <= n; ++i)
//                views.emplace_back(conf.n_.createView(Domain(1,n)));

//            std::vector<View> ldiag;
//            for (int i = 1; i <= n; ++i)
//            {
//                LinearConstraint l(LinearConstraint::Relation::EQ);
//                l.add(views[i-1]*1);
//                l.addRhs(-i);
//                ldiag.emplace_back(conf.n_.createView());
//                l.add(ldiag.back()*-1);
//                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
//            }

//            std::vector<View> rdiag;
//            for (int i = 1; i <= n; ++i)
//            {
//                LinearConstraint l(LinearConstraint::Relation::EQ);
//                l.add(views[i-1]*1);
//                l.addRhs(i);
//                rdiag.emplace_back(conf.n_.createView());
//                l.add(rdiag.back()*-1);
//                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
//            }



//            {
//                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
//                for (auto i : views)
//                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
//                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
//            }
//            {
//                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
//                for (auto i : ldiag)
//                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
//                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
//            }
//            {
//                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
//                for (auto i : rdiag)
//                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
//                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
//            }



//            REQUIRE(conf.n_.prepare());

//            REQUIRE(conf.n_.finalize());


//            if (conf.n_.getConfig().minLitsPerVar == -1)
//            {
//                ///name the order lits
//                /// i just use free id's for this.
//                /// in the next incremental step these id's need to be
//                /// made false variables

//                Clasp::OutputTable& st = f.ctx.output;

//                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
//                {
//                    std::string varname;
//                    switch(i)
//                    {
//                    case 0: varname="q(1)"; break;
//                    case 1: varname="q(2)"; break;
//                    case 2: varname="q(3)"; break;
//                    case 3: varname="q(4)"; break;
//                    case 4: varname="q(5)"; break;
//                    case 5: varname="q(6)"; break;
//                    case 6: varname="q(7)"; break;
//                    case 7: varname="q(8)"; break;
//                    case 8: varname="q(9)"; break;
//                    case 9: varname="q(10)"; break;
//                    }

//                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
//                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
//                    {
//                        std::stringstream ss;
//                        ss << varname << "<=" << *litresit;
//                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
//                    }
//                }


//            }
//            //f.ctx.startAddConstraints(1000);
//            //REQUIRE(conf.n_.createClauses()); /// UNSAT
//            f.prepare();

//            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
//            //f.solve(&to);
//            f.solve();
//            //            std::cout << "conflicts: " << f.ctx.master()->stats.conflicts << std::endl;
//            //            std::cout << "choices: " << f.ctx.master()->stats.choices << std::endl;
//            //            std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
//            /// THIS IS THE NUMBER FOR n=10
//            REQUIRE(f.summary().numEnum==std::min((unsigned int)(724),numM));
//            //std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
//            //            std::cout << "Printing took "
//            //                      << std::chrono::duration_cast<std::chrono::seconds>(end - start).count()
//            //                      << "s.\n";
//        }
//    }


//    void disjoint0Ex()
//    {
//        for (auto i : conf1)
//            disjoint0Exaux(i);
//    }

//    void disjoint0Exaux(order::Config c)
//    {
//        {
//            //std::cout << "start SMM3 " << std::endl;
//            Clasp::ClaspFacade f;
//            auto myconf =  c;
//            myconf.dlprop = 2; // dlprop comes after
//            ClingconConfig conf(f.ctx, myconf);
//            unsigned int numM = 999;
//            //std::cout << "dl prop "<<myconf.dlprop << ",   " << numM << " models " << std::endl;
//            conf.solve.numModels = numM;
//            //conf.solve.project =  1;

//            Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
//            //lp.start(f.ctx);




//            REQUIRE(lp.end()); /// UNSAT

//            MySharedContext& solver = conf.creator_;


//            const int n = 10;

//            std::vector<View> views;
//            for (int i = 1; i <= n; ++i)
//                views.emplace_back(conf.n_.createView(Domain(1,n)));

//            std::vector<View> ldiag;
//            for (int i = 1; i <= n; ++i)
//            {
//                /*LinearConstraint l(LinearConstraint::Relation::EQ);
//                l.add(views[i-1]*1);
//                l.addRhs(-i);
//                ldiag.emplace_back(conf.n_.createView());
//                l.add(ldiag.back()*-1);
//                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
//                */
//                ldiag.emplace_back(views[i-1].v,1,i);
//            }

//            std::vector<View> rdiag;
//            for (int i = 1; i <= n; ++i)
//            {
//                /*LinearConstraint l(LinearConstraint::Relation::EQ);
//                l.add(views[i-1]*1);
//                l.addRhs(i);
//                rdiag.emplace_back(conf.n_.createView());
//                l.add(rdiag.back()*-1);
//                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));*/
//                rdiag.emplace_back(views[i-1].v,1,-i);
//            }



//            {
//                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
//                for (auto i : views)
//                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
//                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
//            }
//            {
//                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
//                for (auto i : ldiag)
//                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
//                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
//            }
//            {
//                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
//                for (auto i : rdiag)
//                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
//                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
//            }



//            REQUIRE(conf.n_.prepare());

//            REQUIRE(conf.n_.finalize());


//            if (conf.n_.getConfig().minLitsPerVar == -1)
//            {
//                ///name the order lits
//                /// i just use free id's for this.
//                /// in the next incremental step these id's need to be
//                /// made false variables

//                Clasp::OutputTable& st = f.ctx.output;

//                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
//                {
//                    std::string varname;
//                    switch(i)
//                    {
//                    case 0: varname="q(1)"; break;
//                    case 1: varname="q(2)"; break;
//                    case 2: varname="q(3)"; break;
//                    case 3: varname="q(4)"; break;
//                    case 4: varname="q(5)"; break;
//                    case 5: varname="q(6)"; break;
//                    case 6: varname="q(7)"; break;
//                    case 7: varname="q(8)"; break;
//                    case 8: varname="q(9)"; break;
//                    case 9: varname="q(10)"; break;
//                    }

//                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
//                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
//                    {
//                        std::stringstream ss;
//                        ss << varname << "<=" << *litresit;
//                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
//                    }
//                }


//            }
//            //f.ctx.startAddConstraints(1000);
//            //REQUIRE(conf.n_.createClauses()); /// UNSAT
//            f.prepare();

//            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
//            //f.solve(&to);
//            f.solve();
//            //            std::cout << "conflicts: " << f.ctx.master()->stats.conflicts << std::endl;
//            //            std::cout << "choices: " << f.ctx.master()->stats.choices << std::endl;
//            //            std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
//            /// THIS IS THE NUMBER FOR n=10
//            REQUIRE(f.summary().numEnum==std::min((unsigned int)(724),numM));
//            //std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
//            //            std::cout << "Printing took "
//            //                      << std::chrono::duration_cast<std::chrono::seconds>(end - start).count()
//            //                      << "s.\n";
//        }
//    }




    void disjoint1aux(order::Config c)
    {
        {
            //std::cout << "start SMM3 " << std::endl;
            Clasp::ClaspFacade f;
            auto myconf =  c;
            //myconf.dlprop = 2; // dlprop comes after
            ClingconConfig conf(f.ctx, myconf);
            conf.solve.numModels = 999;
            //conf.solve.project =  1;

            Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
            //lp.start(f.ctx);




            REQUIRE(lp.end()); /// UNSAT

            MySharedContext& solver = conf.creator_;


            const int n = 8;

            std::vector<View> views;
            for (int i = 1; i <= n; ++i)
                views.emplace_back(conf.n_.createView(Domain(1,n)));

            std::vector<View> ldiag;
            for (int i = 1; i <= n; ++i)
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(views[i-1]*1);
                l.addRhs(-i);
                ldiag.emplace_back(conf.n_.createView());
                l.add(ldiag.back()*-1);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            std::vector<View> rdiag;
            for (int i = 1; i <= n; ++i)
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(views[i-1]*1);
                l.addRhs(i);
                rdiag.emplace_back(conf.n_.createView());
                l.add(rdiag.back()*-1);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }



            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
                for (auto i : views)
                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }
            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
                for (auto i : ldiag)
                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }
            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
                for (auto i : rdiag)
                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }



            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < n; ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="q(1)"; break;
                    case 1: varname="q(2)"; break;
                    case 2: varname="q(3)"; break;
                    case 3: varname="q(4)"; break;
                    case 4: varname="q(5)"; break;
                    case 5: varname="q(6)"; break;
                    case 6: varname="q(7)"; break;
                    case 7: varname="q(8)"; break;
                    case 8: varname="q(9)"; break;
                    case 9: varname="q(10)"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }

            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            /// THIS IS THE NUMBER FOR n=10
            REQUIRE(f.summary().numEnum==92);
            //TODO: compare with translation based approach
        }
    }

    TEST_CASE("Disjoint1", "1")
    {
        for (auto i : conf1)
            disjoint1aux(i);
    }



    void disjoint1Exaux(order::Config c)
    {
        {
            //std::cout << "start SMM3 " << std::endl;
            Clasp::ClaspFacade f;
            auto myconf =  c;
            //myconf.dlprop = 2; // dlprop comes after
            ClingconConfig conf(f.ctx, myconf);
            conf.solve.numModels = 999;
            //conf.solve.project =  1;

            Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
            //lp.start(f.ctx);




            REQUIRE(lp.end()); /// UNSAT

            MySharedContext& solver = conf.creator_;


            const int n = 8;

            std::vector<View> views;
            for (int i = 1; i <= n; ++i)
                views.emplace_back(conf.n_.createView(Domain(1,n)));

            std::vector<View> ldiag;
            for (int i = 1; i <= n; ++i)
            {
                /*LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(views[i-1]*1);
                l.addRhs(-i);
                ldiag.emplace_back(conf.n_.createView());
                l.add(ldiag.back()*-1);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));*/
                ldiag.emplace_back(views[i-1].v,1,i);
            }

            std::vector<View> rdiag;
            for (int i = 1; i <= n; ++i)
            {
                /*LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(views[i-1]*1);
                l.addRhs(i);
                rdiag.emplace_back(conf.n_.createView());
                l.add(rdiag.back()*-1);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));*/
                rdiag.emplace_back(views[i-1].v,1,-i);
            }



            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
                for (auto i : views)
                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }
            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
                for (auto i : ldiag)
                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }
            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
                for (auto i : rdiag)
                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }



            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="q(1)"; break;
                    case 1: varname="q(2)"; break;
                    case 2: varname="q(3)"; break;
                    case 3: varname="q(4)"; break;
                    case 4: varname="q(5)"; break;
                    case 5: varname="q(6)"; break;
                    case 6: varname="q(7)"; break;
                    case 7: varname="q(8)"; break;
                    case 8: varname="q(9)"; break;
                    case 9: varname="q(10)"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }

            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            /// THIS IS THE NUMBER FOR n=10
            REQUIRE(f.summary().numEnum==92);
            //TODO: compare with translation based approach
        }
    }


    TEST_CASE("Disjoint1Ex", "1")
    {
        for (auto i : conf1)
            disjoint1Exaux(i);
    }


    TEST_CASE("Disjoint2", "1")
    {
        {
            //std::cout << "start SMM3 " << std::endl;
            Config my = lazyDiffSolveConfig;
            my.minLitsPerVar = -1;
            Clasp::ClaspFacade f;
            ClingconConfig conf(f.ctx, my);
            conf.solve.numModels = 999;
            //conf.solve.project =  1;

            Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
            //lp.start(f.ctx);




            REQUIRE(lp.end()); /// UNSAT

            MySharedContext& solver = conf.creator_;


            const int n = 6;

            std::vector<View> views;
            for (int i = 1; i <= n; ++i)
                views.emplace_back(conf.n_.createView(Domain(1,n)));

            std::vector<View> ldiag;
            for (int i = 1; i <= n; ++i)
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(views[i-1]*1);
                l.addRhs(-i);
                ldiag.emplace_back(conf.n_.createView());
                l.add(ldiag.back()*-1);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            std::vector<View> rdiag;
            for (int i = 1; i <= n; ++i)
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(views[i-1]*1);
                l.addRhs(i);
                rdiag.emplace_back(conf.n_.createView());
                l.add(rdiag.back()*-1);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }



            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
                for (auto i : views)
                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }
            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
                for (auto i : ldiag)
                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }
            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
                for (auto i : rdiag)
                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }






            View q1 = views[0];
            View q2 = views[1];
            View q3 = views[2];
            View q4 = views[3];
            View q5 = views[4];
            View q6 = views[5];

            solver.createNewLiterals(50); /// for the equality lits that i generate manually ...
            Clasp::Literal eqs[6][6];
            for (auto i = 1; i<=6; ++i)
            {
                eqs[0][i-1] = toClaspFormat(conf.n_.getEqualLit(q1,i));
                eqs[1][i-1] = toClaspFormat(conf.n_.getEqualLit(q2,i));
                eqs[2][i-1] = toClaspFormat(conf.n_.getEqualLit(q3,i));
                eqs[3][i-1] = toClaspFormat(conf.n_.getEqualLit(q4,i));
                eqs[4][i-1] = toClaspFormat(conf.n_.getEqualLit(q5,i));
                eqs[5][i-1] = toClaspFormat(conf.n_.getEqualLit(q6,i));
            }

            // exclude sol 1
            LitVec v = {~conf.n_.getEqualLit(q1,2),
                        ~conf.n_.getEqualLit(q2,4),
                        ~conf.n_.getEqualLit(q3,6),
                        ~conf.n_.getEqualLit(q4,1),
                        ~conf.n_.getEqualLit(q5,3),
                        ~conf.n_.getEqualLit(q6,5)};
            // exclude sol 3
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q1,5),
                 ~conf.n_.getEqualLit(q2,3),
                 ~conf.n_.getEqualLit(q3,1),
                 ~conf.n_.getEqualLit(q4,6),
                 ~conf.n_.getEqualLit(q5,4),
                 ~conf.n_.getEqualLit(q6,2)};
            REQUIRE(conf.creator_.createClause(v));

            // enforce sol 2
            v = {conf.n_.getEqualLit(q2,6)};
            REQUIRE(conf.creator_.createClause(v));

            v = {~conf.n_.getEqualLit(q1,5)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q1,6)};
            REQUIRE(conf.creator_.createClause(v));

            v = {~conf.n_.getEqualLit(q3,4)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q3,5)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q3,6)};
            REQUIRE(conf.creator_.createClause(v));

            v = {~conf.n_.getEqualLit(q4,4)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q4,6)};
            REQUIRE(conf.creator_.createClause(v));

            v = {~conf.n_.getEqualLit(q5,2)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q5,3)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q5,4)};
            REQUIRE(conf.creator_.createClause(v));

            v = {~conf.n_.getEqualLit(q5,6)};
            REQUIRE(conf.creator_.createClause(v));

            v = {~conf.n_.getEqualLit(q6,2)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q6,3)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q6,6)};
            REQUIRE(conf.creator_.createClause(v));






            solver.makeRestFalse();
            REQUIRE(conf.n_.prepare());


            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < n; ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="q(1)"; break;
                    case 1: varname="q(2)"; break;
                    case 2: varname="q(3)"; break;
                    case 3: varname="q(4)"; break;
                    case 4: varname="q(5)"; break;
                    case 5: varname="q(6)"; break;
                        //                case 6: varname="q(7)"; break;
                        //                case 7: varname="q(8)"; break;
                        //                case 8: varname="q(9)"; break;
                        //                case 9: varname="q(10)"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }

                st.add("q1==1",(eqs[1-1][1-1]));
                st.add("q1==2",(eqs[1-1][2-1]));
                st.add("q1==3",(eqs[1-1][3-1]));
                st.add("q1==4",(eqs[1-1][4-1]));
                st.add("q1==5",(eqs[1-1][5-1]));
                st.add("q1==6",(eqs[1-1][6-1]));


                st.add("q2==1",(eqs[2-1][1-1]));
                st.add("q2==2",(eqs[2-1][2-1]));
                st.add("q2==3",(eqs[2-1][3-1]));
                st.add("q2==4",(eqs[2-1][4-1]));
                st.add("q2==5",(eqs[2-1][5-1]));
                st.add("q2==6",(eqs[2-1][6-1]));

                st.add("q3==1",(eqs[3-1][1-1]));
                st.add("q3==2",(eqs[3-1][2-1]));
                st.add("q3==3",(eqs[3-1][3-1]));
                st.add("q3==4",(eqs[3-1][4-1]));
                st.add("q3==5",(eqs[3-1][5-1]));
                st.add("q3==6",(eqs[3-1][6-1]));

                st.add("q4==1",(eqs[4-1][1-1]));
                st.add("q4==2",(eqs[4-1][2-1]));
                st.add("q4==3",(eqs[4-1][3-1]));
                st.add("q4==4",(eqs[4-1][4-1]));
                st.add("q4==5",(eqs[4-1][5-1]));
                st.add("q4==6",(eqs[4-1][6-1]));

                st.add("q5==1",(eqs[5-1][1-1]));
                st.add("q5==2",(eqs[5-1][2-1]));
                st.add("q5==3",(eqs[5-1][3-1]));
                st.add("q5==4",(eqs[5-1][4-1]));
                st.add("q5==5",(eqs[5-1][5-1]));
                st.add("q5==6",(eqs[5-1][6-1]));

                st.add("q6==1",(eqs[6-1][1-1]));
                st.add("q6==2",(eqs[6-1][2-1]));
                st.add("q6==3",(eqs[6-1][3-1]));
                st.add("q6==4",(eqs[6-1][4-1]));
                st.add("q6==5",(eqs[6-1][5-1]));
                st.add("q6==6",(eqs[6-1][6-1]));


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==1);
            //TODO: compare with translation based approach
        }
    }


    TEST_CASE("Disjoint2Ex", "1")
    {
        {
            //std::cout << "start SMM3 " << std::endl;
            Config my = lazyDiffSolveConfig;
            my.minLitsPerVar = -1;
            Clasp::ClaspFacade f;
            ClingconConfig conf(f.ctx, my);
            conf.solve.numModels = 999;
            //conf.solve.project =  1;

            Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
            //lp.start(f.ctx);




            REQUIRE(lp.end()); /// UNSAT

            MySharedContext& solver = conf.creator_;


            const int n = 6;

            std::vector<View> views;
            for (int i = 1; i <= n; ++i)
                views.emplace_back(conf.n_.createView(Domain(1,n)));

            std::vector<View> ldiag;
            for (int i = 1; i <= n; ++i)
            {
                /*LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(views[i-1]*1);
                l.addRhs(-i);
                ldiag.emplace_back(conf.n_.createView());
                l.add(ldiag.back()*-1);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));*/
                ldiag.emplace_back(views[i-1].v,1,i);

            }

            std::vector<View> rdiag;
            for (int i = 1; i <= n; ++i)
            {
                /*LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(views[i-1]*1);
                l.addRhs(i);
                rdiag.emplace_back(conf.n_.createView());
                l.add(rdiag.back()*-1);
                conf.n_.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));*/
                rdiag.emplace_back(views[i-1].v,1,-i);
            }



            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
                for (auto i : views)
                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }
            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
                for (auto i : ldiag)
                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }
            {
                std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
                for (auto i : rdiag)
                    disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
                conf.n_.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }

            solver.createNewLiterals(200);
            View q1 = views[0];
            View q2 = views[1];
            View q3 = views[2];
            View q4 = views[3];
            View q5 = views[4];
            View q6 = views[5];
            // exclude sol 1
            LitVec v = {~conf.n_.getEqualLit(q1,2),
                        ~conf.n_.getEqualLit(q2,4),
                        ~conf.n_.getEqualLit(q3,6),
                        ~conf.n_.getEqualLit(q4,1),
                        ~conf.n_.getEqualLit(q5,3),
                        ~conf.n_.getEqualLit(q6,5)};
            // exclude sol 3
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q1,5),
                 ~conf.n_.getEqualLit(q2,3),
                 ~conf.n_.getEqualLit(q3,1),
                 ~conf.n_.getEqualLit(q4,6),
                 ~conf.n_.getEqualLit(q5,4),
                 ~conf.n_.getEqualLit(q6,2)};
            REQUIRE(conf.creator_.createClause(v));

            // enforce sol 2
            v = {conf.n_.getEqualLit(q2,6)};
            REQUIRE(conf.creator_.createClause(v));

            v = {~conf.n_.getEqualLit(q1,5)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q1,6)};
            REQUIRE(conf.creator_.createClause(v));

            v = {~conf.n_.getEqualLit(q3,4)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q3,5)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q3,6)};
            REQUIRE(conf.creator_.createClause(v));

            v = {~conf.n_.getEqualLit(q4,4)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q4,6)};
            REQUIRE(conf.creator_.createClause(v));

            v = {~conf.n_.getEqualLit(q5,2)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q5,3)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q5,4)};
            REQUIRE(conf.creator_.createClause(v));

            v = {~conf.n_.getEqualLit(q5,6)};
            REQUIRE(conf.creator_.createClause(v));

            v = {~conf.n_.getEqualLit(q6,2)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q6,3)};
            REQUIRE(conf.creator_.createClause(v));
            v = {~conf.n_.getEqualLit(q6,6)};
            REQUIRE(conf.creator_.createClause(v));

            solver.makeRestFalse();
            REQUIRE(conf.n_.prepare());


            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                Clasp::OutputTable& st = f.ctx.output;

                for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                {
                    std::string varname;
                    switch(i)
                    {
                    case 0: varname="q(1)"; break;
                    case 1: varname="q(2)"; break;
                    case 2: varname="q(3)"; break;
                    case 3: varname="q(4)"; break;
                    case 4: varname="q(5)"; break;
                    case 5: varname="q(6)"; break;
                        //                case 6: varname="q(7)"; break;
                        //                case 7: varname="q(8)"; break;
                        //                case 8: varname="q(9)"; break;
                        //                case 9: varname="q(10)"; break;
                    }

                    auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                    for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                    {
                        std::stringstream ss;
                        ss << varname << "<=" << *litresit;
                        st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                    }
                }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==1);
            //TODO: compare with translation based approach
        }
    }


    TEST_CASE("Big1", "1")
    {

        //std::cout << "start SMM3 " << std::endl;
        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, lazySolveConfigProp4);
        conf.solve.numModels = 13;

        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;

        unsigned int factor = 1000000;
        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View s = conf.n_.createView(Domain(0,1*factor));
            View e = conf.n_.createView(Domain(0,2*factor));
            View n = conf.n_.createView(Domain(0,3*factor));

            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s*1);
            l.add(e*1);
            l.add(n*-1);
            l.addRhs(0);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));

            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                //            Clasp::OutputTable& st = f.ctx.output;

                //            for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                //            {
                //                std::string varname;
                //                switch(i)
                //                {
                //                case 0: varname="s"; break;
                //                case 1: varname="e"; break;
                //                case 2: varname="n"; break;
                //                default:
                //                {
                //                    std::stringstream ss;
                //                    ss << i;
                //                    varname="V" + ss.str();
                //                    break;
                //                }
                //                }

                //                auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                //                for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                //                {
                //                    std::stringstream ss;
                //                    ss << varname << "<=" << *litresit;
                //                    st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                //                }
                //            }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();


            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was Test4 " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==13);
        }
    }


    TEST_CASE("bigger1", "1")
    {

        //std::cout << "start SMM3 " << std::endl;
        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, lazySolveConfigProp4);
        conf.solve.numModels = 13;

        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        //unsigned int factor = 100000000;
        unsigned int factor = (2147483648 / 3)-1;
        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View s = conf.n_.createView(Domain(0,1*factor));
            View e = conf.n_.createView(Domain(0,2*factor));
            View n = conf.n_.createView(Domain(0,3*factor));




            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s*1);
            l.add(e*1);
            l.add(n*-1);
            l.addRhs(0);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));

            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                //            Clasp::OutputTable& st = f.ctx.output;

                //            for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                //            {
                //                std::string varname;
                //                switch(i)
                //                {
                //                case 0: varname="s"; break;
                //                case 1: varname="e"; break;
                //                case 2: varname="n"; break;
                //                default:
                //                {
                //                    std::stringstream ss;
                //                    ss << i;
                //                    varname="V" + ss.str();
                //                    break;
                //                }
                //                }

                //                auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                //                for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                //                {
                //                    std::stringstream ss;
                //                    ss << varname << "<=" << *litresit;
                //                    st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                //                }
                //            }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();


            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was Test4 " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==13);
        }
    }


    TEST_CASE("Bigger2", "1")
    {

        //std::cout << "start SMM3 " << std::endl;
        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, lazySolveConfigProp4);
        conf.solve.numModels = 13;

        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        //unsigned int factor = 100000000;
        unsigned int factor = Domain::max;
        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View s = conf.n_.createView(Domain(0,factor));
            View e = conf.n_.createView(Domain(0,factor));
            View n = conf.n_.createView(Domain(0,factor));




            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(s);
            l.add(e);
            l.add(n*-3);
            l.addRhs(65536);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));

            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                //            Clasp::OutputTable& st = f.ctx.output;

                //            for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                //            {
                //                std::string varname;
                //                switch(i)
                //                {
                //                case 0: varname="s"; break;
                //                case 1: varname="e"; break;
                //                case 2: varname="n"; break;
                //                default:
                //                {
                //                    std::stringstream ss;
                //                    ss << i;
                //                    varname="V" + ss.str();
                //                    break;
                //                }
                //                }

                //                auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                //                for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                //                {
                //                    std::stringstream ss;
                //                    ss << varname << "<=" << *litresit;
                //                    st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                //                }
                //            }


            }
            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();


            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was Test4 " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==13);
        }
    }

    TEST_CASE("Bigger3", "1")
    {

        //std::cout << "start SMM3 " << std::endl;
        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, lazySolveConfigProp4);
        conf.solve.numModels = 13;

        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        //unsigned int factor = 100000000;
        unsigned int factor = Domain::max;
        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View s = conf.n_.createView(Domain(0,factor));
            View e = conf.n_.createView(Domain(0,factor));
            View n = conf.n_.createView(Domain(0,factor));




            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s*1440);
            l.add(e*6);
            l.add(n*-3);
            l.addRhs(136164);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));

            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                //            Clasp::OutputTable& st = f.ctx.output;

                //            for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                //            {
                //                std::string varname;
                //                switch(i)
                //                {
                //                case 0: varname="s"; break;
                //                case 1: varname="e"; break;
                //                case 2: varname="n"; break;
                //                default:
                //                {
                //                    std::stringstream ss;
                //                    ss << i;
                //                    varname="V" + ss.str();
                //                    break;
                //                }
                //                }

                //                auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                //                for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                //                {
                //                    std::stringstream ss;
                //                    ss << varname << "<=" << *litresit;
                //                    st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                //                }
                //            }


            }
            f.prepare();


            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was Test4 " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==13);
        }
    }


    TEST_CASE("Bigger4", "1")
    {

        //std::cout << "start SMM3 " << std::endl;
        Clasp::ClaspFacade f;
        auto myconf = lazySolveConfigProp4;
        myconf.equalityProcessing=false; /// just takes too long
        /// to do domain prop
        /// on the large variables
        ClingconConfig conf(f.ctx, myconf);
        conf.solve.numModels = 13;

        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;


        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View a = conf.n_.createView(Domain());
            //View b = conf.n_.createView(Domain());
            View c = conf.n_.createView(Domain());




            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a*100);
            //l.add(b*-42100);
            l.add(c*123456);
            l.addRhs(1234560);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));

            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                //            Clasp::OutputTable& st = f.ctx.output;

                //            for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                //            {
                //                std::string varname;
                //                switch(i)
                //                {
                //                case 0: varname="a"; break;
                //                case 1: varname="b"; break;
                //                case 2: varname="c"; break;
                //                default:
                //                {
                //                    std::stringstream ss;
                //                    ss << i;
                //                    varname="V" + ss.str();
                //                    break;
                //                }
                //                }

                //                auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                //                for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                //                {
                //                    std::stringstream ss;
                //                    ss << varname << "<=" << *litresit;
                //                    st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                //                }
                //            }


            }
            f.prepare();


            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was Test4 " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==13);
        }
    }

//    void bigger5()
//    {

//        //std::cout << "start SMM3 " << std::endl;
//        Clasp::ClaspFacade f;
//        ClingconConfig conf(f.ctx, lazySolveConfigProp4);
//        conf.solve.numModels = 13;

//        //conf.solve.project =  1;

//        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
//        //lp.start(f.ctx);

//        std::vector<order::ReifiedLinearConstraint> linearConstraints;


//        REQUIRE(lp.end()); /// UNSAT
//        {

//            MySharedContext& solver = conf.creator_;

//            View a = conf.n_.createView(Domain());
//            View b = conf.n_.createView(Domain());
//            View c = conf.n_.createView(Domain());




//            LinearConstraint l(LinearConstraint::Relation::EQ);
//            l.add(a*1000);
//            l.add(b*-42100);
//            l.add(c*123456);
//            l.addRhs(1234560);
//            //std::cout << std::endl << l << std::endl;
//            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));

//            for (auto &i : linearConstraints)
//                conf.n_.addConstraint(std::move(i));

//            REQUIRE(conf.n_.prepare());

//            REQUIRE(conf.n_.finalize());


//            if (conf.n_.getConfig().minLitsPerVar == -1)
//            {
//                ///name the order lits
//                /// i just use free id's for this.
//                /// in the next incremental step these id's need to be
//                /// made false variables

//                //            Clasp::OutputTable& st = f.ctx.output;

//                //            for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
//                //            {
//                //                std::string varname;
//                //                switch(i)
//                //                {
//                //                case 0: varname="a"; break;
//                //                case 1: varname="b"; break;
//                //                case 2: varname="c"; break;
//                //                default:
//                //                {
//                //                    std::stringstream ss;
//                //                    ss << i;
//                //                    varname="V" + ss.str();
//                //                    break;
//                //                }
//                //                }

//                //                auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
//                //                for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
//                //                {
//                //                    std::stringstream ss;
//                //                    ss << varname << "<=" << *litresit;
//                //                    st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
//                //                }
//                //            }


//            }
//            f.prepare();


//            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
//            //f.solve(&to);
//            f.solve();
//            //std::cout << "This was Test4 " << f.summary().numEnum << " models" << std::endl;
//            REQUIRE(f.summary().numEnum==13);
//        }
//    }


    TEST_CASE("Bigger6", "1")
    {

        //std::cout << "start SMM3 " << std::endl;
        Clasp::ClaspFacade f;
        auto myconf = lazySolveConfigProp4;
        myconf.equalityProcessing=false;
        ClingconConfig conf(f.ctx, myconf);
        conf.solve.numModels = 13;

        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;


        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;

            View a = conf.n_.createView(Domain());
            //View b = conf.n_.createView(Domain());
            View c = conf.n_.createView(Domain());




            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(a*25);
            //l.add(b*-42100);
            l.add(c*30864);
            l.addRhs(308640);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));

            for (auto &i : linearConstraints)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            if (conf.n_.getConfig().minLitsPerVar == -1)
            {
                ///name the order lits
                /// i just use free id's for this.
                /// in the next incremental step these id's need to be
                /// made false variables

                //            Clasp::OutputTable& st = f.ctx.output;

                //            for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
                //            {
                //                std::string varname;
                //                switch(i)
                //                {
                //                case 0: varname="a"; break;
                //                case 1: varname="b"; break;
                //                case 2: varname="c"; break;
                //                default:
                //                {
                //                    std::stringstream ss;
                //                    ss << i;
                //                    varname="V" + ss.str();
                //                    break;
                //                }
                //                }

                //                auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                //                for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                //                {
                //                    std::stringstream ss;
                //                    ss << varname << "<=" << *litresit;
                //                    st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                //                }
                //            }


            }
            f.prepare();


            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "This was Test4 " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==13);
        }
    }


    TEST_CASE("TestEquality1", "1")
    {

        //std::cout << "start test 0 " << std::endl;
        Clasp::ClaspFacade f;
        auto myconf = lazySolveConfigProp4;
        myconf.dlprop=0;
        ClingconConfig conf(f.ctx, myconf);
        conf.solve.numModels = 150;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        MySharedContext& solver = conf.creator_;

        std::vector<order::ReifiedLinearConstraint> linearConstraints;
        std::vector<order::Literal> lits;
        solver.createNewLiterals(16);
        for (unsigned int i = 1; i<= 16; ++i)
            lits.emplace_back(solver.getNewLiteral(true));



        REQUIRE(lp.end()); /// UNSAT



        {
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(conf.n_.createView(Domain(0,9)));
            l.addRhs(9);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[0],Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(conf.n_.createView(Domain(0,9)));
            l.addRhs(7);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[1],Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::GE);
            l.add(conf.n_.createView(Domain(0,9)));
            l.addRhs(0);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[2],Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::GE);
            l.add(conf.n_.createView(Domain(0,9)));
            l.addRhs(-1);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[3],Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(conf.n_.createView(Domain(0,9)));
            l.addRhs(11);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[4],Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(conf.n_.createView(Domain(0,9))*-1);
            l.addRhs(0);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[5],Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::GE);
            l.add(conf.n_.createView(Domain(0,9))*-1);
            l.addRhs(-9);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[6],Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(conf.n_.createView(Domain(0,9))*-1);
            l.addRhs(2);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[7],Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::GE);
            l.add(conf.n_.createView(Domain(0,9))*-1);
            l.addRhs(-11);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[8],Direction::EQ));
        }

        /// false cases


        {
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(conf.n_.createView(Domain(0,9)));
            l.addRhs(-2);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[9],Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::GE);
            l.add(conf.n_.createView(Domain(0,9)));
            l.addRhs(7);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[10],Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::GE);
            l.add(conf.n_.createView(Domain(0,9)));
            l.addRhs(10);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[11],Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::GE);
            l.add(conf.n_.createView(Domain(0,9)));
            l.addRhs(100);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[12],Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(conf.n_.createView(Domain(0,9)));
            l.addRhs(-100);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[13],Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(conf.n_.createView(Domain(0,9))*-1);
            l.addRhs(-10);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[14],Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::GE);
            l.add(conf.n_.createView(Domain(0,9))*-1);
            l.addRhs(1);
            //std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),lits[15],Direction::EQ));
        }


        for (auto &i : linearConstraints)
            conf.n_.addConstraint(std::move(i));

        REQUIRE(conf.n_.prepare());

        REQUIRE(conf.n_.finalize());

        REQUIRE((f.ctx.master()->isTrue(toClaspFormat(lits[0]))));
        REQUIRE((!f.ctx.master()->isTrue(toClaspFormat(lits[1])) && !f.ctx.master()->isFalse(toClaspFormat(lits[1]))));
        REQUIRE((f.ctx.master()->isTrue(toClaspFormat(lits[2]))));
        REQUIRE((f.ctx.master()->isTrue(toClaspFormat(lits[3]))));
        REQUIRE((f.ctx.master()->isTrue(toClaspFormat(lits[4]))));
        REQUIRE((f.ctx.master()->isTrue(toClaspFormat(lits[5]))));
        REQUIRE((f.ctx.master()->isTrue(toClaspFormat(lits[6]))));
        REQUIRE((f.ctx.master()->isTrue(toClaspFormat(lits[7]))));
        REQUIRE((f.ctx.master()->isTrue(toClaspFormat(lits[8]))));

        REQUIRE((f.ctx.master()->isFalse(toClaspFormat(lits[9]))));
        REQUIRE((!f.ctx.master()->isTrue(toClaspFormat(lits[10])) && !f.ctx.master()->isFalse(toClaspFormat(lits[10]))));
        REQUIRE((f.ctx.master()->isFalse(toClaspFormat(lits[11]))));
        REQUIRE((f.ctx.master()->isFalse(toClaspFormat(lits[12]))));
        REQUIRE((f.ctx.master()->isFalse(toClaspFormat(lits[13]))));
        REQUIRE((f.ctx.master()->isFalse(toClaspFormat(lits[14]))));
        REQUIRE((f.ctx.master()->isFalse(toClaspFormat(lits[15]))));


    }


    TEST_CASE("TestEquality2", "1")
    {

        // a = 2*b
        // b = 2*c

        // d = 2*e
        // e = 2*f

        ///merge both
        // c = 2*d
        Clasp::ClaspFacade facade;
        ClingconConfig conf(facade.ctx, lazySolveConfigProp4);
        conf.solve.numModels = 13;

        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = facade.startAsp(conf);
        //lp.start(f.ctx);

        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& s = conf.creator_;

            const int range = std::pow(2,12);
            View a = conf.n_.createView(Domain(-range,range));
            View b = conf.n_.createView(Domain(-range,range));
            View c = conf.n_.createView(Domain(-range,range));
            View d = conf.n_.createView(Domain(-range,range));
            View e = conf.n_.createView(Domain(-range,range));
            View f = conf.n_.createView(Domain(-range,range));

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

            View g = conf.n_.createView(Domain(-range,range));
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

            for (auto &i : lc)
                conf.n_.addConstraint(std::move(i));

            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());
        }
    }




    TEST_CASE("TestPidgeon2", "1")
    {

        auto myconf = lazySolveConfigProp4;
        myconf.pidgeon = true;
        Clasp::ClaspFacade f;
        ClingconConfig conf(f.ctx, myconf);
        conf.solve.numModels = 0;
        //conf.solve.project =  1;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);


        REQUIRE(lp.end()); /// UNSAT
        {

            MySharedContext& solver = conf.creator_;


            View q[10];
            for (unsigned int i = 0; i < 4; ++i)
                q[i] = conf.n_.createView(Domain(1,4));

            conf.n_.addConstraint(ReifiedAllDistinct({q[0],q[1],q[2],q[3]},solver.trueLit(),Direction::EQ));

        }


        REQUIRE(conf.n_.prepare());

        REQUIRE(conf.n_.finalize());


        //f.ctx.startAddConstraints(1000);
        //REQUIRE(conf.n_.createClauses()); /// UNSAT
        f.prepare();
        //std::cout << "constraints: " << f.ctx.stats().numConstraints() << std::endl;
        //std::cout << "variables:   " << f.ctx.stats().vars << std::endl;

        //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
        //f.solve(&to);
        f.solve();
        //std::cout << "conflicts: " << f.ctx.master()->stats.conflicts << std::endl;
        //std::cout << "choices: " << f.ctx.master()->stats.choices << std::endl;
        //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
        REQUIRE(f.summary().numEnum==24);
        //TODO: compare with translation based approach

    }


    TEST_CASE("TestDomain2", "1")
    {

        {
            auto myconf = lazySolveConfigProp4;

            Clasp::ClaspFacade f;
            ClingconConfig conf(f.ctx, myconf);
            conf.solve.numModels = 0;
            //conf.solve.project =  1;

            Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
            //lp.start(f.ctx);


            REQUIRE(lp.end()); /// UNSAT
            {

                MySharedContext& solver = conf.creator_;


                View q = conf.n_.createView(Domain(1,10));

                Domain d(2,6);
                d.remove(Domain(5,5));
                solver.createNewLiterals(1);
                conf.n_.addConstraint(ReifiedDomainConstraint(q,std::move(d),solver.getNewLiteral(true),Direction::EQ));

            }


            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();
            //std::cout << "constraints: " << f.ctx.stats().numConstraints() << std::endl;
            //std::cout << "variables:   " << f.ctx.stats().vars << std::endl;

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "conflicts: " << f.ctx.master()->stats.conflicts << std::endl;
            //std::cout << "choices: " << f.ctx.master()->stats.choices << std::endl;
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==10);

        }

        {
            auto myconf = lazySolveConfigProp4;

            Clasp::ClaspFacade f;
            ClingconConfig conf(f.ctx, myconf);
            conf.solve.numModels = 0;
            //conf.solve.project =  1;

            Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
            //lp.start(f.ctx);


            REQUIRE(lp.end()); /// UNSAT
            {

                MySharedContext& solver = conf.creator_;


                View q = conf.n_.createView(Domain(1,10));

                Domain d(2,6);
                d.remove(Domain(5,5));
                solver.createNewLiterals(1);
                conf.n_.addConstraint(ReifiedDomainConstraint(q,std::move(d),solver.getNewLiteral(true),Direction::FWD));

            }


            REQUIRE(conf.n_.prepare());

            REQUIRE(conf.n_.finalize());


            //f.ctx.startAddConstraints(1000);
            //REQUIRE(conf.n_.createClauses()); /// UNSAT
            f.prepare();
            //std::cout << "constraints: " << f.ctx.stats().numConstraints() << std::endl;
            //std::cout << "variables:   " << f.ctx.stats().vars << std::endl;

            //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
            //f.solve(&to);
            f.solve();
            //std::cout << "conflicts: " << f.ctx.master()->stats.conflicts << std::endl;
            //std::cout << "choices: " << f.ctx.master()->stats.choices << std::endl;
            //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
            REQUIRE(f.summary().numEnum==14);
        }

    }

    void equalDistinct(order::Config c)
    {

        Clasp::ClaspFacade f;
        c.equalityProcessing = true;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT


        MySharedContext& solver = conf.creator_;


        {

            View a = conf.n_.createView(Domain(0,50));
            View b = conf.n_.createView(Domain(0,50));
            View c = conf.n_.createView(Domain(0,50));
            View d = conf.n_.createView(Domain(0,50));



            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(a*5);
                l.addRhs(25);
                //std::cout << std::endl << l << std::endl;
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(a*3);
                l.add(b*-1);
                l.addRhs(-3);
                //std::cout << std::endl << l << std::endl;
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            conf.n_.addConstraint(ReifiedAllDistinct({a,b,c*0,d},solver.trueLit(),Direction::EQ));

        }


        for (auto &i : linearConstraints)
            conf.n_.addConstraint(std::move(i));

        REQUIRE(conf.n_.prepare());

        REQUIRE(conf.n_.finalize());


        if (conf.n_.getConfig().minLitsPerVar == -1)
        {
            ///name the order lits
            /// i just use free id's for this.
            /// in the next incremental step these id's need to be
            /// made false variables

            Clasp::OutputTable& st = f.ctx.output;

            for (std::size_t i = 0; i < conf.n_.getVariableCreator().numVariables(); ++i)
            {
                std::string varname;
                switch(i)
                {
                case 0: varname="q[0]"; break;
                case 1: varname="q[1]"; break;
                case 2: varname="q[2]"; break;
                case 3: varname="q[3]"; break;
                case 4: varname="q[4]"; break;
                case 5: varname="q[5]"; break;
                case 6: varname="q[6]"; break;
                case 7: varname="q[7]"; break;
                case 8: varname="q[8]"; break;
                case 9: varname="q[9]"; break;
                }

                auto lr = conf.n_.getVariableCreator().getRestrictor(View(i));
                for (auto litresit = lr.begin(); litresit != lr.end(); ++litresit )
                {
                    std::stringstream ss;
                    ss << varname << "<=" << *litresit;
                    st.add(ss.str().c_str(),toClaspFormat(conf.n_.getVariableCreator().getLELiteral(litresit)));
                }
            }


        }
        //f.ctx.startAddConstraints(1000);
        //REQUIRE(conf.n_.createClauses()); /// UNSAT
        f.prepare();

        //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
        //f.solve(&to);
        f.solve();
        //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
        REQUIRE(f.summary().numEnum==2448);

    }


    TEST_CASE("EqualDistinct", "1")
    {
        for (auto i : conf1)
            equalDistinct(i);
        //crypt112aux(nonlazySolveConfig);
    }



    void distinctDom(order::Config c)
    {
        Clasp::ClaspFacade f;
        c.equalityProcessing = true;
        ClingconConfig conf(f.ctx, c);
        conf.solve.numModels = 0;

        Clasp::Asp::LogicProgram& lp = f.startAsp(conf);
        //lp.start(f.ctx);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;



        REQUIRE(lp.end()); /// UNSAT


        MySharedContext& solver = conf.creator_;


        {

            View a = conf.n_.createView();
            View b = conf.n_.createView();

            {
                ReifiedDomainConstraint d(b,Domain(10,11),solver.trueLit(),Direction::EQ);
                conf.n_.addConstraint(std::move(d));
            }

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(a*1);
                l.add(b*-5);
                l.addRhs(5);
                //std::cout << std::endl << l << std::endl;
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }


        }


        for (auto &i : linearConstraints)
            conf.n_.addConstraint(std::move(i));

        REQUIRE(conf.n_.prepare());

        REQUIRE(conf.n_.finalize());

        f.prepare();

        //Clasp::Cli::TextOutput to(0,Clasp::Cli::TextOutput::format_asp);
        //f.solve(&to);
        f.solve();
        //std::cout << "This was " << f.summary().numEnum << " models" << std::endl;
        REQUIRE(f.summary().numEnum==2);

    }


    TEST_CASE("DistinctDom", "1")
    {
        for (auto i : conf1)
            distinctDom(i);
    }

