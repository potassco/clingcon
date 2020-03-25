# Clingcon

## Building with conda

    conda create -n clingcon -c potassco/label/dev -c conda-forge clingo ninja cmake gxx_linux-64 libcxx clang-tools
    conda activate clingcon
    make

## Linting

The default Makefile will setup the cmake to use `clang-tidy`. It is also
possible to create a compile database to use with linting plugins.

    conda install -n clingcon -c programfan compdb
    conda activate clingcon
    make compdb

With this, plugins like [vim-ale] should be able to lint the source code while
editing.

[vim-ale]: https://github.com/dense-analysis/ale
