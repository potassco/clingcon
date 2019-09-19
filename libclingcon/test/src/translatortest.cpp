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

#include "clingcon/clingconpropagator.h"
#include "clingcon/configs.h"
#include "clingcon/normalizer.h"
#include "clingcon/solver.h"
#include "test/testapp.h"
#include <iostream>

using namespace clingcon;

clingcon::Config test1 = clingcon::Config(10000, false, 1000, -1, 4);
clingcon::Config test2 = clingcon::Config(100, true, 1000, 10000, 4);
clingcon::Config test3 = clingcon::Config(100, false, 0, 10000, 4);
clingcon::Config test4 = clingcon::Config(100, false, 1, 10000, 4);
std::vector< clingcon::Config > stdconfs = {translateConfig, test1, test2};

/// break symm

// Clasp::Literal toClaspFormat(clingcon::Literal l) { return Clasp::Literal::fromRep(l.asUint()); }
// clingcon::Literal toOrderFormat(Clasp::Literal l) { return
// clingcon::Literal::fromRep(l.asUint()); }

namespace
{

std::size_t expectedModels(Clingo::Control &ctl)
{
    size_t counter = 0;
    for (auto &m : ctl.solve())
    {
        ++counter;
        // std::cout << m << std::endl;
        //          // p.printAssignment(m.thread_id());
    }
    return counter;
}

} // namespace

bool testOutOfRange0(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);
    // Propagator t(s);


    Domain d1;
    d1.remove(Domain::min, Domain::max - 3);
    norm.createView(d1);
    norm.createView(d1);
    norm.createView(d1);


    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());

    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // Now every iterator on end can give back a literal (true for LE, false for GE,EQ)
    // s.printDimacs(std::cout); std::cout << std::endl; // expected 10 solutions
    // std::cout << "NumModels:"  << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 27);
    return true;
}

REGISTER(testOutOfRange0);

bool testOutOfRange1(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);
    // Propagator t(s);


    Domain d1;
    d1.remove(Domain::min, Domain::max - 3);
    View v1 = norm.createView(d1);
    View v2 = norm.createView(d1);
    View v3 = norm.createView(d1);

    LinearConstraint l(LinearConstraint::Relation::GE);
    l.add(v1 * 1);
    l.add(v2 * 1);
    l.add(v3 * 10);
    l.addRhs(0);
    // std::cout << std::endl << l << std::endl;


    norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());

    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // Now every iterator on end can give back a literal (true for LE, false for GE,EQ)
    // s.printDimacs(std::cout); std::cout << std::endl; // expected 10 solutions
    // std::cout << "NumModels:"  << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 27);
    return true;
}

REGISTER(testOutOfRange1);


bool testTranslation0(Clingo::Control &ctl)
{

    {
        Stats stats;
        Grounder s(ctl, stats);
        Normalizer norm(s, translateConfig);
        // Propagator t(s);

        norm.createView(Domain(5, 10));
        norm.createView(Domain(5, 10));
        norm.createView(Domain(5, 10));

        norm.prepare();
        // s.createNewLiterals(norm.estimateVariables());

        REQUIRE(norm.finalize());
        clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(),
                                       norm.getConfig(), nullptr, nullptr, norm.constraints());
        ctl.register_propagator(p);


        //            LitVec clauses = cnfToLits({        -2, 3, 0,
        //                                                -4, 5, 0,
        //                                                -6, 7, 0,
        //                                                4, 7, 0,
        //                                                5, 6, 0,
        //                                                2, 7, 0,
        //                                                2, 4, 6,  0,
        //                                                2, 5,     0,
        //                                                3, 6, 0,
        //                                                3, 4, 0});

        //            REQUIRE(compareClauses(s.clauses(),clauses));
        // std::cout << s.clauses() << std::endl;
        // s.printDimacs(std::cout); std::cout << std::endl; // expected 10 solutions
        // std::cout << "NumModels:"  << expectedModels(ctl) << std::endl;
        REQUIRE(expectedModels(ctl) == 216);
    }
    return true;
}

REGISTER(testTranslation0);


bool testTranslation1(Clingo::Control &ctl)
{

    {
        Stats stats;
        Grounder s(ctl, stats);
        Normalizer norm(s, translateConfig);
        // Propagator t(s);

        View v1 = norm.createView(Domain(5, 10));
        View v2 = norm.createView(Domain(5, 10));
        View v3 = norm.createView(Domain(5, 10));

        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(v1 * 1);
        l.add(v2 * 1);
        l.add(v3 * 1);
        l.addRhs(17);
        // std::cout << std::endl << l << std::endl;


        norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
        norm.prepare();
        // s.createNewLiterals(norm.estimateVariables());

        REQUIRE(norm.finalize());
        clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(),
                                       norm.getConfig(), nullptr, nullptr, norm.constraints());
        ctl.register_propagator(p);


        //            LitVec clauses = cnfToLits({        -2, 3, 0,
        //                                                -4, 5, 0,
        //                                                -6, 7, 0,
        //                                                4, 7, 0,
        //                                                5, 6, 0,
        //                                                2, 7, 0,
        //                                                2, 4, 6,  0,
        //                                                2, 5,     0,
        //                                                3, 6, 0,
        //                                                3, 4, 0});

        //            REQUIRE(compareClauses(s.clauses(),clauses));
        // std::cout << s.clauses() << std::endl;
        // s.printDimacs(std::cout); std::cout << std::endl; // expected 10 solutions
        // std::cout << "NumModels:"  << expectedModels(ctl) << std::endl;
        REQUIRE(expectedModels(ctl) == 10);
    }
    return true;
}

REGISTER(testTranslation1);


