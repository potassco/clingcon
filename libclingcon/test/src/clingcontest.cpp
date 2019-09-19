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
#include <memory>
#include <sstream>


#include <algorithm>
#include <chrono>
#include <ctime>


using namespace clingcon;

#define REGISTERCONF(x)                                                                            \
    bool call_0##x(Clingo::Control &ctl) { return x(ctl, conf1[0]); }                              \
    bool call_1##x(Clingo::Control &ctl) { return x(ctl, conf1[1]); }                              \
    bool call_2##x(Clingo::Control &ctl) { return x(ctl, conf1[2]); }                              \
    bool call_3##x(Clingo::Control &ctl) { return x(ctl, conf1[3]); }                              \
    bool call_4##x(Clingo::Control &ctl) { return x(ctl, conf1[4]); }                              \
    REGISTER(call_0##x);                                                                           \
    REGISTER(call_1##x);                                                                           \
    REGISTER(call_2##x);                                                                           \
    REGISTER(call_3##x);                                                                           \
    REGISTER(call_4##x)

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

bool expectedModelsMin(Clingo::Control &ctl, size_t num)
{
    size_t counter = 0;
    for (auto &m : ctl.solve())
    {
        ++counter;
        if (counter == num) return true;
        // std::cout << m << std::endl;
        //          // p.printAssignment(m.thread_id());
    }
    return false;
}
} // namespace
bool empty(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, lazySolveConfigProp4);
    // Propagator t(s);


    norm.prepare();
    // s.createNewLiterals(norm.estimateVariables());

    REQUIRE(norm.finalize());

    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 1);
    return true;
}

REGISTER(empty);


bool simple_logic(Clingo::Control &ctl)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, lazySolveConfigProp4);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    View ss = norm.createView(Domain(0, 9));
    View e = norm.createView(Domain(0, 9));


    {
        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(ss);
        l.add(e);
        l.addRhs(12);
        // std::cout << std::endl << l << std::endl;
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    norm.prepare();


    REQUIRE(norm.finalize());

    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);

    REQUIRE(expectedModels(ctl) == 79);
    return true;
}

REGISTER(simple_logic);

bool test1aux(Clingo::Control &ctl, clingcon::Config c)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    /// {a,b}.
    /// a :- b.
    /// :- a, b, not a+b+c <= 17.
    /// :- not b.
    Clingo::Backend backend = ctl.backend();
    auto a = backend.add_atom();
    auto b = backend.add_atom();

    {
        backend.rule(true, {a, b}, {});
    }

    {
        backend.rule(false, {a}, {int(b)});
    }


    auto constraint1 = backend.add_atom();
    {
        backend.rule(false, {}, {int(a), int(b), -int(constraint1)});
    }

    {
        backend.rule(true, {constraint1}, {});
    }


    auto ia = norm.createView(clingcon::Domain(5, 10));
    auto ib = norm.createView(clingcon::Domain(5, 10));
    auto ic = norm.createView(clingcon::Domain(5, 10));
    clingcon::LinearConstraint l(clingcon::LinearConstraint::Relation::LE);
    l.addRhs(17);
    l.add(ia);
    l.add(ib);
    l.add(ic);
    //// at least getting the literal from the View has to take place after lp.end() has been called
    linearConstraints.emplace_back(
        clingcon::ReifiedLinearConstraint(std::move(l), constraint1, Direction::EQ));
    ////
    // f.ctx.unfreeze();
    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());

    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);

    REQUIRE(expectedModels(ctl) == 442);
    return true;
}

REGISTERCONF(test1aux);

bool test3aux(Clingo::Control &ctl, clingcon::Config c)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    View ss = norm.createView(Domain(7, 9));
    View e = norm.createView(Domain(0, 9));
    //       View n = norm.createView(Domain(0,9));
    //      View d = norm.createView(Domain(0,9));

    View m = norm.createView(Domain(1, 2));


    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1000);
        l.add(e * 100);
        //        l.add(n,10);
        //      l.add(d,1);
        l.add(m * 1000);

        l.addRhs(9500);
        // std::cout << std::endl << l << std::endl;
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 2);
    return true;
}

