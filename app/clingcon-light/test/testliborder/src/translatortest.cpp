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
#include "order/configs.h"
#include <iostream>

using namespace order;

namespace
{


LitVec cnfToLits(const std::vector<int>& cnf)
{
    LitVec ret;
    ret.reserve(cnf.size());
    for (auto i : cnf)
        ret.emplace_back(Literal(abs(i), i < 0));
    return ret;
}

bool clausesEqual(const LitVec& l1, const LitVec& l2)
{
    if (l1.size()!=l2.size())
        return false;
    for (auto i : l1)
        if (l2.end()==std::find(l2.begin(), l2.end(), i))
            return false;
    return true;
}

bool compareClauses(const LitVec& l1, const LitVec& l2)
{
    if (l1.size()!=l2.size())
    {
        return false;
    }
//    auto it1 = l1.begin();
//    auto it2 = l1.begin();
      std::vector<LitVec> s1;
      s1.emplace_back();
      for (auto i : l1)
      {
          if (i!=Literal::fromRep(0))
            s1.back().emplace_back(i);
          else
          {
               std::sort(s1.back().begin(), s1.back().end());
               s1.emplace_back();
          }
      }
      std::vector<LitVec> s2;
      s2.emplace_back();
      for (auto i : l2)
      {
          if (i!=Literal::fromRep(0))
            s2.back().emplace_back(i);
          else
          {
              std::sort(s2.back().begin(), s2.back().end());
              s2.emplace_back();
          }
      }

      if (s1.size()!=s2.size())
      {
          return false;
      }
      std::sort(s1.begin(),s1.end());
      std::sort(s2.begin(),s2.end());
      auto it1 = s1.begin();
      auto it2 = s2.begin();

      while (it1!=s1.end())
      {
          if (!clausesEqual(*it1,*it2))
          {
              return false;
          }
          ++it1; ++it2;
      }

      return true;
}
}


order::Config test1 = order::Config(false,10000,false,{3,1024},false,false,false,false,true,true,0,-1,-1,true,true, false,true,false, 4,true,std::make_pair(64,true),false);
order::Config test2 = order::Config(true,100,false,{0,10000},false,false,false,false,true,true,0,-1,-1,true,true, false,true,false, 4,true,std::make_pair(64,true),true);
order::Config test3 = order::Config(true,100,false,{1000,10000},false,false,false,false,true,true,0,-1,-1,true,true, false,true,false, 4,true,std::make_pair(64,true),false);
order::Config test4 = order::Config(true,100,false,{3,1024},false,false,false,false,false,true,0,-1,-1,true,true, false,true,false, 4,true,std::make_pair(64,true),true);
std::vector<order::Config> stdconfs = {translateConfig,test1,test2};

///break symm