bool testNonLazy1(Clingo::Control &ctl)
{

    {
        Stats stats;
        Grounder s(ctl, stats);
        Normalizer norm(s, nonlazySolveConfig);
        // Propagator t(s);

        View v1 = norm.createView(Domain(5, 10));
        View v2 = norm.createView(Domain(5, 10));
        View v3 = norm.createView(Domain(5, 10));

        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(v1 * 1);
        l.add(v2 * 1);
        l.add(v3 * 1);
        l.addRhs(17);
        // std::cout << std::endl << l << std::endl;


        norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
        norm.prepare();
        // s.createNewLiterals(norm.estimateVariables());

        REQUIRE(norm.finalize());
        clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(),
                                       norm.getConfig(), nullptr, nullptr, norm.constraints());
        ctl.register_propagator(p);


        //            LitVec clauses = cnfToLits({        -2, 3, 0,
        //                                                -4, 5, 0,
        //                                                -6, 7, 0,
        //                                                4, 7, 0,
        //                                                5, 6, 0,
        //                                                2, 7, 0,
        //                                                2, 4, 6,  0,
        //                                                2, 5,     0,
        //                                                3, 6, 0,
        //                                                3, 4, 0});

        //            REQUIRE(compareClauses(s.clauses(),clauses));
        // std::cout << s.clauses() << std::endl;
        // s.printDimacs(std::cout); std::cout << std::endl; // expected 10 solutions
        // std::cout << "NumModels:"  << expectedModels(ctl) << std::endl;
        REQUIRE(expectedModels(ctl) == 10);
    }
    return true;
}

REGISTER(testNonLazy1);


bool testTranslation2(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    View v1 = norm.createView(Domain(5, 7));
    View v2 = norm.createView(Domain(5, 7));
    View v3 = norm.createView(Domain(5, 7));
    LinearConstraint l(LinearConstraint::Relation::LE);
    l.add(v1 * 1);
    l.add(v2 * 1);
    l.add(v3 * 1);
    l.addRhs(17);
    // std::cout << std::endl << l << std::endl;

    // l.normalize();
    // l.sort(t);

    norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.createNewLiteral(), Direction::EQ));

    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());


    //            LitVec clauses = cnfToLits({-3, 4, 0,
    //                                        -5, 6, 0,
    //                                        -7, 8, 0,
    //                                        -2, 5, 8, 0,
    //                                        -2, 6, 7, 0,
    //                                        -2, 3, 8, 0,
    //                                        -2, 3, 5, 7, 0,
    //                                        -2, 3, 6, 0,
    //                                        -2, 4, 7, 0,
    //                                        -2, 4, 5, 0,
    //                                        2, -5, -7, 0,
    //                                        2, -4, -6, -7, 0,
    //                                        2, -4, -5, -8, 0,
    //                                        2, -3, -7, 0,
    //                                        2, -3, -6, -8, 0,
    //                                        2, -3, -5, 0

    //                                       });
    //            REQUIRE(compareClauses(s.clauses(),clauses));

    return true;
}

REGISTER(testTranslation2);

bool testTranslation3(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    View v1 = norm.createView(Domain(5, 7));
    View v2 = norm.createView(Domain(5, 7));
    View v3 = norm.createView(Domain(5, 7));
    LinearConstraint l(LinearConstraint::Relation::LE);
    l.add(v1 * 1);
    l.add(v2 * 1);
    l.add(v3 * 1);
    l.addRhs(17);
    // std::cout << std::endl << l << std::endl;

    // l.normalize();
    // l.sort(t);

    norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));

    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    //            LitVec clauses = cnfToLits({
    //                                           -2, 3, 0,
    //                                           -4, 5, 0,
    //                                           -6, 7, 0,
    //                                           4, 7, 0,
    //                                           5, 6, 0,
    //                                           2, 7, 0,
    //                                           2, 4, 6,  0,
    //                                           2, 5,     0,
    //                                           3, 6, 0,
    //                                           3, 4, 0});
    //            REQUIRE(compareClauses(s.clauses(),clauses));
    REQUIRE(expectedModels(ctl) == 10);

    return true;
}

REGISTER(testTranslation3);

bool testTranslation4(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    View v1 = norm.createView(Domain(0, 5));
    View v2 = norm.createView(Domain(0, 3));
    LinearConstraint l(LinearConstraint::Relation::LE);
    l.add(v1 * 3);
    l.add(v2 * 5);
    l.addRhs(14);
    // std::cout << std::endl << l << std::endl;

    // l.normalize();
    // l.sort(t);

    norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));

    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    //            LitVec clauses = cnfToLits({-2, 3, 0,
    //                                        -3, 4, 0,
    //                                        -4, 5, 0,
    //                                        -6, 7, 0,
    //                                        3, 7, 0,
    //                                        5, 6, 0});
    //            REQUIRE(compareClauses(s.clauses(),clauses));
    // s.printDimacs(std::cout); // 11 solutions
    REQUIRE(expectedModels(ctl) == 11);

    return true;
}

REGISTER(testTranslation4);

bool testTranslation5(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    View v1 = norm.createView(Domain(0, 5));
    View v2 = norm.createView(Domain(0, 5));
    View v3 = norm.createView(Domain(0, 5));
    LinearConstraint l(LinearConstraint::Relation::LE);
    l.add(v1 * 1);
    l.add(v2 * 1);
    l.add(v3 * -1);
    l.addRhs(7);
    // std::cout << std::endl << l << std::endl;

    // l.normalize();
    // l.sort(t);

    norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));

    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    //            LitVec clauses = cnfToLits({-2, 3, 0,
    //                                        -3, 4, 0,
    //                                        -4, 5, 0,
    //                                        -5, 6, 0,
    //                                        -7, 8, 0,
    //                                        -8, 9, 0,
    //                                        -9, 10, 0,
    //                                        -10, 11, 0,
    //                                        -12, 13, 0,
    //                                        -13, 14, 0,
    //                                        -14, 15, 0,
    //                                        -15, 16, 0,
    //                                        4, 11, -12, 0,
    //                                        5, 10, -12, 0,
    //                                        5, 11, -13, 0,
    //                                        6, 9, -12, 0,
    //                                        6, 10, -13, 0,
    //                                        6, 11, -14, 0});
    //            REQUIRE(compareClauses(s.clauses(),clauses));
    // s.printDimacs(std::cout); // 206 solutions
    REQUIRE(expectedModels(ctl) == 206);

    return true;
}

