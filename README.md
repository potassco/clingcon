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