REGISTERCONF(test3aux);


bool test4aux(Clingo::Control &ctl, clingcon::Config c)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    //       View ss = norm.createView(Domain(0,9));
    //       View e = norm.createView(Domain(0,9));

    View d = norm.createView(Domain(0, 4));
    //        View n = norm.createView(Domain(0,5));

    //       View m = norm.createView(Domain(0,9));
    //       View o = norm.createView(Domain(0,9));
    //       View r = norm.createView(Domain(0,9));

    //       View y = norm.createView(Domain(0,9));


    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(d * 1);
        l.addRhs(3);
        // std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(d * 1);
        l.addRhs(1);
        // std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }


    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());

    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 3);
    return true;
}

REGISTERCONF(test4aux);


bool test5aux(Clingo::Control &ctl, clingcon::Config c)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    View ss = norm.createView(Domain(0, 5));
    View e = norm.createView(Domain(0, 5));


    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(e * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 30);
    return true;
}


REGISTERCONF(test5aux);


bool test6aux(Clingo::Control &ctl, clingcon::Config c)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    View s2 = norm.createView(Domain(0, 9));
    View e = norm.createView(Domain(0, 9));
    View n = norm.createView(Domain(0, 9));
    View d = norm.createView(Domain(0, 9));

    //      View m = norm.createView(Domain(0,9));
    //      View o = norm.createView(Domain(0,9));
    //      View r = norm.createView(Domain(0,9));

    //      View y = norm.createView(Domain(0,9));


    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(s2 * 1);
        l.add(e * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(s2 * 1);
        l.add(n * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(s2 * 1);
        l.add(d * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1);
        l.add(n * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1);
        l.add(d * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(n * 1);
        l.add(d * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 5040);
    return true;
}


REGISTERCONF(test6aux);


bool sendMoreMoneyaux(Clingo::Control &ctl, clingcon::Config c)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    View ss = norm.createView(Domain(0, 9));
    View e = norm.createView(Domain(0, 9));
    View n = norm.createView(Domain(0, 9));
    View d = norm.createView(Domain(0, 9));

    View m = norm.createView(Domain(0, 9));
    View o = norm.createView(Domain(0, 9));
    View r = norm.createView(Domain(0, 9));

    View y = norm.createView(Domain(0, 9));


    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss);
        l.addRhs(0);
        // std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(m);
        l.addRhs(0);
        // std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1000);
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
            ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }


    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(e * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(n * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(d * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(m * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(o * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(r * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(y * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1);
        l.add(n * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1);
        l.add(d * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1);
        l.add(m * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1);
        l.add(o * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1);
        l.add(r * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1);
        l.add(y * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(n * 1);
        l.add(d * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(n * 1);
        l.add(m * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(n * 1);
        l.add(o * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(n * 1);
        l.add(r * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(n * 1);
        l.add(y * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(d * 1);
        l.add(m * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(d * 1);
        l.add(o * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(d * 1);
        l.add(r * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(d * 1);
        l.add(y * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(m * 1);
        l.add(o * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(m * 1);
        l.add(r * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(m * 1);
        l.add(y * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(o * 1);
        l.add(r * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(o * 1);
        l.add(y * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 1);
    return true;
}

REGISTERCONF(sendMoreMoneyaux);


bool sendMoreMoney2aux(Clingo::Control &ctl, clingcon::Config c)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);
    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    View ss = norm.createView(Domain(0, 9));
    View e = norm.createView(Domain(0, 9));
    View n = norm.createView(Domain(0, 9));
    View d = norm.createView(Domain(0, 9));

    View m = norm.createView(Domain(0, 9));
    View o = norm.createView(Domain(0, 9));
    View r = norm.createView(Domain(0, 9));

    View y = norm.createView(Domain(0, 9));


    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss);
        l.addRhs(0);
        // std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(m);
        l.addRhs(0);
        // std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1000);
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
            ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }


    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(e * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(n * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(d * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(m * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(o * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(r * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.add(y * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1);
        l.add(n * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1);
        l.add(d * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1);
        l.add(m * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1);
        l.add(o * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1);
        l.add(r * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1);
        l.add(y * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(n * 1);
        l.add(d * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(n * 1);
        l.add(m * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(n * 1);
        l.add(o * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(n * 1);
        l.add(r * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(n * 1);
        l.add(y * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(d * 1);
        l.add(m * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(d * 1);
        l.add(o * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(d * 1);
        l.add(r * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(d * 1);
        l.add(y * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(m * 1);
        l.add(o * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(m * 1);
        l.add(r * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(m * 1);
        l.add(y * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(o * 1);
        l.add(r * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(o * 1);
        l.add(y * -1);
        l.addRhs(0);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 1);
    return true;
}

REGISTERCONF(sendMoreMoney2aux);


bool crypt112aux(Clingo::Control &ctl, clingcon::Config c)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);
    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    View e = norm.createView(Domain(0, 9));
    View i = norm.createView(Domain(0, 9));
    View n = norm.createView(Domain(0, 9));
    View ss = norm.createView(Domain(0, 9));

    View z = norm.createView(Domain(0, 9));
    View w = norm.createView(Domain(0, 9));


    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1000);
        l.add(i * 100);
        l.add(n * 10);
        l.add(ss * 1);
        l.add(e * 1000);
        l.add(i * 100);
        l.add(n * 10);
        l.add(ss * 1);
        l.add(z * -1000);
        l.add(w * -100);
        l.add(e * -10);
        l.add(i * -1);
        l.addRhs(0);
        // std::cout << std::endl << l << std::endl;
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }

    norm.addConstraint(ReifiedAllDistinct({e, i, n, ss, z, w}, s.trueLit(), Direction::EQ));


    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 12);
    return true;
}

REGISTERCONF(crypt112aux);


bool crypt224aux(Clingo::Control &ctl, clingcon::Config c)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


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
            ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(z * 1);
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    norm.addConstraint(ReifiedAllDistinct({z, w, e, i, v, r}, s.trueLit(), Direction::EQ));


    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 12);
    return true;
}

REGISTERCONF(crypt224aux);


bool crypt145aux(Clingo::Control &ctl, clingcon::Config c)
{


    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);


    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    View e = norm.createView();
    View i = norm.createView(Domain(0, 9));
    View n = norm.createView(Domain(0, 9));
    View ss = norm.createView(Domain(0, 9));

    View v = norm.createView(Domain(0, 9));
    View r = norm.createView(Domain(0, 9));

    View f = norm.createView(Domain(0, 9));
    View u = norm.createView(Domain(0, 9));


    norm.addConstraint(ReifiedDomainConstraint(e, Domain(0, 9), s.trueLit(), Direction::EQ));
    {
        LinearConstraint l(LinearConstraint::Relation::GE);
        l.add(e * 1);
        l.addRhs(0);
        // std::cout << std::endl << l << std::endl;
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(e * 1000);
        l.add(i * 100);
        l.add(n * 10);
        l.add(ss * 1);
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
            ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }

    norm.addConstraint(ReifiedAllDistinct({e, i, n, ss, v, r, f, u}, s.trueLit(), Direction::EQ));

    {
        LinearConstraint l(LinearConstraint::Relation::GE);
        l.add(e * 1);
        l.addRhs(4);
        // std::cout << std::endl << l << std::endl;
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.createNewLiteral(), Direction::EQ));
    }


    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 24);
    return true;
}


REGISTERCONF(crypt145aux);


bool allDiff1aux(Clingo::Control &ctl, clingcon::Config c)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);
    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    View s2 = norm.createView(Domain(0, 2));
    View e = norm.createView(Domain(0, 1));

    norm.addConstraint(ReifiedAllDistinct({s2, e}, s.trueLit(), Direction::EQ));


    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());


    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 4);
    return true;
}


REGISTERCONF(allDiff1aux);


bool allDiff2aux(Clingo::Control &ctl, clingcon::Config c)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    View ss = norm.createView(Domain(0, 2));
    View e = norm.createView(Domain(0, 1));
    View n = norm.createView(Domain(0, 1));
    // View d = norm.createView(Domain(0,1));

    norm.addConstraint(ReifiedAllDistinct({ss, e, n}, s.trueLit(), Direction::EQ));


    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());

    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 2);
    return true;
}


REGISTERCONF(allDiff2aux);


bool allDiff3aux(Clingo::Control &ctl, clingcon::Config c)
{


    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);
    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    View ss = norm.createView(Domain(0, 1));
    View e = norm.createView(Domain(0, 1));
    View n = norm.createView(Domain(0, 1));
    // View d = norm.createView(Domain(0,1));

    norm.addConstraint(ReifiedAllDistinct({ss, e, n}, s.trueLit(), Direction::EQ));


    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare() == false);
    return true;
}


REGISTERCONF(allDiff3aux);


bool allDiff4aux(Clingo::Control &ctl, clingcon::Config c)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    View ss = norm.createView(Domain(0, 2));
    View e = norm.createView(Domain(0, 2));
    View n = norm.createView(Domain(0, 2));
    // View d = norm.createView(Domain(0,1));

    norm.addConstraint(ReifiedAllDistinct({ss, e, n}, s.trueLit(), Direction::EQ));


    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 6);
    return true;
}

REGISTERCONF(allDiff4aux);


bool sendMoreMoney3aux(Clingo::Control &ctl, clingcon::Config c)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    View ss = norm.createView(Domain(0, 9));
    View e = norm.createView(Domain(0, 9));
    View n = norm.createView(Domain(0, 9));
    View d = norm.createView(Domain(0, 9));

    View m = norm.createView(Domain(0, 9));
    View o = norm.createView(Domain(0, 9));
    View r = norm.createView(Domain(0, 9));

    View y = norm.createView(Domain(0, 9));


    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.addRhs(0);
        // std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(m * 1);
        l.addRhs(0);
        // std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1000);
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
            ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }


    norm.addConstraint(
        ReifiedAllDistinct({ss, e, n, d, m, o, r, e, m, o, n, e, y}, s.trueLit(), Direction::EQ));


    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 1);
    return true;
}


REGISTERCONF(sendMoreMoney3aux);


bool sendMoreMoney4aux(Clingo::Control &ctl, clingcon::Config c)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    View ss = norm.createView(Domain(9, 9));
    View e = norm.createView(Domain(2, 9));
    View n = norm.createView(Domain(3, 9));
    View d = norm.createView(Domain(5, 9));

    View m = norm.createView(Domain(1, 1));
    View o = norm.createView(Domain(0, 0));
    View r = norm.createView(Domain(8, 9));
    View y = norm.createView(Domain(2, 7));


    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1);
        l.addRhs(0);
        // std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(m * 1);
        l.addRhs(0);
        // std::cout << std::endl << l << std::endl;

        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.falseLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(ss * 1000);
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
            ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }


    norm.addConstraint(
        ReifiedAllDistinct({ss, e, n, d, m, o, r, e, m, o, n, e, y}, s.trueLit(), Direction::EQ));


    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 1);
    return true;
}

REGISTERCONF(sendMoreMoney4aux);


bool unsat1aux(Clingo::Control &ctl, clingcon::Config c)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);


    View v10 = norm.createView(Domain(1, 10));
    View v1 = norm.createView(Domain(1, 10));
    View v0 = norm.createView(Domain(1, 10));
    View v11 = norm.createView(Domain(1, 10));

    {
        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(v10 * 1);
        l.add(v0 * -1);
        l.addRhs(1);
        norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(v0 * 1);
        l.add(v1 * -1);
        l.addRhs(0);
        norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(v1 * 1);
        l.add(v11 * -1);
        l.addRhs(-2);
        norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(v11 * 1);
        l.add(v10 * -1);
        l.addRhs(0);
        norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }


    REQUIRE(norm.prepare() == false);
    return true;
}

REGISTERCONF(unsat1aux);


bool unsat2aux(Clingo::Control &ctl, clingcon::Config c)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);

    View v10 = norm.createView(Domain(1, 10));
    View v1 = norm.createView(Domain(1, 10));
    View v0 = norm.createView(Domain(1, 10));
    View v11 = norm.createView(Domain(1, 10));

    {
        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(v10 * 1);
        l.add(v0 * -1);
        l.addRhs(1);
        norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(v0 * 1);
        l.add(v1 * -1);
        l.addRhs(0);
        norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(v1 * 1);
        l.add(v11 * -1);
        l.addRhs(-2);
        norm.addConstraint(ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::LE);
        l.add(v11 * 1);
        l.add(v10 * -1);
        l.addRhs(0);
        norm.addConstraint(
            ReifiedLinearConstraint(std::move(l), s.createNewLiteral(), Direction::EQ));
    }

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 450);
    return true;
}