REGISTER(testTranslation5);


bool testTranslation6(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    View v1 = norm.createView(Domain(0, 5));
    View v2 = norm.createView(Domain(0, 5));
    View v3 = norm.createView(Domain(0, 5));
    LinearConstraint l(LinearConstraint::Relation::LE);
    l.add(v1 * -1);
    l.add(v2 * -1);
    l.add(v3 * 1);
    l.addRhs(-8);
    // std::cout << std::endl << l << std::endl;

    // l.normalize();
    // l.sort(t);

    norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.createNewLiteral(), Direction::EQ));

    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    //            LitVec clauses = cnfToLits({-3, 4, 0,
    //                                        -4, 5, 0,
    //                                        -5, 6, 0,
    //                                        -6, 7, 0,
    //                                        -8, 9, 0,
    //                                        -9, 10, 0,
    //                                        -10, 11, 0,
    //                                        -11, 12, 0,
    //                                        -13, 14, 0,
    //                                        -14, 15, 0,
    //                                        -15, 16, 0,
    //                                        -16, 17, 0,
    //                                        -2, 15, 0,
    //                                        -2, -12, 14, 0,
    //                                        -2, -11, 13, 0,
    //                                        -2, -10, 0,
    //                                        -2, -7, 14, 0,
    //                                        -2, -7, -12, 13, 0,
    //                                        -2, -7, -11, 0,
    //                                        -2, -6, 13, 0,
    //                                        -2, -6, -12, 0,
    //                                        -2, -5, 0,
    //                                        2, 5, 12, -13, 0,
    //                                        2, 6, 11, -13, 0,
    //                                        2, 6, 12, -14, 0,
    //                                        2, 7, 10, -13, 0,
    //                                        2, 7, 11, -14, 0,
    //                                        2, 7, 12, -15, 0
    //                                       });
    //            REQUIRE(compareClauses(s.clauses(),clauses));


    // s.printDimacs(std::cout); // 216 solutions
    REQUIRE(expectedModels(ctl) == 216);

    return true;
}

REGISTER(testTranslation6);

bool testTranslation7(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    View v1 = norm.createView(Domain(0, 5));
    View v2 = norm.createView(Domain(0, 5));
    View v3 = norm.createView(Domain(0, 5));
    LinearConstraint l(LinearConstraint::Relation::EQ);
    l.add(v1 * 1);
    l.add(v2 * 1);
    l.add(v3 * -1);
    l.addRhs(7);
    // std::cout << std::endl << l << std::endl;

    // l.normalize();
    // l.sort(t);

    norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));

    norm.prepare();

    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    //            LitVec clauses = cnfToLits({-2, 3, 0,
    //                                        -3, 4, 0,
    //                                        -5, 6, 0,
    //                                        -6, 7, 0,
    //                                        -8, 9, 0,
    //                                        -9, 10, 0,
    //                                        2, 7, -8, 0,
    //                                        3, 6, -8, 0,
    //                                        3, 7, -9, 0,
    //                                        4, 5, -8, 0,
    //                                        4, 6, -9, 0,
    //                                        4, 7, -10, 0,
    //                                        -7, 10, 0,
    //                                        -6, 9, 0,
    //                                        -5, 8, 0,
    //                                        -4, 10, 0,
    //                                        -4, -7, 9, 0,
    //                                        -4, -6, 8, 0,
    //                                        -4, -5, 0,
    //                                        -3, 9, 0,
    //                                        -3, -7, 8, 0,
    //                                        -3, -6, 0,
    //                                        -2, 8, 0,
    //                                        -2, -7, 0
    //                                       });
    //            REQUIRE(compareClauses(s.clauses(),clauses));

    REQUIRE(expectedModels(ctl) == 10);

    return true;
}
REGISTER(testTranslation7);


bool testTranslation8(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    View v1 = norm.createView(Domain(0, 5));
    View v2 = norm.createView(Domain(0, 5));
    View v3 = norm.createView(Domain(0, 5));
    LinearConstraint l(LinearConstraint::Relation::LE);
    l.add(v1 * 1);
    l.add(v2 * 1);
    l.add(v3 * -1);
    l.addRhs(7);
    // std::cout << std::endl << l << std::endl;

    // l.normalize();
    // l.sort(t);

    norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));

    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    //            LitVec clauses = cnfToLits({-2, 3, 0,
    //                                        -3, 4, 0,
    //                                        -4, 5, 0,
    //                                        -5, 6, 0,
    //                                        -7, 8, 0,
    //                                        -8, 9, 0,
    //                                        -9, 10, 0,
    //                                        -10, 11, 0,
    //                                        -12, 13, 0,
    //                                        -13, 14, 0,
    //                                        -14, 15, 0,
    //                                        -15, 16, 0,
    //                                        4, 11, -12, 0,
    //                                        5, 10, -12, 0,
    //                                        5, 11, -13, 0,
    //                                        6, 9, -12, 0,
    //                                        6, 10, -13, 0,
    //                                        6, 11, -14, 0
    //                                       });
    //            REQUIRE(compareClauses(s.clauses(),clauses));
    // s.printDimacs(std::cout); // 206
    REQUIRE(expectedModels(ctl) == 206);

    return true;
}

REGISTER(testTranslation8);

