#!/bin/bash
set -xe

VERSION="$(grep '#define CLINGCON_VERSION "[^.]\+.[^.]\+.[^.]\+"' libclingcon/clingcon.h | colrm 1 25 | tr -d '"')"
MINOR=${VERSION%.*}
MAJOR=${MINOR%.*}

cd "$(dirname $0)/.."

prefix=/home/wv/opt/clingcon-${VERSION}
source="$(pwd -P)"

mkdir -p ${prefix}

mkdir -p build/${VERSION}
(
    cd build/${VERSION}
    cd "$(pwd -P)"
    cmake "${source}" \
        -DCMAKE_BUILD_TYPE=release \
        -DCMAKE_VERBOSE_MAKEFILE=On \
        -DCMAKE_INSTALL_PREFIX="${prefix}" \
        -DClingo_DIR="/home/wv/opt/clingo-banane/lib/cmake/Clingo" \
        -DCLINGCON_BUILD_WITH_SYSTEM_CLINGO=On \
        -DCMAKE_EXE_LINKER_FLAGS="-s -static-libgcc -static-libstdc++" \
        -DCMAKE_SHARED_LINKER_FLAGS="-s -static-libgcc -static-libstdc++"
    make -j8
    make install
)

cd /home/wv/bin/linux/64
(
    rm -f clingcon{-${VERSION},-${MAJOR},-${MINOR}}

    for x in clingcon; do
        ln -s ${prefix}/bin/${x} ${x}-${VERSION}
        ln -s ${x}-${VERSION} ${x}-${MINOR}
        ln -s ${x}-${MINOR}   ${x}-${MAJOR}
        ln -s ${x}-${MAJOR}   ${x}
    done
)

