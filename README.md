# Clingcon

![clingcon CI tests](https://github.com/potassco/clingcon/workflows/tests/badge.svg)

Clingcon is an answer set solver for constraint logic programs building upon
the answer set solver [clingo]. It extends the high-level modeling language of
ASP with constraints over finite domain integer variables.

The latest source release is available under the [releases][release] tab.
Binary packages can be installed using on of the following package managers:

- Packages for a wide range of platforms are available on [Anaconda][conda]
- Ubuntu users can install packages from our [Ubuntu PPA][ubuntu]
- Clingcon's Python module is available on [PyPI][pypi]

## Multi-level optimization

The elements of `&minimize` and `&maximize` can be given an optimization level
after `@`. The levels are optimized lexicographically, the highest level first.
An element without a level is at level 0.

    &dom { 0..10 } = cost.
    &dom { 0..10 } = delay.
    &sum { cost; delay } >= 10.
    &minimize { cost@2; delay@1 }.

The optimum of this program is `cost=0 delay=10`. There are two ways to handle
the levels:

    --lex-opt=<arg>              : Optimize the levels of the minimize constraint [auto]
          <arg>: {auto|multi-shot|weighted}
            auto      : multi-shot if the application steps through the levels
                        and one thread is used, weighted otherwise
            multi-shot: optimize one level per solve call
            weighted  : fold the levels into one minimize constraint

In multi-shot mode, a level that is proven optimal is fixed with a constraint
and the next level is optimized in the next solve call; the optimum of the
level has to fit into a value for that, which weighted mode does not need. An
application that
uses the library drives this loop: it calls `clingcon_enable_steps` before its
first solve call and `clingcon_step` after every solve call, and solves again
on the same control object as long as `clingcon_step` reports more work. In
weighted mode the levels are folded into one minimize constraint with
mixed-radix weights and one solve call is enough, but the weights grow with the
domains of the variables.

The levels are handled by the propagator. A `&minimize` with levels is not
translated into clingo's `#minimize`, and the levels are not ordered against
clingo's own `#minimize` statements.

## Heuristic directives

A `&heuristic` directive tells the decision heuristic which integer variable to
branch on and which value to try first, in the style of MiniZinc's search
annotations. A directive changes the order in which the models of a program are
found, not the models themselves.

    &heuristic(P, VarSel, ValSel [, Tie] [, Lvl]) { x; y; ... }.

The directives are consulted in increasing `P`. A directive acts whenever one of
its variables is unassigned; an empty or omitted set covers all integer
variables. `VarSel` picks the variable:

| `VarSel`          | branches on                                  |
| ----------------- | -------------------------------------------- |
| `input_order`     | the first unassigned variable of the set     |
| `first_fail`      | the smallest domain width                    |
| `anti_first_fail` | the largest domain width                     |
| `smallest`        | the smallest lower bound                     |
| `largest`         | the largest upper bound                      |
| `degree`          | the variable that most constraints watch     |
| `dom_deg`         | the smallest ratio of domain width to degree |
| `vsids`           | the variable clasp selected                  |

`ValSel` picks the value:

| `ValSel`                 | tries first                  |
| ------------------------ | ---------------------------- |
| `indomain_min`           | the lower bound              |
| `indomain_max`           | the upper bound              |
| `indomain_split`         | the lower half of the domain |
| `indomain_reverse_split` | the upper half of the domain |

`Tie` is `none` or `degree`. The `degree` tie-breaker takes the variable that
more constraints watch. It has an effect with `first_fail`, `anti_first_fail`,
`smallest` and `largest`, and clingcon reports it as ignored with the others.

The `vsids` variable selection is cooperative: clasp selects the atom and the
directive selects the value of that variable if the atom is an order atom of
one of its variables. The choice of clasp stands otherwise.

`Lvl` scopes a directive to one `@priority` level. The directive is active while
that level is optimized in multi-shot mode. Weighted mode optimizes all levels
at once and has no active level, so a scoped directive does not act there and
clingcon says so on standard error.

    &dom { 0..20 } = start(T) :- T = 1..3.
    &sum { start(T); 5 } <= start(T+1) :- T = 1..2.
    &heuristic(0, first_fail, indomain_min) { start(T) : T = 1..3 }.

Where no directive applies, `--order-heuristic` still steers the variable of
the literal clasp selected. Besides `max-chain` there are `earliest-start`,
which branches on the lower bound of that variable, and `tightest-window`,
which splits its domain in half.

    --order-heuristic=<arg>      : Make the decision heuristic aware of order literals [none]
          <arg>: {none,max-chain,earliest-start,tightest-window}[,<i>]
            none           : use clasp's heuristic
            max-chain      : assign chains of literals
            earliest-start : branch on the lower bound of the variable
            tightest-window: split the domain of the variable
          <i>  : Only enable for thread <i>

## Bound stepping

After a model, branch-and-bound posts an upper bound on the minimize
constraint so that the next model has to be better. The bound is the value of
the model minus one, so the number of models between the first one and the
optimum grows with the gap between them. Bisecting that gap instead needs a
number of models that is logarithmic in its size.

    --bound-step=<arg>           : Pick the bound posted after a model [linear]
          <arg>: {linear|bisect|hybrid[:K]}
            linear    : the value of the model minus one
            bisect    : the midpoint between the lower bound and that value
            hybrid[:K]: bisect while the gap is larger than K, then linear [10]

The lower bound that the midpoint is taken from follows from the domains of
the variables in the minimize constraint. A solve call that ends because no
model meets a midpoint bound has not proven optimality; the values between the
midpoint and the last model are searched in a further solve call, and once
nothing is left below the last model, one more solve call reports it, so that
the call that ends the search ends with the optimum. An application that uses
the library therefore drives `bisect` and `hybrid` with the same loop as
multi-shot optimization above.

Bisection helps when the first model is far from the optimum. When it is
close, a midpoint is refuted often and the solve calls that this costs
outweigh the models that are skipped, which is what `hybrid` switches away
from.

## Building a release version with conda

    conda create -n clingcon -c potassco/label/dev -c conda-forge \
          clingo ninja cmake cxx-compiler
    conda activate clingcon
    cmake -G Ninja \
          -Bbuild/release \
          -H. \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX="${CONDA_PREFIX}" \
          -DCMAKE_INSTALL_LIBDIR=lib \
          -DCLINGCON_MANAGE_RPATH=Off
    cmake --build build/release --target install
    clingcon --version

## Development

The Makefile is meant for development. It is also possible to create a compile
database to use with linting plugins.

    conda install -n clingcon -c conda-forge -c programfan \
          compdb libcxx clang-tools
    conda activate clingcon
    make compdb

With this, plugins like [vim-ale] should be able to lint the source code while
editing.

### Profiling

The makefile enables easy setup of profiling. To profile, use

    make BUILD_TYPE=profile
    ./build/profile/bin/clingcon <options>

To visualize the callgraph, use

    pprof --gv ./build/profile/bin/clingcon clingcon.solve.prof

To browse the profiling data, use

    pprof --callgrind ./build/profile/bin/clingcon clingcon.solve.prof > clingcon.solve.out
    kcachegrind clingcon.solve.out

[vim-ale]: https://github.com/dense-analysis/ale/
[release]: https://github.com/potassco/clingcon/releases/
[clingo]: https://github.com/potassco/clingo/
[conda]: https://anaconda.org/conda-forge/clingcon/
[pypi]: https://pypi.org/project/clingcon/
[ubuntu]: https://launchpad.net/~potassco/