bool testTranslation9(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    View v1 = norm.createView(Domain(0, 5));
    View v2 = norm.createView(Domain(0, 5));
    View v3 = norm.createView(Domain(0, 5));
    LinearConstraint l(LinearConstraint::Relation::GE);
    l.add(v1 * 1);
    l.add(v2 * 1);
    l.add(v3 * -1);
    l.addRhs(7);
    // std::cout << std::endl << l << std::endl;

    // l.normalize();
    // l.sort(t);

    norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));

    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    //            LitVec clauses = cnfToLits({-2, 3, 0,
    //                                        -3, 4, 0,
    //                                        -5, 6, 0,
    //                                        -6, 7, 0,
    //                                        -8, 9, 0,
    //                                        -9, 10, 0,
    //                                        -7, 10, 0,
    //                                        -6, 9, 0,
    //                                        -5, 8, 0,
    //                                        -4, 10, 0,
    //                                        -4, -7, 9, 0,
    //                                        -4, -6, 8, 0,
    //                                        -4, -5, 0,
    //                                        -3, 9, 0,
    //                                        -3, -7, 8, 0,
    //                                        -3, -6, 0,
    //                                        -2, 8, 0,
    //                                        -2, -7, 0
    //                                       });
    //            REQUIRE(compareClauses(s.clauses(),clauses));
    // s.printDimacs(std::cout);//20
    REQUIRE(expectedModels(ctl) == 20);
    return true;
}

REGISTER(testTranslation9);

bool testTranslation10(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    View v1 = norm.createView(Domain(0, 5));
    View v2 = norm.createView(Domain(0, 5));
    View v3 = norm.createView(Domain(0, 5));
    LinearConstraint l(LinearConstraint::Relation::EQ);
    l.add(v1 * 1);
    l.add(v2 * 1);
    l.add(v3 * -1);
    l.addRhs(7);
    // std::cout << std::endl << l << std::endl;

    // l.normalize();
    // l.sort(t);

    norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));

    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    //            LitVec clauses = cnfToLits({-2, 3, 0,
    //                                        -3, 4, 0,
    //                                        -5, 6, 0,
    //                                        -6, 7, 0,
    //                                        -8, 9, 0,
    //                                        -9, 10, 0,
    //                                        2, 7, -8, 0,
    //                                        3, 6, -8, 0,
    //                                        3, 7, -9, 0,
    //                                        4, 5, -8, 0,
    //                                        4, 6, -9, 0,
    //                                        4, 7, -10, 0,
    //                                        -7, 10, 0,
    //                                        -6, 9, 0,
    //                                        -5, 8, 0,
    //                                        -4, 10, 0,
    //                                        -4, -7, 9, 0,
    //                                        -4, -6, 8, 0,
    //                                        -4, -5, 0,
    //                                        -3, 9, 0,
    //                                        -3, -7, 8, 0,
    //                                        -3, -6, 0,
    //                                        -2, 8, 0,
    //                                        -2, -7, 0
    //                                       });
    //            REQUIRE(compareClauses(s.clauses(),clauses));

    // s.printDimacs(std::cout);//10
    REQUIRE(expectedModels(ctl) == 10);
    return true;
}


REGISTER(testTranslation10);


bool testTranslation11(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    View v1 = norm.createView(Domain(0, 5));
    View v2 = norm.createView(Domain(0, 5));
    View v3 = norm.createView(Domain(0, 5));
    LinearConstraint l(LinearConstraint::Relation::LE);
    l.add(v1 * 1);
    l.add(v2 * 1);
    l.add(v3 * -1);
    l.addRhs(7);
    // std::cout << std::endl << l << std::endl;

    // l.normalize();
    // l.sort(t);

    norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.createNewLiteral(), Direction::EQ));

    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    //            LitVec clauses = cnfToLits({-3, 4, 0,
    //                                        -4, 5, 0,
    //                                        -5, 6, 0,
    //                                        -6, 7, 0,
    //                                        -8, 9, 0,
    //                                        -9, 10, 0,
    //                                        -10, 11, 0,
    //                                        -11, 12, 0,
    //                                        -13, 14, 0,
    //                                        -14, 15, 0,
    //                                        -15, 16, 0,
    //                                        -16, 17, 0,
    //                                        -2, 5, 12, -13, 0,
    //                                        -2, 6, 11, -13, 0,
    //                                        -2, 6, 12, -14, 0,
    //                                        -2, 7, 10, -13, 0,
    //                                        -2, 7, 11, -14, 0,
    //                                        -2, 7, 12, -15, 0,
    //                                        2, 15, 0,
    //                                        2, -12, 14, 0,
    //                                        2, -11, 13, 0,
    //                                        2, -10, 0,
    //                                        2, -7, 14, 0,
    //                                        2, -7, -12, 13, 0,
    //                                        2, -7, -11, 0,
    //                                        2, -6, 13, 0,
    //                                        2, -6, -12, 0,
    //                                        2, -5, 0,
    //                                       });
    //            REQUIRE(compareClauses(s.clauses(),clauses));
    // s.printDimacs(std::cout); // 216, 206, 10
    REQUIRE(expectedModels(ctl) == 216);
    return true;
}

REGISTER(testTranslation11);