REGISTERCONF(unsat2aux);


bool big1(Clingo::Control &ctl)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, lazySolveConfigProp4);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    unsigned int factor = 1000000;


    View ss = norm.createView(Domain(0, 1 * factor));
    View e = norm.createView(Domain(0, 2 * factor));
    View n = norm.createView(Domain(0, 3 * factor));

    LinearConstraint l(LinearConstraint::Relation::EQ);
    l.add(ss * 1);
    l.add(e * 1);
    l.add(n * -1);
    l.addRhs(0);
    // std::cout << std::endl << l << std::endl;
    linearConstraints.emplace_back(
        ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModelsMin(ctl, 13));
    return true;
}


REGISTER(big1);


bool bigger1(Clingo::Control &ctl)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, lazySolveConfigProp4);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    // unsigned int factor = 100000000;
    unsigned int factor = (2147483648 / 3) - 1;

    View ss = norm.createView(Domain(0, 1 * factor));
    View e = norm.createView(Domain(0, 2 * factor));
    View n = norm.createView(Domain(0, 3 * factor));


    LinearConstraint l(LinearConstraint::Relation::EQ);
    l.add(ss * 1);
    l.add(e * 1);
    l.add(n * -1);
    l.addRhs(0);
    // std::cout << std::endl << l << std::endl;
    linearConstraints.emplace_back(
        ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModelsMin(ctl, 13));
    return true;
}