//Clasp::Literal toClaspFormat(order::Literal l) { return Clasp::Literal::fromRep(l.asUint()); }
//order::Literal toOrderFormat(Clasp::Literal l) { return order::Literal::fromRep(l.asUint()); }


    std::size_t expectedModels(MySolver& s)
    {
        Clasp::ClaspFacade f;
        Clasp::ClaspConfig conf;
        conf.solve.numModels = 0;
        Clasp::SatBuilder& b = f.startSat(conf);
        b.prepareProblem(s.numVars());

        Clasp::LitVec clause;
        for (auto i : s.clauses())
        {
            if (i==Literal::fromRep(0))
            {
                b.addClause(clause);
                clause.clear();
            }
            else
                clause.push_back(Clasp::Literal::fromRep(i.asUint()));
        }
        assert(clause.size()==0);
        clause.push_back(Clasp::Literal(1,false));
        b.addClause(clause);
        b.endProgram();
        f.prepare();
        f.solve();
        return f.summary().numEnum;
    }

    TEST_CASE("testOutOfRange", "translatortest")
    {

        {
            MySolver s;
            Normalizer norm(s,translateConfig);
            //Propagator t(s);


            Domain d1;
            d1.remove(Domain::min, Domain::max-3);
            View v1 = norm.createView(d1);
            View v2 = norm.createView(d1);
            View v3 = norm.createView(d1);

            LinearConstraint l(LinearConstraint::Relation::GE);
            l.add(v1*1);
            l.add(v2*1);
            l.add(v3*10);
            l.addRhs(0);
            //std::cout << std::endl << l << std::endl;


            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());



            //Now every iterator on end can give back a literal (true for LE, false for GE,EQ)
            //s.printDimacs(std::cout); std::cout << std::endl; // expected 10 solutions
            //std::cout << "NumModels:"  << expectedModels(s) << std::endl;
            REQUIRE(expectedModels(s)==27);
        }

        {
            MySolver s;
            Normalizer norm(s,translateConfig);
            //Propagator t(s);


            Domain d1;
            d1.remove(Domain::min, Domain::max-12);
            View v1 = norm.createView(d1);
            View v2 = norm.createView(d1);
            View v3 = norm.createView(d1);
            View v4 = norm.createView(d1);

            LinearConstraint l(LinearConstraint::Relation::GE);
            l.add(v1*1);
            l.add(v2*1);
            l.add(v3*1);
            l.add(v4*1);
            l.addRhs(0);
            //std::cout << std::endl << l << std::endl;


            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));

            try{
            norm.prepare();
            }
            catch(std::runtime_error)
            {

                return;
            }
            REQUIRE(false);
        }
    }

    TEST_CASE("testTranslation1", "translatortest")
    {

        {
            MySolver s;
            Normalizer norm(s,translateConfig);
            //Propagator t(s);

            View v1 = norm.createView(Domain(5,10));
            View v2 = norm.createView(Domain(5,10));
            View v3 = norm.createView(Domain(5,10));

            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(v1*1);
            l.add(v2*1);
            l.add(v3*1);
            l.addRhs(17);
            //std::cout << std::endl << l << std::endl;


            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));
            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({        -2, 3, 0,
                                                -4, 5, 0,
                                                -6, 7, 0,
                                                4, 7, 0,
                                                5, 6, 0,
                                                2, 7, 0,
                                                2, 4, 6,  0,
                                                2, 5,     0,
                                                3, 6, 0,
                                                3, 4, 0});

            REQUIRE(compareClauses(s.clauses(),clauses));
            //std::cout << s.clauses() << std::endl;
            //s.printDimacs(std::cout); std::cout << std::endl; // expected 10 solutions
            //std::cout << "NumModels:"  << expectedModels(s) << std::endl;
            REQUIRE(expectedModels(s)==10);

        }

        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            View v1 = norm.createView(Domain(5,7));
            View v2 = norm.createView(Domain(5,7));
            View v3 = norm.createView(Domain(5,7));
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(v1*1);
            l.add(v2*1);
            l.add(v3*1);
            l.addRhs(17);
            //std::cout << std::endl << l << std::endl;

            //l.normalize();
            //l.sort(t);

            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.getNewLiteral(true),Direction::EQ));

            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());



            LitVec clauses = cnfToLits({-3, 4, 0,
                                        -5, 6, 0,
                                        -7, 8, 0,
                                        -2, 5, 8, 0,
                                        -2, 6, 7, 0,
                                        -2, 3, 8, 0,
                                        -2, 3, 5, 7, 0,
                                        -2, 3, 6, 0,
                                        -2, 4, 7, 0,
                                        -2, 4, 5, 0,
                                        2, -5, -7, 0,
                                        2, -4, -6, -7, 0,
                                        2, -4, -5, -8, 0,
                                        2, -3, -7, 0,
                                        2, -3, -6, -8, 0,
                                        2, -3, -5, 0

                                       });
            REQUIRE(compareClauses(s.clauses(),clauses));

        }

        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            View v1 = norm.createView(Domain(5,7));
            View v2 = norm.createView(Domain(5,7));
            View v3 = norm.createView(Domain(5,7));
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(v1*1);
            l.add(v2*1);
            l.add(v3*1);
            l.addRhs(17);
            //std::cout << std::endl << l << std::endl;

            //l.normalize();
            //l.sort(t);

            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));

            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());




            LitVec clauses = cnfToLits({
                                           -2, 3, 0,
                                           -4, 5, 0,
                                           -6, 7, 0,
                                           4, 7, 0,
                                           5, 6, 0,
                                           2, 7, 0,
                                           2, 4, 6,  0,
                                           2, 5,     0,
                                           3, 6, 0,
                                           3, 4, 0});
            REQUIRE(compareClauses(s.clauses(),clauses));
            REQUIRE(expectedModels(s)==10);

        }

        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            View v1 = norm.createView(Domain(0,5));
            View v2 = norm.createView(Domain(0,3));
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(v1*3);
            l.add(v2*5);
            l.addRhs(14);
            //std::cout << std::endl << l << std::endl;

            //l.normalize();
            //l.sort(t);

            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));

            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());




            LitVec clauses = cnfToLits({-2, 3, 0,
                                        -3, 4, 0,
                                        -4, 5, 0,
                                        -6, 7, 0,
                                        3, 7, 0,
                                        5, 6, 0});
            REQUIRE(compareClauses(s.clauses(),clauses));
            //s.printDimacs(std::cout); // 11 solutions
            REQUIRE(expectedModels(s)==11);

        }

        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            View v1 = norm.createView(Domain(0,5));
            View v2 = norm.createView(Domain(0,5));
            View v3 = norm.createView(Domain(0,5));
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(v1*1);
            l.add(v2*1);
            l.add(v3*-1);
            l.addRhs(7);
            //std::cout << std::endl << l << std::endl;

            //l.normalize();
            //l.sort(t);

            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));

            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({-2, 3, 0,
                                        -3, 4, 0,
                                        -4, 5, 0,
                                        -5, 6, 0,
                                        -7, 8, 0,
                                        -8, 9, 0,
                                        -9, 10, 0,
                                        -10, 11, 0,
                                        -12, 13, 0,
                                        -13, 14, 0,
                                        -14, 15, 0,
                                        -15, 16, 0,
                                        4, 11, -12, 0,
                                        5, 10, -12, 0,
                                        5, 11, -13, 0,
                                        6, 9, -12, 0,
                                        6, 10, -13, 0,
                                        6, 11, -14, 0});
            REQUIRE(compareClauses(s.clauses(),clauses));
            //s.printDimacs(std::cout); // 206 solutions
            REQUIRE(expectedModels(s)==206);

        }


        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            View v1 = norm.createView(Domain(0,5));
            View v2 = norm.createView(Domain(0,5));
            View v3 = norm.createView(Domain(0,5));
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(v1*-1);
            l.add(v2*-1);
            l.add(v3*1);
            l.addRhs(-8);
            //std::cout << std::endl << l << std::endl;

            //l.normalize();
            //l.sort(t);

            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.getNewLiteral(true),Direction::EQ));

            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());



            LitVec clauses = cnfToLits({-3, 4, 0,
                                        -4, 5, 0,
                                        -5, 6, 0,
                                        -6, 7, 0,
                                        -8, 9, 0,
                                        -9, 10, 0,
                                        -10, 11, 0,
                                        -11, 12, 0,
                                        -13, 14, 0,
                                        -14, 15, 0,
                                        -15, 16, 0,
                                        -16, 17, 0,
                                        -2, 15, 0,
                                        -2, -12, 14, 0,
                                        -2, -11, 13, 0,
                                        -2, -10, 0,
                                        -2, -7, 14, 0,
                                        -2, -7, -12, 13, 0,
                                        -2, -7, -11, 0,
                                        -2, -6, 13, 0,
                                        -2, -6, -12, 0,
                                        -2, -5, 0,
                                        2, 5, 12, -13, 0,
                                        2, 6, 11, -13, 0,
                                        2, 6, 12, -14, 0,
                                        2, 7, 10, -13, 0,
                                        2, 7, 11, -14, 0,
                                        2, 7, 12, -15, 0
                                       });
            REQUIRE(compareClauses(s.clauses(),clauses));


            //s.printDimacs(std::cout); // 216 solutions
            REQUIRE(expectedModels(s)==216);

        }


        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            View v1 = norm.createView(Domain(0,5));
            View v2 = norm.createView(Domain(0,5));
            View v3 = norm.createView(Domain(0,5));
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(v1*1);
            l.add(v2*1);
            l.add(v3*-1);
            l.addRhs(7);
            //std::cout << std::endl << l << std::endl;

            //l.normalize();
            //l.sort(t);

            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));

            norm.prepare();

            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());



            LitVec clauses = cnfToLits({-2, 3, 0,
                                        -3, 4, 0,
                                        -5, 6, 0,
                                        -6, 7, 0,
                                        -8, 9, 0,
                                        -9, 10, 0,
                                        2, 7, -8, 0,
                                        3, 6, -8, 0,
                                        3, 7, -9, 0,
                                        4, 5, -8, 0,
                                        4, 6, -9, 0,
                                        4, 7, -10, 0,
                                        -7, 10, 0,
                                        -6, 9, 0,
                                        -5, 8, 0,
                                        -4, 10, 0,
                                        -4, -7, 9, 0,
                                        -4, -6, 8, 0,
                                        -4, -5, 0,
                                        -3, 9, 0,
                                        -3, -7, 8, 0,
                                        -3, -6, 0,
                                        -2, 8, 0,
                                        -2, -7, 0
                                       });
            REQUIRE(compareClauses(s.clauses(),clauses));

            REQUIRE(expectedModels(s)==10);

        }


        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            View v1 = norm.createView(Domain(0,5));
            View v2 = norm.createView(Domain(0,5));
            View v3 = norm.createView(Domain(0,5));
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(v1*1);
            l.add(v2*1);
            l.add(v3*-1);
            l.addRhs(7);
            //std::cout << std::endl << l << std::endl;

            //l.normalize();
            //l.sort(t);

            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));

            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({-2, 3, 0,
                                        -3, 4, 0,
                                        -4, 5, 0,
                                        -5, 6, 0,
                                        -7, 8, 0,
                                        -8, 9, 0,
                                        -9, 10, 0,
                                        -10, 11, 0,
                                        -12, 13, 0,
                                        -13, 14, 0,
                                        -14, 15, 0,
                                        -15, 16, 0,
                                        4, 11, -12, 0,
                                        5, 10, -12, 0,
                                        5, 11, -13, 0,
                                        6, 9, -12, 0,
                                        6, 10, -13, 0,
                                        6, 11, -14, 0
                                       });
            REQUIRE(compareClauses(s.clauses(),clauses));
            //s.printDimacs(std::cout); // 206
            REQUIRE(expectedModels(s)==206);

        }

        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            View v1 = norm.createView(Domain(0,5));
            View v2 = norm.createView(Domain(0,5));
            View v3 = norm.createView(Domain(0,5));
            LinearConstraint l(LinearConstraint::Relation::GE);
            l.add(v1*1);
            l.add(v2*1);
            l.add(v3*-1);
            l.addRhs(7);
            //std::cout << std::endl << l << std::endl;

            //l.normalize();
            //l.sort(t);

            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));

            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({-2, 3, 0,
                                        -3, 4, 0,
                                        -5, 6, 0,
                                        -6, 7, 0,
                                        -8, 9, 0,
                                        -9, 10, 0,
                                        -7, 10, 0,
                                        -6, 9, 0,
                                        -5, 8, 0,
                                        -4, 10, 0,
                                        -4, -7, 9, 0,
                                        -4, -6, 8, 0,
                                        -4, -5, 0,
                                        -3, 9, 0,
                                        -3, -7, 8, 0,
                                        -3, -6, 0,
                                        -2, 8, 0,
                                        -2, -7, 0
                                       });
            REQUIRE(compareClauses(s.clauses(),clauses));
            //s.printDimacs(std::cout);//20
            REQUIRE(expectedModels(s)==20);
        }

        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            View v1 = norm.createView(Domain(0,5));
            View v2 = norm.createView(Domain(0,5));
            View v3 = norm.createView(Domain(0,5));
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(v1*1);
            l.add(v2*1);
            l.add(v3*-1);
            l.addRhs(7);
            //std::cout << std::endl << l << std::endl;

            //l.normalize();
            //l.sort(t);

            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.trueLit(),Direction::EQ));

            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({-2, 3, 0,
                                        -3, 4, 0,
                                        -5, 6, 0,
                                        -6, 7, 0,
                                        -8, 9, 0,
                                        -9, 10, 0,
                                        2, 7, -8, 0,
                                        3, 6, -8, 0,
                                        3, 7, -9, 0,
                                        4, 5, -8, 0,
                                        4, 6, -9, 0,
                                        4, 7, -10, 0,
                                        -7, 10, 0,
                                        -6, 9, 0,
                                        -5, 8, 0,
                                        -4, 10, 0,
                                        -4, -7, 9, 0,
                                        -4, -6, 8, 0,
                                        -4, -5, 0,
                                        -3, 9, 0,
                                        -3, -7, 8, 0,
                                        -3, -6, 0,
                                        -2, 8, 0,
                                        -2, -7, 0
                                       });
            REQUIRE(compareClauses(s.clauses(),clauses));

            //s.printDimacs(std::cout);//10
            REQUIRE(expectedModels(s)==10);
        }






        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            View v1 = norm.createView(Domain(0,5));
            View v2 = norm.createView(Domain(0,5));
            View v3 = norm.createView(Domain(0,5));
            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(v1*1);
            l.add(v2*1);
            l.add(v3*-1);
            l.addRhs(7);
            //std::cout << std::endl << l << std::endl;

            //l.normalize();
            //l.sort(t);

            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.getNewLiteral(true),Direction::EQ));

            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({-3, 4, 0,
                                        -4, 5, 0,
                                        -5, 6, 0,
                                        -6, 7, 0,
                                        -8, 9, 0,
                                        -9, 10, 0,
                                        -10, 11, 0,
                                        -11, 12, 0,
                                        -13, 14, 0,
                                        -14, 15, 0,
                                        -15, 16, 0,
                                        -16, 17, 0,
                                        -2, 5, 12, -13, 0,
                                        -2, 6, 11, -13, 0,
                                        -2, 6, 12, -14, 0,
                                        -2, 7, 10, -13, 0,
                                        -2, 7, 11, -14, 0,
                                        -2, 7, 12, -15, 0,
                                        2, 15, 0,
                                        2, -12, 14, 0,
                                        2, -11, 13, 0,
                                        2, -10, 0,
                                        2, -7, 14, 0,
                                        2, -7, -12, 13, 0,
                                        2, -7, -11, 0,
                                        2, -6, 13, 0,
                                        2, -6, -12, 0,
                                        2, -5, 0,
                                       });
            REQUIRE(compareClauses(s.clauses(),clauses));
            //s.printDimacs(std::cout); // 216, 206, 10
            REQUIRE(expectedModels(s)==216);
        }

        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            View v1 = norm.createView(Domain(0,5));
            View v2 = norm.createView(Domain(0,5));
            View v3 = norm.createView(Domain(0,5));
            LinearConstraint l(LinearConstraint::Relation::GE);
            l.add(v1*1);
            l.add(v2*1);
            l.add(v3*-1);
            l.addRhs(7);
            //std::cout << std::endl << l << std::endl;

            //l.normalize();
            //l.sort(t);

            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.getNewLiteral(true),Direction::EQ));

            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({-3, 4, 0,
                                        -4, 5, 0,
                                        -5, 6, 0,
                                        -6, 7, 0,
                                        -8, 9, 0,
                                        -9, 10, 0,
                                        -10, 11, 0,
                                        -11, 12, 0,
                                        -13, 14, 0,
                                        -14, 15, 0,
                                        -15, 16, 0,
                                        -16, 17, 0,
                                        -2, 16, 0,
                                        -2, -12, 15, 0,
                                        -2, -11, 14, 0,
                                        -2, -10, 13, 0,
                                        -2, -9, 0,
                                        -2, -7, 15, 0,
                                        -2, -7, -12, 14, 0,
                                        -2, -7, -11, 13, 0,
                                        -2, -7, -10, 0,
                                        -2, -6, 14, 0,
                                        -2, -6, -12, 13, 0,
                                        -2, -6, -11, 0,
                                        -2, -5, 13, 0,
                                        -2, -5, -12, 0,
                                        -2, -4, 0,
                                        2, 4, 12, -13, 0,
                                        2, 5, 11, -13, 0,
                                        2, 5, 12, -14, 0,
                                        2, 6, 10, -13, 0,
                                        2, 6, 11, -14, 0,
                                        2, 6, 12, -15, 0,
                                        2, 7, 9, -13, 0,
                                        2, 7, 10, -14, 0,
                                        2, 7, 11, -15, 0,
                                        2, 7, 12, -16, 0
                                       });
            REQUIRE(compareClauses(s.clauses(),clauses));
            //s.printDimacs(std::cout);//216, 20, 196
            REQUIRE(expectedModels(s)==216);
        }

        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            View v1 = norm.createView(Domain(0,5));
            View v2 = norm.createView(Domain(0,5));
            View v3 = norm.createView(Domain(0,5));
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(v1*1);
            l.add(v2*1);
            l.add(v3*-1);
            l.addRhs(7);
            //std::cout << std::endl << l << std::endl;

            //l.normalize();
            //l.sort(t);

            norm.addConstraint(ReifiedLinearConstraint(std::move(l),s.getNewLiteral(true),Direction::EQ));

            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());



            LitVec clauses = cnfToLits({-3, -2, 0,
                                        -4, -2, 0,
                                        -3, -4, 0,
                                        3, 4, 2, 0,
                                        -5, 6, 0,
                                        -6, 7, 0,
                                        -7, 8, 0,
                                        -8, 9, 0,
                                        -10, 11, 0,
                                        -11, 12, 0,
                                        -12, 13, 0,
                                        -13, 14, 0,
                                        -15, 16, 0,
                                        -16, 17, 0,
                                        -17, 18, 0,
                                        -18, 19, 0,
                                        -2, 7, 14, -15, 0,
                                        -2, 8, 13, -15, 0,
                                        -2, 8, 14, -16, 0,
                                        -2, 9, 12, -15, 0,
                                        -2, 9, 13, -16, 0,
                                        -2, 9, 14, -17, 0,
                                        -2, 18, 0,
                                        -2, -14, 17, 0,
                                        -2, -13, 16, 0,
                                        -2, -12, 15, 0,
                                        -2, -11, 0,
                                        -2, -9, 17, 0,
                                        -2, -9, -14, 16, 0,
                                        -2, -9, -13, 15, 0,
                                        -2, -9, -12, 0,
                                        -2, -8, 16, 0,
                                        -2, -8, -14, 15, 0,
                                        -2, -8, -13, 0,
                                        -2, -7, 15, 0,
                                        -2, -7, -14, 0,
                                        -2, -6, 0,
                                        -3, 6, 14, -15, 0,
                                        -3, 7, 13, -15, 0,
                                        -3, 7, 14, -16, 0,
                                        -3, 8, 12, -15, 0,
                                        -3, 8, 13, -16, 0,
                                        -3, 8, 14, -17, 0,
                                        -3, 9, 11, -15, 0,
                                        -3, 9, 12, -16, 0,
                                        -3, 9, 13, -17, 0,
                                        -3, 9, 14, -18, 0,
                                        -4, 17, 0,
                                        -4, -14, 16, 0,
                                        -4, -13, 15, 0,
                                        -4, -12, 0,
                                        -4, -9, 16, 0,
                                        -4, -9, -14, 15, 0,
                                        -4, -9, -13, 0,
                                        -4, -8, 15, 0,
                                        -4, -8, -14, 0,
                                        -4, -7, 0});
            REQUIRE(compareClauses(s.clauses(),clauses));

            //s.printDimacs(std::cout); // 216 solutions, 10, 206
            //std::cout << std::endl;
            REQUIRE(expectedModels(s)==216);

        }

        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            std::vector<View> vars;
            const int low=0;
            const int high=3;
            for (int i = low; i <= high; ++i)
                vars.emplace_back(norm.createView(Domain(low,high)));
            ReifiedAllDistinct alldiff(std::move(vars),s.trueLit(),Direction::EQ);


            norm.addConstraint(std::move(alldiff));

            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());



            LitVec clauses = cnfToLits({});
            //assert(compareClauses(s.clauses(),clauses));

            //s.printDimacs(std::cout); std::cout << std::endl;//24 solutions (low=0, high=3)
            //std::cout << "NumModels: " << expectedModels(s) << std::endl;
            REQUIRE(expectedModels(s)==24);
        }

        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            std::vector<View> vars;
            const int low=0;
            const int high=2;
            for (int i = low; i <= high; ++i)
                vars.emplace_back(norm.createView(Domain(low,high)));
            ReifiedAllDistinct alldiff(std::move(vars),s.falseLit(),Direction::EQ);


            norm.addConstraint(std::move(alldiff));

            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());



            LitVec clauses = cnfToLits({});
            //assert(compareClauses(s.clauses(),clauses));

            //s.printDimacs(std::cout); //21 solutions
            //std::cout << expectedModels(s) << std::endl;
            REQUIRE(expectedModels(s)==21);
        }

        {
            MySolver s;
            Normalizer norm(s, translateConfig);



            std::vector<View> vars;
            const int low=0;
            const int high=2;
            for (int i = low; i <= high; ++i)
                vars.emplace_back(norm.createView(Domain(low,high)));
            ReifiedAllDistinct alldiff(std::move(vars),s.getNewLiteral(true),Direction::EQ);


            norm.addConstraint(std::move(alldiff));

            norm.prepare();
            //s.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());



            LitVec clauses = cnfToLits({});
            //assert(compareClauses(s.clauses(),clauses));

            //s.printDimacs(std::cout);  // 27 (6+21)
            REQUIRE(expectedModels(s)==27);
        }
    }

    TEST_CASE("testEqual1", "translatortest")
    {
        {
            MySolver solver;
            Normalizer norm(solver, translateConfig);

            ///V0 +9 = V1
            /// V1 domain not given

            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(norm.createView(Domain(1,10)));
            l.addRhs(-9);
            l.add(norm.createView()*-1);
            norm.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));

            REQUIRE(norm.prepare());
            //solver.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({});
            //assert(compareClauses(s.clauses(),clauses));

            //solver.printDimacs(std::cout);std::cout << std::endl;
            //std::cout << "numModels: " << expectedModels(solver) << std::endl;
            REQUIRE(expectedModels(solver)==10);
        }

    }

    TEST_CASE("testLess12", "translatortest")
    {
        {
            MySolver solver;
            Normalizer norm(solver, translateConfig);



            LinearConstraint l(LinearConstraint::Relation::LE);
            l.add(norm.createView(Domain(0,9)));
            l.add(norm.createView(Domain(0,9)));
            l.addRhs(12);
            norm.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));

            REQUIRE(norm.prepare());
            //solver.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({});
            //assert(compareClauses(s.clauses(),clauses));

            //solver.printDimacs(std::cout);std::cout << std::endl;
            //std::cout << "numModels: " << expectedModels(solver) << std::endl;
            REQUIRE(expectedModels(solver)==79);
        }

    }



    TEST_CASE("SendMoreTest1", "translatortest")
    {
        MySolver solver;
        Normalizer norm(solver, translateConfig);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;

        {


            View s = norm.createView(Domain(7,9));
            View e = norm.createView(Domain(0,9));

            View m = norm.createView(Domain(1,2));

            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(s*1000);
                l.add(e*100);
                l.add(m*1000);
                l.addRhs(9500);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }



        }

        for (auto &i : linearConstraints)
            norm.addConstraint(std::move(i));


        REQUIRE(norm.prepare());
        //solver.createNewLiterals(norm.estimateVariables());

        REQUIRE(norm.finalize());



        LitVec clauses = cnfToLits({});
        //assert(compareClauses(s.clauses(),clauses));

        //solver.printDimacs(std::cout);std::cout << std::endl;
        //std::cout << "numModels: " << expectedModels(solver) << std::endl;
        REQUIRE(expectedModels(solver)==2);
    }

    TEST_CASE("SendMoreTest2", "translatortest")
    {
        MySolver solver;
        Normalizer norm(solver, translateConfig);


        std::vector<order::ReifiedLinearConstraint> linearConstraints;

        {



            View d = norm.createView(Domain(0,4));

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



        }

        for (auto &i : linearConstraints)
            norm.addConstraint(std::move(i));


        REQUIRE(norm.prepare());
        //solver.createNewLiterals(norm.estimateVariables());

        REQUIRE(norm.finalize());



        LitVec clauses = cnfToLits({});
        //assert(compareClauses(s.clauses(),clauses));

        //solver.printDimacs(std::cout);std::cout << std::endl;
        //std::cout << "numModels: " << expectedModels(solver) << std::endl;
        REQUIRE(expectedModels(solver)==3);
    }


    TEST_CASE("SendMoreTest3", "translatortest")
    {
        MySolver solver;
        Normalizer norm(solver, translateConfig);


        std::vector<order::ReifiedLinearConstraint> linearConstraints;

        {
            View s = norm.createView(Domain(0,9));
            View e = norm.createView(Domain(0,9));

            View n = norm.createView(Domain(0,9));

            View d = norm.createView(Domain(0,9));

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


        }

        for (auto &i : linearConstraints)
            norm.addConstraint(std::move(i));


        REQUIRE(norm.prepare());
        //solver.createNewLiterals(norm.estimateVariables());

        REQUIRE(norm.finalize());



        LitVec clauses = cnfToLits({});
        //assert(compareClauses(s.clauses(),clauses));

        //solver.printDimacs(std::cout);std::cout << std::endl;
        //std::cout << "numModels: " << expectedModels(solver) << std::endl;
        REQUIRE(expectedModels(solver)==5040);
    }

    TEST_CASE("SendMoreMoneyTranslate", "translatortest")
    {
        //std::cout << "sendMoreMoney1" << std::endl;
        MySolver solver;
        Normalizer norm(solver, translateConfig);


        std::vector<order::ReifiedLinearConstraint> linearConstraints;

        {

            View s = norm.createView(Domain(0,9));
            View e = norm.createView(Domain(0,9));
            View n = norm.createView(Domain(0,9));
            View d = norm.createView(Domain(0,9));

            View m = norm.createView(Domain(0,9));
            View o = norm.createView(Domain(0,9));
            View r = norm.createView(Domain(0,9));


            View y = norm.createView(Domain(0,9));

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


        }

        for (auto &i : linearConstraints)
            norm.addConstraint(std::move(i));


        REQUIRE(norm.prepare());
        //solver.createNewLiterals(norm.estimateVariables());

        REQUIRE(norm.finalize());



        LitVec clauses = cnfToLits({});
        //assert(compareClauses(s.clauses(),clauses));

        //solver.printDimacs(std::cout);std::cout << std::endl;
        //std::cout << std::count(solver.clauses().begin(), solver.clauses().end(),Literal::fromRep(0)) << std::endl;
        //std::cout << "numModels: " << expectedModels(solver) << std::endl;
        REQUIRE(expectedModels(solver)==1);
    }

    TEST_CASE("SendMoreMoneyTranslate2", "translatortest")
    {
        //std::cout << "sendMoreMoney2" << std::endl;
        MySolver solver;
        Normalizer norm(solver, translateConfig);


        std::vector<order::ReifiedLinearConstraint> linearConstraints;

        {

            View s = norm.createView(Domain(0,9));
            View e = norm.createView(Domain(0,9));
            View n = norm.createView(Domain(0,9));
            View d = norm.createView(Domain(0,9));

            View m = norm.createView(Domain(0,9));
            View o = norm.createView(Domain(0,9));
            View r = norm.createView(Domain(0,9));


            View y = norm.createView(Domain(0,9));

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

            norm.addConstraint(ReifiedAllDistinct({s,e,n,d,m,o,r,e,m,o,n,e,y},solver.trueLit(),Direction::EQ));

        }

        for (auto &i : linearConstraints)
            norm.addConstraint(std::move(i));



        REQUIRE(norm.prepare());
        //solver.createNewLiterals(norm.estimateVariables());

        REQUIRE(norm.finalize());
        //solver.printDimacs(std::cout);std::cout << std::endl;




        LitVec clauses = cnfToLits({});
        //assert(compareClauses(s.clauses(),clauses));

        //solver.printDimacs(std::cout);std::cout << std::endl;
        //std::cout << "numModels: " << expectedModels(solver) << std::endl;
        REQUIRE(expectedModels(solver)==1);
    }


    TEST_CASE("nQueensTranslate", "translatortest")
    {
        //std::cout << "sendMoreMoney2" << std::endl;
        MySolver solver;
        //conf.disjoint2distinct = false;
        Normalizer norm(solver, translateConfig);


        std::vector<order::ReifiedLinearConstraint> linearConstraints;

        {
            View q[10];
            for (unsigned int i = 0; i < 10; ++i)
                q[i] = norm.createView(Domain(1,10));

            {
            std::vector<std::vector<std::pair<View,ReifiedDNF>>> views;

            for (unsigned int i = 0; i < 10; ++i)
            {
                std::vector<std::vector<order::Literal>> l;
                l.push_back(std::vector<order::Literal>());

                std::vector<std::pair<View,ReifiedDNF>> one;
                one.push_back(std::make_pair(q[i],ReifiedDNF(std::move(l))));
                views.emplace_back(one);

            }
            norm.addConstraint(ReifiedDisjoint(std::move(views),solver.trueLit(),Direction::EQ));
            }

            {
            std::vector<std::vector<std::pair<View,ReifiedDNF>>> views;

            for (unsigned int i = 0; i < 10; ++i)
            {
                std::vector<std::vector<order::Literal>> l;
                l.push_back(std::vector<order::Literal>());

                std::vector<std::pair<View,ReifiedDNF>> one;

                order::LinearConstraint lin(order::LinearConstraint::Relation::EQ);
                lin.addRhs(-i-1);
                lin.add(q[i]*1);
                View b = norm.createView();
                lin.add(b*-1);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(lin),solver.trueLit(),Direction::EQ));


                one.push_back(std::make_pair(b,ReifiedDNF(std::move(l))));
                views.emplace_back(one);
            }
            norm.addConstraint(ReifiedDisjoint(std::move(views),solver.trueLit(),Direction::EQ));
            }


            {
            std::vector<std::vector<std::pair<View,ReifiedDNF>>> views;
            for (unsigned int i = 0; i < 10; ++i)
            {
                std::vector<std::vector<order::Literal>> l;
                l.push_back(std::vector<order::Literal>());

                std::vector<std::pair<View,ReifiedDNF>> one;

                order::LinearConstraint lin(order::LinearConstraint::Relation::EQ);
                lin.addRhs(i+1);
                lin.add(q[i]*1);
                View b = norm.createView();
                lin.add(b*-1);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(lin),solver.trueLit(),Direction::EQ));


                one.push_back(std::make_pair(b,ReifiedDNF(std::move(l))));
                views.emplace_back(one);
            }
            norm.addConstraint(ReifiedDisjoint(std::move(views),solver.trueLit(),Direction::EQ));
            }
            //norm.addConstraint(ReifiedDisjoint({s,e,n,d,m,o,r,e,m,o,n,e,y},solver.trueLit(),false));

        }

        for (auto &i : linearConstraints)
            norm.addConstraint(std::move(i));



        REQUIRE(norm.prepare());
        //solver.createNewLiterals(norm.estimateVariables());

        REQUIRE(norm.finalize());
        //solver.printDimacs(std::cout);std::cout << std::endl;




        LitVec clauses = cnfToLits({});
        //assert(compareClauses(s.clauses(),clauses));

        //solver.printDimacs(std::cout);std::cout << std::endl;
        //std::cout << "numModels: " << expectedModels(solver) << std::endl;
        REQUIRE(expectedModels(solver)==724);
    }

    TEST_CASE("nQueensDirectTranslate", "translatortest")
    {
        //std::cout << "sendMoreMoney2" << std::endl;
        MySolver solver;
        //conf.disjoint2distinct = false;
        Normalizer norm(solver, translateConfig);


        std::vector<order::ReifiedLinearConstraint> linearConstraints;

        {
            View q[10];
            for (unsigned int i = 0; i < 10; ++i)
                q[i] = norm.createView(Domain(1,10));

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
            norm.addConstraint(ReifiedDisjoint(std::move(vars),solver.trueLit(),Direction::EQ));
            }

            {
            std::vector<std::vector<std::pair<View,ReifiedDNF>>> vars;

            for (unsigned int i = 0; i < 10; ++i)
            {
                std::vector<std::vector<order::Literal>> l;
                l.push_back(std::vector<order::Literal>());

                std::vector<std::pair<View,ReifiedDNF>> one;

                q[i].c = i+1;
                one.push_back(std::make_pair(q[i],ReifiedDNF(std::move(l))));
                vars.emplace_back(one);
            }
            norm.addConstraint(ReifiedDisjoint(std::move(vars),solver.trueLit(),Direction::EQ));
            }


            {
            std::vector<std::vector<std::pair<View,ReifiedDNF>>> vars;
            for (unsigned int i = 0; i < 10; ++i)
            {
                std::vector<std::vector<order::Literal>> l;
                l.push_back(std::vector<order::Literal>());

                std::vector<std::pair<View,ReifiedDNF>> one;

                q[i].c= -i-1;
                one.push_back(std::make_pair(q[i],ReifiedDNF(std::move(l))));
                vars.emplace_back(one);
            }
            norm.addConstraint(ReifiedDisjoint(std::move(vars),solver.trueLit(),Direction::EQ));
            }
            //norm.addConstraint(ReifiedDisjoint({s,e,n,d,m,o,r,e,m,o,n,e,y},solver.trueLit(),false));

        }

        for (auto &i : linearConstraints)
            norm.addConstraint(std::move(i));



        REQUIRE(norm.prepare());
        //solver.createNewLiterals(norm.estimateVariables());

        REQUIRE(norm.finalize());
        //solver.printDimacs(std::cout);std::cout << std::endl;




        LitVec clauses = cnfToLits({});
        //assert(compareClauses(s.clauses(),clauses));

        //solver.printDimacs(std::cout);std::cout << std::endl;
        //std::cout << "numModels: " << expectedModels(solver) << std::endl;
        REQUIRE(expectedModels(solver)==724);
    }

    void crypt112_aux(order::Config conf)
    {
        MySolver solver;
        Normalizer norm(solver, conf);


        std::vector<order::ReifiedLinearConstraint> linearConstraints;

        {

            View e = norm.createView(Domain(0,9));
            View i = norm.createView(Domain(0,9));
            View n = norm.createView(Domain(0,9));
            View s = norm.createView(Domain(0,9));

            View z = norm.createView(Domain(0,9));
            View w = norm.createView(Domain(0,9));


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

            norm.addConstraint(ReifiedAllDistinct({e,i,n,s,z,w},solver.trueLit(),Direction::EQ));

        }

        for (auto &i : linearConstraints)
            norm.addConstraint(std::move(i));



        REQUIRE(norm.prepare());
        //solver.createNewLiterals(norm.estimateVariables());

        REQUIRE(norm.finalize());



        LitVec clauses = cnfToLits({});
        //assert(compareClauses(s.clauses(),clauses));

        //solver.printDimacs(std::cout);std::cout << std::endl;
        //std::cout << "numModels: " << expectedModels(solver) << std::endl;
        REQUIRE(expectedModels(solver)==12);
    }

    TEST_CASE("Crypt112Translate", "translatortest")
    {
        for (auto i : stdconfs)
            crypt112_aux(i);
    }



    TEST_CASE("Crypt224Translate", "translatortest")
    {
        MySolver solver;
        Normalizer norm(solver, translateConfig);


        std::vector<order::ReifiedLinearConstraint> linearConstraints;

        {

            View z = norm.createView(Domain(0,9));
            View w = norm.createView(Domain(0,9));
            View e = norm.createView(Domain(0,9));
            View i = norm.createView(Domain(0,9));

            View v = norm.createView(Domain(0,9));
            View r = norm.createView(Domain(0,9));


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
                l.add(z);
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.falseLit(),Direction::EQ));
            }

            norm.addConstraint(ReifiedAllDistinct({z,w,e,i,v,r},solver.trueLit(),Direction::EQ));

        }

        for (auto &i : linearConstraints)
            norm.addConstraint(std::move(i));



        REQUIRE(norm.prepare());
        //solver.createNewLiterals(norm.estimateVariables());

        REQUIRE(norm.finalize());



        LitVec clauses = cnfToLits({});
        //assert(compareClauses(s.clauses(),clauses));

        //solver.printDimacs(std::cout);std::cout << std::endl;
        //std::cout << "numModels: " << expectedModels(solver) << std::endl;
        REQUIRE(expectedModels(solver)==12);
        //REQUIRE(false);
    }

    TEST_CASE("crypt145Translate", "translatortest")
    {
        MySolver solver;
        Normalizer norm(solver, translateConfig);

        std::vector<order::ReifiedLinearConstraint> linearConstraints;

        {

            View e = norm.createView();
            View i = norm.createView(Domain(0,9));
            View n = norm.createView(Domain(0,9));
            View s = norm.createView(Domain(0,9));

            View v = norm.createView(Domain(0,9));
            View r = norm.createView(Domain(0,9));

            View f = norm.createView(Domain(0,9));
            View u = norm.createView(Domain(0,9));


            norm.addConstraint(ReifiedDomainConstraint(e,Domain(0,9),solver.trueLit(),Direction::EQ));
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

            norm.addConstraint(ReifiedAllDistinct({e,i,n,s,v,r,f,u},solver.trueLit(),Direction::EQ));

            {
                LinearConstraint l(LinearConstraint::Relation::GE);
                l.add(e*1);
                l.addRhs(4);
                //std::cout << std::endl << l << std::endl;
                linearConstraints.emplace_back(ReifiedLinearConstraint(std::move(l),solver.getNewLiteral(false),Direction::EQ));
            }

        }



        for (auto &i : linearConstraints)
            norm.addConstraint(std::move(i));


        REQUIRE(norm.prepare());
        //solver.createNewLiterals(norm.estimateVariables());

        REQUIRE(norm.finalize());


        LitVec clauses = cnfToLits({});
        //assert(compareClauses(s.clauses(),clauses));

        //solver.printDimacs(std::cout);std::cout << std::endl;
        REQUIRE(expectedModels(solver)==24);
    }

    TEST_CASE("allDiffTranslate1", "translatortest")
    {
        {
            MySolver solver;
            Normalizer norm(solver, translateConfig);

            std::vector<order::ReifiedLinearConstraint> linearConstraints;

            View e = norm.createView(Domain(0,0));
            View i = norm.createView(Domain(1,1));
            View n = norm.createView(Domain(2,2));
            View s = norm.createView(Domain(3,3));

            View v = norm.createView(Domain(4,4));
            View r = norm.createView(Domain(5,5));

            View f = norm.createView(Domain(6,6));
            View u = norm.createView(Domain(7,7));


            norm.addConstraint(ReifiedAllDistinct({e,i,n,s,v,r,f,u},solver.trueLit(),Direction::EQ));



            REQUIRE(norm.prepare());
            //solver.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({});
            //assert(compareClauses(s.clauses(),clauses));

            //solver.printDimacs(std::cout);std::cout << std::endl;
            //std::cout << "numModels: " << expectedModels(solver) << std::endl;
            REQUIRE(expectedModels(solver)==1);
        }

        {
            MySolver solver;
            Normalizer norm(solver, translateConfig);

            View e = norm.createView(Domain(0,1));
            View i = norm.createView(Domain(1,2));

            norm.addConstraint(ReifiedAllDistinct({e,i},solver.trueLit(),Direction::EQ));

            REQUIRE(norm.prepare());
            //solver.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({});
            //assert(compareClauses(s.clauses(),clauses));

            //solver.printDimacs(std::cout);std::cout << std::endl;
            //std::cout << "numModels: " << expectedModels(solver) << std::endl;
            REQUIRE(expectedModels(solver)==3);
        }

        {
            MySolver solver;
            Normalizer norm(solver, translateConfig);

            View e = norm.createView(Domain(0,3));
            View i = norm.createView(Domain(1,4));
            View n = norm.createView(Domain(2,5));
            View s = norm.createView(Domain(3,6));

            View v = norm.createView(Domain(4,7));
            View r = norm.createView(Domain(5,8));

            View f = norm.createView(Domain(6,9));
            View u = norm.createView(Domain(7,10));


            norm.addConstraint(ReifiedAllDistinct({e,i,n,s,v,r,f,u},solver.trueLit(),Direction::EQ));



            REQUIRE(norm.prepare());
            //solver.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({});
            //assert(compareClauses(s.clauses(),clauses));

            //solver.printDimacs(std::cout);std::cout << std::endl;
            //std::cout << "numModels: " << expectedModels(solver) << std::endl;
            REQUIRE(expectedModels(solver)==2227);
        }
    }