bool testTranslation12(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    View v1 = norm.createView(Domain(0, 5));
    View v2 = norm.createView(Domain(0, 5));
    View v3 = norm.createView(Domain(0, 5));
    LinearConstraint l(LinearConstraint::Relation::GE);
    l.add(v1 * 1);
    l.add(v2 * 1);
    l.add(v3 * -1);
    l.addRhs(7);
    // std::cout << std::endl << l << std::endl;

    // l.normalize();
    // l.sort(t);

    norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.createNewLiteral(), Direction::EQ));

    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    //            LitVec clauses = cnfToLits({-3, 4, 0,
    //                                        -4, 5, 0,
    //                                        -5, 6, 0,
    //                                        -6, 7, 0,
    //                                        -8, 9, 0,
    //                                        -9, 10, 0,
    //                                        -10, 11, 0,
    //                                        -11, 12, 0,
    //                                        -13, 14, 0,
    //                                        -14, 15, 0,
    //                                        -15, 16, 0,
    //                                        -16, 17, 0,
    //                                        -2, 16, 0,
    //                                        -2, -12, 15, 0,
    //                                        -2, -11, 14, 0,
    //                                        -2, -10, 13, 0,
    //                                        -2, -9, 0,
    //                                        -2, -7, 15, 0,
    //                                        -2, -7, -12, 14, 0,
    //                                        -2, -7, -11, 13, 0,
    //                                        -2, -7, -10, 0,
    //                                        -2, -6, 14, 0,
    //                                        -2, -6, -12, 13, 0,
    //                                        -2, -6, -11, 0,
    //                                        -2, -5, 13, 0,
    //                                        -2, -5, -12, 0,
    //                                        -2, -4, 0,
    //                                        2, 4, 12, -13, 0,
    //                                        2, 5, 11, -13, 0,
    //                                        2, 5, 12, -14, 0,
    //                                        2, 6, 10, -13, 0,
    //                                        2, 6, 11, -14, 0,
    //                                        2, 6, 12, -15, 0,
    //                                        2, 7, 9, -13, 0,
    //                                        2, 7, 10, -14, 0,
    //                                        2, 7, 11, -15, 0,
    //                                        2, 7, 12, -16, 0
    //                                       });
    //            REQUIRE(compareClauses(s.clauses(),clauses));
    // s.printDimacs(std::cout);//216, 20, 196
    REQUIRE(expectedModels(ctl) == 216);
    return true;
}

REGISTER(testTranslation12);

bool testTranslation13(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    View v1 = norm.createView(Domain(0, 5));
    View v2 = norm.createView(Domain(0, 5));
    View v3 = norm.createView(Domain(0, 5));
    LinearConstraint l(LinearConstraint::Relation::EQ);
    l.add(v1 * 1);
    l.add(v2 * 1);
    l.add(v3 * -1);
    l.addRhs(7);
    // std::cout << std::endl << l << std::endl;

    // l.normalize();
    // l.sort(t);

    norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.createNewLiteral(), Direction::EQ));

    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    //            LitVec clauses = cnfToLits({-3, -2, 0,
    //                                        -4, -2, 0,
    //                                        -3, -4, 0,
    //                                        3, 4, 2, 0,
    //                                        -5, 6, 0,
    //                                        -6, 7, 0,
    //                                        -7, 8, 0,
    //                                        -8, 9, 0,
    //                                        -10, 11, 0,
    //                                        -11, 12, 0,
    //                                        -12, 13, 0,
    //                                        -13, 14, 0,
    //                                        -15, 16, 0,
    //                                        -16, 17, 0,
    //                                        -17, 18, 0,
    //                                        -18, 19, 0,
    //                                        -2, 7, 14, -15, 0,
    //                                        -2, 8, 13, -15, 0,
    //                                        -2, 8, 14, -16, 0,
    //                                        -2, 9, 12, -15, 0,
    //                                        -2, 9, 13, -16, 0,
    //                                        -2, 9, 14, -17, 0,
    //                                        -2, 18, 0,
    //                                        -2, -14, 17, 0,
    //                                        -2, -13, 16, 0,
    //                                        -2, -12, 15, 0,
    //                                        -2, -11, 0,
    //                                        -2, -9, 17, 0,
    //                                        -2, -9, -14, 16, 0,
    //                                        -2, -9, -13, 15, 0,
    //                                        -2, -9, -12, 0,
    //                                        -2, -8, 16, 0,
    //                                        -2, -8, -14, 15, 0,
    //                                        -2, -8, -13, 0,
    //                                        -2, -7, 15, 0,
    //                                        -2, -7, -14, 0,
    //                                        -2, -6, 0,
    //                                        -3, 6, 14, -15, 0,
    //                                        -3, 7, 13, -15, 0,
    //                                        -3, 7, 14, -16, 0,
    //                                        -3, 8, 12, -15, 0,
    //                                        -3, 8, 13, -16, 0,
    //                                        -3, 8, 14, -17, 0,
    //                                        -3, 9, 11, -15, 0,
    //                                        -3, 9, 12, -16, 0,
    //                                        -3, 9, 13, -17, 0,
    //                                        -3, 9, 14, -18, 0,
    //                                        -4, 17, 0,
    //                                        -4, -14, 16, 0,
    //                                        -4, -13, 15, 0,
    //                                        -4, -12, 0,
    //                                        -4, -9, 16, 0,
    //                                        -4, -9, -14, 15, 0,
    //                                        -4, -9, -13, 0,
    //                                        -4, -8, 15, 0,
    //                                        -4, -8, -14, 0,
    //                                        -4, -7, 0});
    //            REQUIRE(compareClauses(s.clauses(),clauses));

    // s.printDimacs(std::cout); // 216 solutions, 10, 206
    // std::cout << std::endl;
    REQUIRE(expectedModels(ctl) == 216);

    return true;
}

REGISTER(testTranslation13);

bool testTranslation14(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    std::vector< View > vars;
    const int low = 0;
    const int high = 3;
    for (int i = low; i <= high; ++i) vars.emplace_back(norm.createView(Domain(low, high)));
    ReifiedAllDistinct alldiff(std::move(vars), s.trueLit(), Direction::EQ);


    norm.addConstraint(std::move(alldiff));

    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    ////LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // s.printDimacs(std::cout); std::cout << std::endl;//24 solutions (low=0, high=3)
    // std::cout << "NumModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 24);
    return true;
}

REGISTER(testTranslation14);