REGISTER(bigger1);


bool bigger2(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, lazySolveConfigProp4);


    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    // unsigned int factor = 100000000;
    unsigned int factor = Domain::max;


    View ss = norm.createView(Domain(0, factor));
    View e = norm.createView(Domain(0, factor));
    View n = norm.createView(Domain(0, factor));


    LinearConstraint l(LinearConstraint::Relation::LE);
    l.add(ss);
    l.add(e);
    l.add(n * -3);
    l.addRhs(65536);
    // std::cout << std::endl << l << std::endl;
    linearConstraints.emplace_back(
        ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());

    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModelsMin(ctl, 13));
    return true;
}


REGISTER(bigger2);


bool bigger3(Clingo::Control &ctl)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, lazySolveConfigProp4);


    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    // unsigned int factor = 100000000;
    unsigned int factor = Domain::max;

    View ss = norm.createView(Domain(0, factor));
    View e = norm.createView(Domain(0, factor));
    View n = norm.createView(Domain(0, factor));


    LinearConstraint l(LinearConstraint::Relation::EQ);
    l.add(ss * 1440);
    l.add(e * 6);
    l.add(n * -3);
    l.addRhs(136164);
    // std::cout << std::endl << l << std::endl;
    linearConstraints.emplace_back(
        ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModelsMin(ctl, 13));
    return true;
}

