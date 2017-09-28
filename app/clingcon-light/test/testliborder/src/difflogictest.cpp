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

//#include <cppunit/extensions/HelperMacros.h>
//#include "clasp/clasp_facade.h"
//#include "clasp/solver.h"
//#include "clasp/cli/clasp_output.h"
//#include "order/dlpropagator.h"
//#include <memory>
//#include <sstream>
//#include <unordered_set>




//class DiffLogicTest : public CppUnit::TestFixture
//{
//    CPPUNIT_TEST_SUITE( DiffLogicTest );
//    CPPUNIT_TEST( testGraphActivate );

//    CPPUNIT_TEST_SUITE_END();
//private:
//public:
//    void setUp()
//    {
//    }

//    void tearDown()
//    {
//    }


//    bool test(const difflogic::DLPropagator& g, const std::vector<difflogic::DLPropagator::EdgeId>& all, const std::unordered_set<difflogic::DLPropagator::EdgeId>& active) const
//    {
//        for (const auto&i : all)
//        {
//            auto opp = -i;
//            if (active.count(i))
//            {
//                if (!g.isTrue(i))
//                    return false;
//                if (!g.isFalse(opp))
//                    return false;
//            }
//            else
//                if (active.count(opp))
//                {
//                    if (!g.isTrue(opp))
//                        return false;
//                    if (!g.isFalse(i))
//                        return false;
//                }
//                else
//                {
//                    if (!g.isUnknown(i))
//                        return false;
//                }
//        }
//        return true;
//    }



//    void testGraphActivate()
//    {

//        {
//        using namespace difflogic;
//        DLPropagator g;

//        DLPropagator::Variable x = g.newVar();
//        DLPropagator::Variable y = g.newVar();
//        DLPropagator::Variable z = g.newVar();
//        DLPropagator::Variable w = g.newVar();

//        /// this is a cycle free graph
//        std::vector<DLPropagator::EdgeId> edges;
//        std::unordered_set<DLPropagator::EdgeId> active;


//        edges.emplace_back(g.addEdge(y,4,w));//1
//        edges.emplace_back(g.addEdge(w,2,z));//2
//        edges.emplace_back(g.addEdge(z,-3,y));//3
//        edges.emplace_back(g.addEdge(y,5,z));//4
//        edges.emplace_back(g.addEdge(z,-7,x));//5
//        edges.emplace_back(g.addEdge(x,3,y));//6
//        edges.emplace_back(g.addEdge(x,6,w));//7

////        for (const auto& i : edges)
////            g.addEdge(i.in,i.weight,i.out);


//        g.activate(1/*y,4,w*/);
//        active.emplace(1);
//        CPPUNIT_ASSERT(test(g,edges,active));

//        g.activate(2/*w,2,z*/);
//        active.emplace(2);
//        CPPUNIT_ASSERT(test(g,edges,active));

//        g.activate(5/*z,-7,x*/);
//        active.emplace(5);
//        CPPUNIT_ASSERT(test(g,edges,active));


//        g.activate(3/*z,-3,y*/);
//        active.emplace(3);
//        CPPUNIT_ASSERT(test(g,edges,active));


//        g.activate(4/*y,5,z*/);
//        active.emplace(4);
//        CPPUNIT_ASSERT(test(g,edges,active));

//        g.activate(6/*x,3,y*/);
//        active.emplace(6);
//        CPPUNIT_ASSERT(test(g,edges,active));

//        g.activate(7/*x,6,w*/);
//        active.emplace(7);
//        CPPUNIT_ASSERT(test(g,edges,active));

//        CPPUNIT_ASSERT(g.getPotential(x)==-7);
//        CPPUNIT_ASSERT(g.getPotential(y)==-4);
//        CPPUNIT_ASSERT(g.getPotential(z)==0);
//        CPPUNIT_ASSERT(g.getPotential(w)==-1);
//        }

//        {
//        using namespace difflogic;
//        DLPropagator g;

//        DLPropagator::Variable start = g.newVar();
//        DLPropagator::Variable a = g.newVar();
//        DLPropagator::Variable b = g.newVar();
//        DLPropagator::Variable c = g.newVar();
//        DLPropagator::Variable finish = g.newVar();

//        std::vector<DLPropagator::EdgeId> edges;
//        std::unordered_set<DLPropagator::EdgeId> active;


//        edges.emplace_back(g.addEdge(start,1,a));  //1
//        edges.emplace_back(g.addEdge(a,-2,b));     //2
//        edges.emplace_back(g.addEdge(b,-3,c));     //3
//        edges.emplace_back(g.addEdge(c,4,a));      //4
//        edges.emplace_back(g.addEdge(c,3,start));  //5
//        edges.emplace_back(g.addEdge(c,-2,finish));//6



//        g.activate(1);
//        active.emplace(1);
//        CPPUNIT_ASSERT(test(g,edges,active));

//        g.activate(2);
//        active.emplace(2);
//        CPPUNIT_ASSERT(test(g,edges,active));