bool testTranslation145(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    std::vector< View > vars;
    const int low = 0;
    const int high = 2;
    for (int i = low; i <= high; ++i) vars.emplace_back(norm.createView(Domain(low, high)));
    ReifiedAllDistinct alldiff(std::move(vars), s.falseLit(), Direction::EQ);


    norm.addConstraint(std::move(alldiff));

    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    ////LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // s.printDimacs(std::cout); //21 solutions
    // std::cout << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 21);
    return true;
}

REGISTER(testTranslation145);

bool testTranslation15(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, translateConfig);


    std::vector< View > vars;
    const int low = 0;
    const int high = 2;
    for (int i = low; i <= high; ++i) vars.emplace_back(norm.createView(Domain(low, high)));
    ReifiedAllDistinct alldiff(std::move(vars), s.createNewLiteral(), Direction::EQ);


    norm.addConstraint(std::move(alldiff));

    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    ////LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // s.printDimacs(std::cout);  // 27 (6+21)
    REQUIRE(expectedModels(ctl) == 27);
    return true;
}
REGISTER(testTranslation15);

bool testEqual1(Clingo::Control &ctl)

{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);

    /// V0 +9 = V1
    /// V1 domain not given

    LinearConstraint l(LinearConstraint::Relation::EQ);
    l.add(norm.createView(Domain(1, 10)));
    l.addRhs(-9);
    l.add(norm.createView() * -1);
    norm.addConstraint(ReifiedLinearConstraint(std::move(l), solver.trueLit(), Direction::EQ));

    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 10);
    return true;
}

REGISTER(testEqual1);


bool testLess12(Clingo::Control &ctl)
{

    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);


    LinearConstraint l(LinearConstraint::Relation::LE);
    l.add(norm.createView(Domain(0, 9)));
    l.add(norm.createView(Domain(0, 9)));
    l.addRhs(12);
    norm.addConstraint(ReifiedLinearConstraint(std::move(l), solver.trueLit(), Direction::EQ));

    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 79);
    return true;
}

REGISTER(testLess12);


bool sendMoreTest1(Clingo::Control &ctl)
{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    {


        View s = norm.createView(Domain(7, 9));
        View e = norm.createView(Domain(0, 9));

        View m = norm.createView(Domain(1, 2));

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1000);
            l.add(e * 100);
            l.add(m * 1000);
            l.addRhs(9500);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.trueLit(), Direction::EQ));
        }
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));


    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 2);
    return true;
}

REGISTER(sendMoreTest1);


bool sendMoreTest2(Clingo::Control &ctl)
{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);


    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    {


        View d = norm.createView(Domain(0, 4));

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(d * 1);
            l.addRhs(3);
            // std::cout << std::endl << l << std::endl;

            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(d * 1);
            l.addRhs(1);
            // std::cout << std::endl << l << std::endl;

            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));


    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 3);
    return true;
}

REGISTER(sendMoreTest2);


bool sendMoreTest3(Clingo::Control &ctl)
{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);


    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    {
        View s = norm.createView(Domain(0, 9));
        View e = norm.createView(Domain(0, 9));

        View n = norm.createView(Domain(0, 9));

        View d = norm.createView(Domain(0, 9));

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1);
            l.add(e * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1);
            l.add(n * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1);
            l.add(d * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(e * 1);
            l.add(n * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(e * 1);
            l.add(d * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(n * 1);
            l.add(d * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));


    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 5040);
    return true;
}

REGISTER(sendMoreTest3);