REGISTER(bigger3);


bool bigger4(Clingo::Control &ctl)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, lazySolveConfigProp4);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;


    View a = norm.createView(Domain());
    // View b = norm.createView(Domain());
    View c = norm.createView(Domain());


    LinearConstraint l(LinearConstraint::Relation::EQ);
    l.add(a * 100);
    // l.add(b*-42100);
    l.add(c * 123456);
    l.addRhs(1234560);
    // std::cout << std::endl << l << std::endl;
    linearConstraints.emplace_back(
        ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModelsMin(ctl, 13));
    return true;
}

REGISTER(bigger4);

bool bigger6(Clingo::Control &ctl)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, lazySolveConfigProp4);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;
    View a = norm.createView(Domain());
    // View b = norm.createView(Domain());
    View c = norm.createView(Domain());


    LinearConstraint l(LinearConstraint::Relation::EQ);
    l.add(a * 25);
    // l.add(b*-42100);
    l.add(c * 30864);
    l.addRhs(308640);
    // std::cout << std::endl << l << std::endl;
    linearConstraints.emplace_back(
        ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());

    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModelsMin(ctl, 13));
    return true;
}

REGISTER(bigger6);


bool testPidgeon2(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, lazySolveConfigProp4);

    View q[10];
    for (unsigned int i = 0; i < 4; ++i) q[i] = norm.createView(Domain(1, 4));

    norm.addConstraint(ReifiedAllDistinct({q[0], q[1], q[2], q[3]}, s.trueLit(), Direction::EQ));


    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 24);
    return true;
}

