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