bool sendMoreMoneyTranslate(Clingo::Control &ctl)
{
    // std::cout << "sendMoreMoney1" << std::endl;
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);


    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    {

        View s = norm.createView(Domain(0, 9));
        View e = norm.createView(Domain(0, 9));
        View n = norm.createView(Domain(0, 9));
        View d = norm.createView(Domain(0, 9));

        View m = norm.createView(Domain(0, 9));
        View o = norm.createView(Domain(0, 9));
        View r = norm.createView(Domain(0, 9));


        View y = norm.createView(Domain(0, 9));

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1);
            l.addRhs(0);
            // std::cout << std::endl << l << std::endl;

            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(m * 1);
            l.addRhs(0);
            // std::cout << std::endl << l << std::endl;

            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }


        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1000);
            l.add(e * 100);
            l.add(n * 10);
            l.add(d * 1);
            l.add(m * 1000);
            l.add(o * 100);
            l.add(r * 10);
            l.add(e * 1);
            l.add(m * -10000);
            l.add(o * -1000);
            l.add(n * -100);
            l.add(e * -10);
            l.add(y * -1);
            l.addRhs(0);
            // std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.trueLit(), Direction::EQ));
        }


        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1);
            l.add(e * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1);
            l.add(n * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1);
            l.add(d * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1);
            l.add(m * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1);
            l.add(o * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1);
            l.add(r * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1);
            l.add(y * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(e * 1);
            l.add(n * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(e * 1);
            l.add(d * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(e * 1);
            l.add(m * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(e * 1);
            l.add(o * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(e * 1);
            l.add(r * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(e * 1);
            l.add(y * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(n * 1);
            l.add(d * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(n * 1);
            l.add(m * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(n * 1);
            l.add(o * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(n * 1);
            l.add(r * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(n * 1);
            l.add(y * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(d * 1);
            l.add(m * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(d * 1);
            l.add(o * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(d * 1);
            l.add(r * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(d * 1);
            l.add(y * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(m * 1);
            l.add(o * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(m * 1);
            l.add(r * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(m * 1);
            l.add(y * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(o * 1);
            l.add(r * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(o * 1);
            l.add(y * -1);
            l.addRhs(0);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));


    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << std::count(solver.clauses().begin(), solver.clauses().end(),Literal::fromRep(0))
    // << std::endl; std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 1);
    return true;
}

REGISTER(sendMoreMoneyTranslate);

bool sendMoreMoneyTranslate2(Clingo::Control &ctl)
{
    // std::cout << "sendMoreMoney2" << std::endl;
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);


    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    {

        View s = norm.createView(Domain(0, 9));
        View e = norm.createView(Domain(0, 9));
        View n = norm.createView(Domain(0, 9));
        View d = norm.createView(Domain(0, 9));

        View m = norm.createView(Domain(0, 9));
        View o = norm.createView(Domain(0, 9));
        View r = norm.createView(Domain(0, 9));


        View y = norm.createView(Domain(0, 9));

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1);
            l.addRhs(0);
            // std::cout << std::endl << l << std::endl;

            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(m * 1);
            l.addRhs(0);
            // std::cout << std::endl << l << std::endl;

            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }


        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(s * 1000);
            l.add(e * 100);
            l.add(n * 10);
            l.add(d * 1);
            l.add(m * 1000);
            l.add(o * 100);
            l.add(r * 10);
            l.add(e * 1);
            l.add(m * -10000);
            l.add(o * -1000);
            l.add(n * -100);
            l.add(e * -10);
            l.add(y * -1);
            l.addRhs(0);
            // std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.trueLit(), Direction::EQ));
        }

        norm.addConstraint(ReifiedAllDistinct({s, e, n, d, m, o, r, e, m, o, n, e, y},
                                              solver.trueLit(), Direction::EQ));
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));


    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    // solver.printDimacs(std::cout);std::cout << std::endl;


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 1);
    return true;
}

REGISTER(sendMoreMoneyTranslate2);


bool distinctCard(Clingo::Control &ctl)
{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, test2);


    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    {

        View e = norm.createView(Domain(0, 9));
        View i = norm.createView(Domain(0, 9));


        norm.addConstraint(ReifiedAllDistinct({e, i}, solver.trueLit(), Direction::EQ));
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));


    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 90);
    return true;
}

REGISTER(distinctCard);


bool crypt112_aux(Clingo::Control &ctl, clingcon::Config conf)
{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, conf);


    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    {

        View e = norm.createView(Domain(0, 9));
        View i = norm.createView(Domain(0, 9));
        View n = norm.createView(Domain(0, 9));
        View s = norm.createView(Domain(0, 9));

        View z = norm.createView(Domain(0, 9));
        View w = norm.createView(Domain(0, 9));


        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(e * 1000);
            l.add(i * 100);
            l.add(n * 10);
            l.add(s * 1);
            l.add(e * 1000);
            l.add(i * 100);
            l.add(n * 10);
            l.add(s * 1);
            l.add(z * -1000);
            l.add(w * -100);
            l.add(e * -10);
            l.add(i * -1);
            l.addRhs(0);
            // std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.trueLit(), Direction::EQ));
        }

        norm.addConstraint(ReifiedAllDistinct({e, i, n, s, z, w}, solver.trueLit(), Direction::EQ));
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));


    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 12);
    return true;
}


bool crypt112Translate1(Clingo::Control &ctl) { return crypt112_aux(ctl, test1); }

bool crypt112Translate2(Clingo::Control &ctl) { return crypt112_aux(ctl, test2); }

bool crypt112Translate3(Clingo::Control &ctl) { return crypt112_aux(ctl, test3); }

bool crypt112Translate4(Clingo::Control &ctl) { return crypt112_aux(ctl, test4); }

REGISTER(crypt112Translate1);
REGISTER(crypt112Translate2);
REGISTER(crypt112Translate3);
REGISTER(crypt112Translate4);


bool crypt224Translate(Clingo::Control &ctl)
{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);


    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    {

        View z = norm.createView(Domain(0, 9));
        View w = norm.createView(Domain(0, 9));
        View e = norm.createView(Domain(0, 9));
        View i = norm.createView(Domain(0, 9));

        View v = norm.createView(Domain(0, 9));
        View r = norm.createView(Domain(0, 9));


        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(z * 1000);
            l.add(w * 100);
            l.add(e * 10);
            l.add(i * 1);
            l.add(z * 1000);
            l.add(w * 100);
            l.add(e * 10);
            l.add(i * 1);
            l.add(v * -1000);
            l.add(i * -100);
            l.add(e * -10);
            l.add(r * -1);
            l.addRhs(0);
            // std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.trueLit(), Direction::EQ));
        }

        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(z);
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.falseLit(), Direction::EQ));
        }

        norm.addConstraint(ReifiedAllDistinct({z, w, e, i, v, r}, solver.trueLit(), Direction::EQ));
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));


    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 12);
    return true;
}

REGISTER(crypt224Translate);

bool crypt145Translate(Clingo::Control &ctl)
{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    {

        View e = norm.createView();
        View i = norm.createView(Domain(0, 9));
        View n = norm.createView(Domain(0, 9));
        View s = norm.createView(Domain(0, 9));

        View v = norm.createView(Domain(0, 9));
        View r = norm.createView(Domain(0, 9));

        View f = norm.createView(Domain(0, 9));
        View u = norm.createView(Domain(0, 9));


        norm.addConstraint(
            ReifiedDomainConstraint(e, Domain(0, 9), solver.trueLit(), Direction::EQ));
        {
            LinearConstraint l(LinearConstraint::Relation::GE);
            l.add(e * 1);
            l.addRhs(0);
            // std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.trueLit(), Direction::EQ));
        }
        {
            LinearConstraint l(LinearConstraint::Relation::EQ);
            l.add(e * 1000);
            l.add(i * 100);
            l.add(n * 10);
            l.add(s * 1);
            l.add(v * 1000);
            l.add(i * 100);
            l.add(e * 10);
            l.add(r * 1);
            l.add(f * -10000);
            l.add(u * -1000);
            l.add(e * -100);
            l.add(n * -10);
            l.add(f * -1);
            l.addRhs(0);
            // std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.trueLit(), Direction::EQ));
        }

        norm.addConstraint(
            ReifiedAllDistinct({e, i, n, s, v, r, f, u}, solver.trueLit(), Direction::EQ));

        {
            LinearConstraint l(LinearConstraint::Relation::GE);
            l.add(e * 1);
            l.addRhs(4);
            // std::cout << std::endl << l << std::endl;
            linearConstraints.emplace_back(
                ReifiedLinearConstraint(std::move(l), solver.createNewLiteral(), Direction::EQ));
        }
    }


    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));


    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    REQUIRE(expectedModels(ctl) == 24);
    return true;
}
REGISTER(crypt145Translate);