//        g.activate(3);
//        active.emplace((3));
//        active.emplace((-4));
//        active.emplace((-5));
//        CPPUNIT_ASSERT(test(g,edges,active));

//        CPPUNIT_ASSERT(g.reason((-5))==(std::vector<difflogic::DLPropagator::EdgeId>{(3), (2), (1)}));
//        CPPUNIT_ASSERT(g.reason((-4))==(std::vector<difflogic::DLPropagator::EdgeId>{(3), (2)}));

//        g.activate(6);
//        active.emplace((6));
//        CPPUNIT_ASSERT(test(g,edges,active));

//        CPPUNIT_ASSERT(g.reason((-5))==(std::vector<difflogic::DLPropagator::EdgeId>{(3), (2), (1)}));
//        CPPUNIT_ASSERT(g.reason((-4))==(std::vector<difflogic::DLPropagator::EdgeId>{(3), (2)}));

//        g.undo();
//        active.erase((6));

//        CPPUNIT_ASSERT(g.reason((-5))==(std::vector<difflogic::DLPropagator::EdgeId>{(3), (2), (1)}));
//        CPPUNIT_ASSERT(g.reason((-4))==(std::vector<difflogic::DLPropagator::EdgeId>{(3), (2)}));

//        g.activate(-6);
//        active.emplace((-6));
//        CPPUNIT_ASSERT(test(g,edges,active));

//        CPPUNIT_ASSERT(g.reason((-5))==(std::vector<difflogic::DLPropagator::EdgeId>{(3), (2), (1)}));
//        CPPUNIT_ASSERT(g.reason((-4))==(std::vector<difflogic::DLPropagator::EdgeId>{(3), (2)}));

//        g.undo();
//        active.erase((-6));

//        CPPUNIT_ASSERT(g.reason((-5))==(std::vector<difflogic::DLPropagator::EdgeId>{(3), (2), (1)}));
//        CPPUNIT_ASSERT(g.reason((-4))==(std::vector<difflogic::DLPropagator::EdgeId>{(3), (2)}));

//        g.undo();
//        active.erase((3));
//        active.erase((-4));
//        active.erase((-5));
//        CPPUNIT_ASSERT(test(g,edges,active));


//        }

//        {
//        difflogic::DLPropagator g;

//        difflogic::DLPropagator::Variable x = g.newVar();
//        difflogic::DLPropagator::Variable y = g.newVar();
//        difflogic::DLPropagator::Variable z = g.newVar();
//        difflogic::DLPropagator::Variable w = g.newVar();
//        g.addEdge(y,4,w);
//        g.addEdge(w,2,z);
//        g.addEdge(z,-3,y);
//        g.addEdge(y,5,z);
//        g.addEdge(z,-7,x);
//        g.addEdge(x,3,y);
//        g.addEdge(x,6,w);


//        g.activate(1);
//        g.activate(2);
//        g.activate(3);
//        g.activate(4);
//        g.activate(5);
//        g.activate(6);
//        g.activate(7);








//        CPPUNIT_ASSERT(g.getPotential(x)==-7);
//        CPPUNIT_ASSERT(g.getPotential(y)==-4);
//        CPPUNIT_ASSERT(g.getPotential(z)==0);
//        CPPUNIT_ASSERT(g.getPotential(w)==-1);
//        }

//        {
//        difflogic::DLPropagator g;

//        difflogic::DLPropagator::Variable x = g.newVar();
//        difflogic::DLPropagator::Variable y = g.newVar();
//        difflogic::DLPropagator::Variable z = g.newVar();
//        difflogic::DLPropagator::Variable w = g.newVar();
//        g.addEdge(y,4,w);//1
//        g.addEdge(w,2,z);//2
//        g.addEdge(z,-3,y);//3
//        g.addEdge(y,5,z);//4
//        g.addEdge(z,-7,x);//5
//        g.addEdge(x,3,y);//6
//        g.addEdge(x,6,w);//7


//        g.activate(4);
//        g.activate(5);
//        g.activate(3);
//        g.activate(2);
//        g.activate(1);
//        g.activate(6);
//        g.activate(7);




//        CPPUNIT_ASSERT(g.getPotential(x)==-7);
//        CPPUNIT_ASSERT(g.getPotential(y)==-4);
//        CPPUNIT_ASSERT(g.getPotential(z)==0);
//        CPPUNIT_ASSERT(g.getPotential(w)==-1);
//        }




//        /*TODO: unit test for graph, example from bellman ford wiki

//                GOAL: we want to compute the shortest distance to a virtual q, this is our potential function
//                Then we want to activate edges and update the potential function to do a dijkstra and to find all implications

//                Computing reasons could be troublesome, as we do not have the potential function anymore (in some cases)! because we backtracked
//                WARNING, computing reasons is not always on the decisionlevel of p

//                */

//        CPPUNIT_ASSERT(true);

//    }


//};

//CPPUNIT_TEST_SUITE_REGISTRATION (DiffLogicTest);
