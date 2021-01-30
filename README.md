# Clingcon

![clingcon CI tests](https://github.com/potassco/clingcon/workflows/tests/badge.svg)

The clingcon-5 project is still in development. A release has to wait until
clingo-5.5.0 is ready. The project is already usable with the latest
development version of clingo. The latest stable clingcon release is 3.3.0
available under the [releases][release] tab and in the [clingcon-3] branch.

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

The Makefile is meant for development and sets up cmake to use `clang-tidy`. It
is also possible to create a compile database to use with linting plugins.

    conda install -n clingcon -c conda-forge -c programfan \
          compdb libcxx clang-tools
    conda activate clingcon
    make compdb

With this, plugins like [vim-ale] should be able to lint the source code while
editing.

[vim-ale]: https://github.com/dense-analysis/ale
[release]: https://github.com/potassco/clingcon/releases
[clingcon-3]: https://github.com/potassco/clingcon/tree/clingcon-3