/*
    void alldiff5()
    {
        {

            //test this what the unequal a != b is translated to and if we could speed up using a=5 (direct) atoms
            MySolver solver;
            Normalizer norm(solver, translateConfig);

            std::vector<order::ReifiedLinearConstraint> linearConstraints;

            View s = norm.createView(Domain(0,9));
            View e = norm.createView(Domain(0,9));
            View n = norm.createView(Domain(0,9));
            View d = norm.createView(Domain(0,9));
            View m = norm.createView(Domain(0,9));
            View o = norm.createView(Domain(0,9));
            View r = norm.createView(Domain(0,9));
            View y = norm.createView(Domain(0,9));


            norm.addConstraint(ReifiedAllDistinct({s,e,n,d,m,o,r,y},solver.trueLit(),false));



            REQUIRE(norm.prepare());
            //solver.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({});
            //assert(compareClauses(s.clauses(),clauses));

            //solver.printDimacs(std::cout);std::cout << std::endl;
            //std::cout << "numModels: " << expectedModels(solver) << std::endl;
            REQUIRE(expectedModels(solver)==1814400);
        }

    }

*/
    TEST_CASE("domainC1", "translatortest")
    {
        {
            MySolver solver;
            Normalizer norm(solver, translateConfig);

            std::vector<order::ReifiedLinearConstraint> linearConstraints;

            View e = norm.createView(Domain(0,9));

            Domain d(5,5);
            norm.addConstraint(ReifiedDomainConstraint(e,std::move(d),solver.getNewLiteral(true),Direction::EQ));



            REQUIRE(norm.prepare());
            //solver.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({});
            //assert(compareClauses(s.clauses(),clauses));

            //solver.printDimacs(std::cout);std::cout << std::endl;
            //std::cout << "numModels: " << expectedModels(solver) << std::endl;
            REQUIRE(expectedModels(solver)==10);
        }

        {
            MySolver solver;
            Normalizer norm(solver, translateConfig);

            std::vector<order::ReifiedLinearConstraint> linearConstraints;

            View e = norm.createView(Domain(0,9));

            Domain d(5,7);
            d.remove(6,6);
            norm.addConstraint(ReifiedDomainConstraint(e,std::move(d),solver.getNewLiteral(true),Direction::EQ));

            {
            LinearConstraint l1(LinearConstraint::Relation::EQ);
            l1.add(e);
            l1.addRhs(7);
            norm.addConstraint(ReifiedLinearConstraint(std::move(l1),solver.getNewLiteral(true),Direction::EQ));
            }
            {
            LinearConstraint l1(LinearConstraint::Relation::NE);
            l1.add(e);
            l1.addRhs(5);
            norm.addConstraint(ReifiedLinearConstraint(std::move(l1),solver.getNewLiteral(true),Direction::EQ));
            }
            LinearConstraint l1(LinearConstraint::Relation::NE);
            l1.add(e);
            l1.addRhs(7);
            norm.addConstraint(ReifiedLinearConstraint(std::move(l1),solver.getNewLiteral(true),Direction::EQ));



            REQUIRE(norm.prepare());
            //solver.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


//            LitVec clauses = cnfToLits({-5,-3,0,
//                                        5,3,0,
//                                        -4,3,-2,0,
//                                        4,2,0,
//                                        -3,2,0,
//                                        -6,7,0,
//                                        -7,8,0,
//                                        -8,9,0,
//                                        -9,10,0,
//                                        -10,11,0,
//                                        -11,12,0,
//                                        -12,13,0,
//                                        -13,14,0,
//                                        -4,-11,10,0,
//                                        4,-10,0,
//                                        4,11,0,
//                                        3,-13,12,0,
//                                        -3,-12,0,
//                                        -3,13,0});
//            REQUIRE(solver.clauses()==clauses);

            //solver.printDimacs(std::cout);std::cout << std::endl;
            //std::cout << "numModels: " << expectedModels(solver) << std::endl;
            REQUIRE(expectedModels(solver)==10);
        }

        {
            MySolver solver;
            Normalizer norm(solver, translateConfig);

            std::vector<order::ReifiedLinearConstraint> linearConstraints;

            View e = norm.createView(Domain(0,9));

            Domain d(5,7);
            d.remove(6,6);
            auto lit = solver.getNewLiteral(true);
            norm.addConstraint(ReifiedDomainConstraint(e,std::move(d),lit,Direction::EQ));

            {
            LinearConstraint l1(LinearConstraint::Relation::EQ);
            l1.add(e);
            l1.addRhs(7);
            norm.addConstraint(ReifiedLinearConstraint(std::move(l1),~lit,Direction::EQ));
            }

            {
            LinearConstraint l1(LinearConstraint::Relation::EQ);
            l1.add(e);
            l1.addRhs(5);
            norm.addConstraint(ReifiedLinearConstraint(std::move(l1),~lit,Direction::EQ));
            }





            REQUIRE(norm.prepare());
            //solver.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());



            //solver.printDimacs(std::cout);std::cout << std::endl;
            //std::cout << "numModels: " << expectedModels(solver) << std::endl;
            REQUIRE(expectedModels(solver)==0);
        }


        {
            MySolver solver;
            Normalizer norm(solver, translateConfig);

            std::vector<order::ReifiedLinearConstraint> linearConstraints;

            View e = norm.createView(Domain(0,99));

            Domain d(5,10);
            d.unify(20,30);
            d.unify(40,50);
            norm.addConstraint(ReifiedDomainConstraint(e,std::move(d),solver.getNewLiteral(true),Direction::EQ));



            REQUIRE(norm.prepare());
            //solver.createNewLiterals(norm.estimateVariables());

            REQUIRE(norm.finalize());


            LitVec clauses = cnfToLits({});
            //assert(compareClauses(s.clauses(),clauses));

            //solver.printDimacs(std::cout);std::cout << std::endl;
            //std::cout << "numModels: " << expectedModels(solver) << std::endl;
            REQUIRE(expectedModels(solver)==100);
        }
    }
    void testLiteralReuse()
    {
        /*MySolver s;
                    Normalizer norm(s);

                    Domain dom(1,100);

                    dom.remove(30,39);


                    Variable a = norm.createVariable(dom);
                    //norm.getVariableCreator().createOrderLiterals();
                    {
                    LinearConstraint l(LinearConstraint::Relation::LE);
                    l.add(a,5);
                    l.addRhs(25);
                    Literal l1 = norm.addReifiedLinearConstraint(std::move(l));
                    assert(l1.var()==5+1);
                    assert(l1.sign()==false);
                    }

                    {
                    LinearConstraint l(LinearConstraint::Relation::LT);
                    l.add(a,5);
                    l.addRhs(30);
                    Literal l1 = norm.addReifiedLinearConstraint(std::move(l));
                    assert(l1.var()==5+1);
                    assert(l1.sign()==false);
                    }

                    {
                    LinearConstraint l(LinearConstraint::Relation::LE);
                    l.add(a,1);
                    l.addRhs(36);
                    Literal l1 = norm.addReifiedLinearConstraint(std::move(l));
                    assert(l1.var()==29+1);
                    assert(l1.sign()==false);
                    }

                    {
                    LinearConstraint l(LinearConstraint::Relation::LE);
                    l.add(a,10);
                    l.addRhs(358);
                    Literal l1 = norm.addReifiedLinearConstraint(std::move(l));
                    assert(l1.var()==29+1);
                    assert(l1.sign()==false);
                    }

                    {
                    LinearConstraint l(LinearConstraint::Relation::LE);
                    l.add(a,-10);
                    l.addRhs(-358);
                    Literal l1 = norm.addReifiedLinearConstraint(std::move(l));
                    assert(l1.var()==29+1);
                    assert(l1.sign()==true);
                    }



                    {
                    LinearConstraint l(LinearConstraint::Relation::GE);
                    l.add(a,1);
                    l.addRhs(5);
                    Literal l1 = norm.addReifiedLinearConstraint(std::move(l));
                    assert(l1.var()==4+1);
                    assert(l1.sign()==true);
                    }

                    {
                    LinearConstraint l(LinearConstraint::Relation::GT);
                    l.add(a,1);
                    l.addRhs(4);
                    Literal l1 = norm.addReifiedLinearConstraint(std::move(l));
                    assert(l1.var()==4+1);
                    assert(l1.sign()==true);
                    }


                    {
                    LinearConstraint l(LinearConstraint::Relation::GE);
                    l.add(a,-10);
                    l.addRhs(-358);
                    Literal l1 = norm.addReifiedLinearConstraint(std::move(l));
                    assert(l1.var()==30+1);
                    assert(l1.sign()==false);
                    }*/
    }

    TEST_CASE("testAllDiffSplitting", "translatortest")
    {

        MySolver s;
        Normalizer norm(s, translateConfig);



        std::vector<View> views;
        const int low=0;
        const int high=49;
        for (int i = low; i <= high; ++i)
            views.emplace_back(norm.createView(Domain(low,high)));
        ReifiedAllDistinct alldiff(std::move(views),s.falseLit(),Direction::EQ);

        norm.addConstraint(std::move(alldiff));

        norm.prepare();
        //s.createNewLiterals(norm.estimateVariables());

        REQUIRE(norm.finalize());



        //LitVec clauses = cnfToLits({});
        //assert(compareClauses(s.clauses(),clauses));

        //s.printDimacs(std::cout); //21 solutions
        //assert(expectedModels(s)==21);

    }


    TEST_CASE("disjoint1Translate", "translatortest")
    {
        {
            MySolver solver;
            Normalizer norm(solver, translateConfig);

            const int n = 10;


            std::vector<View> views;
            for (int i = 1; i < 10; ++i)
                views.emplace_back(norm.createView(Domain(1,n)));

            std::vector<View> ldiag;
            for (int i = 1; i < 10; ++i)
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(views[i-1]*1);
                l.addRhs(-i);
                ldiag.emplace_back(norm.createView());
                l.add(ldiag.back()*-1);
                norm.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }

            ///WTF, this is the same as ldiag, where is the difference!
            std::vector<View> rdiag;
            for (int i = 1; i < 10; ++i)
            {
                LinearConstraint l(LinearConstraint::Relation::EQ);
                l.add(views[i-1]*1);
                l.addRhs(-i);
                rdiag.emplace_back(norm.createView());
                l.add(rdiag.back()*-1);
                norm.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),Direction::EQ));
            }



            {
            std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
            for (auto i : views)
                disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
            norm.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }
            {
            std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
            for (auto i : ldiag)
                disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
            norm.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }
            {
            std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
            for (auto i : rdiag)
                disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
            norm.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }



            REQUIRE(norm.prepare());
            //solver.createNewLiterals(norm.estimateVariables());
            /// I can not create cardinality constraints for sat solver
            /*
            REQUIRE(norm.createClauses());

            translate(solver, norm.getVariableCreator(), norm.removeConstraints(), norm.getConfig());
            LitVec clauses = cnfToLits({});
            //assert(compareClauses(s.clauses(),clauses));

            //solver.printDimacs(std::cout);std::cout << std::endl;
            //std::cout << "numModels: " << expectedModels(solver) << std::endl;
            REQUIRE(expectedModels(solver)==724);*/
        }

    }

    TEST_CASE("disjoint1DirectTranslate", "translatortest")
    {
        {
            MySolver solver;
            Normalizer norm(solver, translateConfig);

            const int n = 10;


            std::vector<View> views;
            for (int i = 1; i < 10; ++i)
                views.emplace_back(norm.createView(Domain(1,n)));

            std::vector<View> ldiag;
            for (int i = 1; i < 10; ++i)
            {
                ldiag.emplace_back(views[i-1].v,1,i);
//                LinearConstraint l(LinearConstraint::Relation::EQ);
//                l.add(views[i-1],1);
//                l.addRhs(-i);
//                ldiag.emplace_back(norm.createView());
//                l.add(ldiag.back(),-1);
//                norm.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),false));
            }

            std::vector<View> rdiag;
            for (int i = 1; i < 10; ++i)
            {
                rdiag.emplace_back(views[i-1].v,1,i);
//                LinearConstraint l(LinearConstraint::Relation::EQ);
//                l.add(views[i-1],1);
//                l.addRhs(-i);
//                rdiag.emplace_back(norm.createView());
//                l.add(rdiag.back(),-1);
//                norm.addConstraint(ReifiedLinearConstraint(std::move(l),solver.trueLit(),false));
            }



            {
            std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
            for (auto i : views)
                disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
            norm.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }
            {
            std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
            for (auto i : ldiag)
                disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
            norm.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }
            {
            std::vector<std::vector<std::pair<View,ReifiedDNF>>> disj;
            for (auto i : rdiag)
                disj.emplace_back(std::vector<std::pair<View,ReifiedDNF>>{std::make_pair(i,ReifiedDNF(std::vector<std::vector<Literal>>{std::vector<Literal>()}))});
            norm.addConstraint(ReifiedDisjoint(std::move(disj),solver.trueLit(),Direction::EQ));
            }



            REQUIRE(norm.prepare());
            //solver.createNewLiterals(norm.estimateVariables());
            /// I can not create cardinality constraints for sat solver
            /*
            REQUIRE(norm.createClauses());

            translate(solver, norm.getVariableCreator(), norm.removeConstraints(), norm.getConfig());
            LitVec clauses = cnfToLits({});
            //assert(compareClauses(s.clauses(),clauses));

            //solver.printDimacs(std::cout);std::cout << std::endl;
            //std::cout << "numModels: " << expectedModels(solver) << std::endl;
            REQUIRE(expectedModels(solver)==724);*/
        }

    }