REGISTER(testPidgeon2);


bool testDomain21(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, lazySolveConfigProp4);


    View q = norm.createView(Domain(1, 10));

    Domain d(2, 6);
    d.remove(Domain(5, 5));

    norm.addConstraint(
        ReifiedDomainConstraint(q, std::move(d), s.createNewLiteral(), Direction::EQ));


    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());

    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 10);
    return true;
}

REGISTER(testDomain21);

bool testDomain22(Clingo::Control &ctl)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, lazySolveConfigProp4);

    auto myconf = lazySolveConfigProp4;


    View q = norm.createView(Domain(1, 10));

    Domain d(2, 6);
    d.remove(Domain(5, 5));
    norm.addConstraint(
        ReifiedDomainConstraint(q, std::move(d), s.createNewLiteral(), Direction::FWD));


    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());

    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 14);
    return true;
}

REGISTER(testDomain22);


bool equalDistinct(Clingo::Control &ctl, clingcon::Config c)
{

    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);

    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    View a = norm.createView(Domain(0, 50));
    View b = norm.createView(Domain(0, 50));
    View cc = norm.createView(Domain(0, 50));
    View d = norm.createView(Domain(0, 50));


    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(a * 5);
        l.addRhs(25);
        // std::cout << std::endl << l << std::endl;
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(a * 3);
        l.add(b * -1);
        l.addRhs(-3);
        // std::cout << std::endl << l << std::endl;
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }

    norm.addConstraint(ReifiedAllDistinct({a, b, cc * 0, d}, s.trueLit(), Direction::EQ));


    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());


    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 2448);
    return true;
}


REGISTERCONF(equalDistinct);


bool distinctDom(Clingo::Control &ctl, clingcon::Config c)
{
    Stats stats;
    Grounder s(ctl, stats);
    Normalizer norm(s, c);
    std::vector< clingcon::ReifiedLinearConstraint > linearConstraints;

    View a = norm.createView();
    View b = norm.createView();

    {
        ReifiedDomainConstraint d(b, Domain(10, 11), s.trueLit(), Direction::EQ);
        norm.addConstraint(std::move(d));
    }

    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(a * 1);
        l.add(b * -5);
        l.addRhs(5);
        // std::cout << std::endl << l << std::endl;
        linearConstraints.emplace_back(
            ReifiedLinearConstraint(std::move(l), s.trueLit(), Direction::EQ));
    }

    for (auto &i : linearConstraints) norm.addConstraint(std::move(i));

    REQUIRE(norm.prepare());

    REQUIRE(norm.finalize());

    clingcon::ClingconPropagator p(stats, s.trueLit(), norm.getVariableCreator(), norm.getConfig(),
                                   nullptr, nullptr, norm.constraints());
    ctl.register_propagator(p);
    REQUIRE(expectedModels(ctl) == 2);
    return true;
}

REGISTERCONF(distinctDom);