bool allDiffTranslate1(Clingo::Control &ctl)

{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    View e = norm.createView(Domain(0, 0));
    View i = norm.createView(Domain(1, 1));
    View n = norm.createView(Domain(2, 2));
    View s = norm.createView(Domain(3, 3));

    View v = norm.createView(Domain(4, 4));
    View r = norm.createView(Domain(5, 5));

    View f = norm.createView(Domain(6, 6));
    View u = norm.createView(Domain(7, 7));


    norm.addConstraint(
        ReifiedAllDistinct({e, i, n, s, v, r, f, u}, solver.trueLit(), Direction::EQ));


    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 1);
    return true;
}

REGISTER(allDiffTranslate1);


bool allDiffTranslate2(Clingo::Control &ctl)
{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);

    View e = norm.createView(Domain(0, 1));
    View i = norm.createView(Domain(1, 2));

    norm.addConstraint(ReifiedAllDistinct({e, i}, solver.trueLit(), Direction::EQ));

    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 3);
    return true;
}

REGISTER(allDiffTranslate2);

bool allDiffTranslate3(Clingo::Control &ctl)
{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);

    View e = norm.createView(Domain(0, 3));
    View i = norm.createView(Domain(1, 4));
    View n = norm.createView(Domain(2, 5));
    View s = norm.createView(Domain(3, 6));

    View v = norm.createView(Domain(4, 7));
    View r = norm.createView(Domain(5, 8));

    View f = norm.createView(Domain(6, 9));
    View u = norm.createView(Domain(7, 10));


    norm.addConstraint(
        ReifiedAllDistinct({e, i, n, s, v, r, f, u}, solver.trueLit(), Direction::EQ));


    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 2227);
    return true;
}
REGISTER(allDiffTranslate3);


bool domainC1(Clingo::Control &ctl)
{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    View e = norm.createView(Domain(0, 9));

    Domain d(5, 5);
    norm.addConstraint(
        ReifiedDomainConstraint(e, std::move(d), solver.createNewLiteral(), Direction::EQ));


    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 10);
    return true;
}

REGISTER(domainC1);

bool domainC2(Clingo::Control &ctl)
{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    View e = norm.createView(Domain(0, 9));

    Domain d(5, 7);
    d.remove(6, 6);
    norm.addConstraint(
        ReifiedDomainConstraint(e, std::move(d), solver.createNewLiteral(), Direction::EQ));

    {
        LinearConstraint l1(LinearConstraint::Relation::EQ);
        l1.add(e);
        l1.addRhs(7);
        norm.addConstraint(
            ReifiedLinearConstraint(std::move(l1), solver.createNewLiteral(), Direction::EQ));
    }
    {
        LinearConstraint l1(LinearConstraint::Relation::NE);
        l1.add(e);
        l1.addRhs(5);
        norm.addConstraint(
            ReifiedLinearConstraint(std::move(l1), solver.createNewLiteral(), Direction::EQ));
    }
    LinearConstraint l1(LinearConstraint::Relation::NE);
    l1.add(e);
    l1.addRhs(7);
    norm.addConstraint(
        ReifiedLinearConstraint(std::move(l1), solver.createNewLiteral(), Direction::EQ));


    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


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

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 10);
    return true;
}

REGISTER(domainC2);
bool domainC3(Clingo::Control &ctl)
{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);

    View e = norm.createView(Domain(0, 9));

    Domain d(5, 7);
    d.remove(6, 6);
    auto lit = solver.createNewLiteral();
    norm.addConstraint(ReifiedDomainConstraint(e, std::move(d), lit, Direction::EQ));

    {
        LinearConstraint l1(LinearConstraint::Relation::EQ);
        l1.add(e);
        l1.addRhs(7);
        norm.addConstraint(ReifiedLinearConstraint(std::move(l1), -lit, Direction::EQ));
    }

    {
        LinearConstraint l1(LinearConstraint::Relation::EQ);
        l1.add(e);
        l1.addRhs(5);
        norm.addConstraint(ReifiedLinearConstraint(std::move(l1), -lit, Direction::EQ));
    }

    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 0);
    return true;
}

REGISTER(domainC3);

bool domainC4(Clingo::Control &ctl)
{
    Stats stats;
    Grounder solver(ctl, stats);
    Normalizer norm(solver, translateConfig);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    View e = norm.createView(Domain(0, 99));

    Domain d(5, 10);
    d.unify(20, 30);
    d.unify(40, 50);
    norm.addConstraint(
        ReifiedDomainConstraint(e, std::move(d), solver.createNewLiteral(), Direction::EQ));


    REQUIRE(norm.prepare());
    // solver.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());
    clingcon::ClingconPropagator p(stats, solver.trueLit(), norm.getVariableCreator(),
                                   norm.getConfig(), nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);


    // LitVec clauses = cnfToLits({});
    // assert(compareClauses(s.clauses(),clauses));

    // solver.printDimacs(std::cout);std::cout << std::endl;
    // std::cout << "numModels: " << expectedModels(ctl) << std::endl;
    REQUIRE(expectedModels(ctl) == 100);
    return true;
}
REGISTER(domainC4);
