#!/bin/bash

mkdir build
cd build

cmake .. \
    -DCLINGCON_MANAGE_RPATH=Off \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER="${CXX}" \
    -DCMAKE_INSTALL_LIBDIR="lib" \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DPYCLINGCON_ENABLE="require" \
    -DPython_ROOT_DIR="${PREFIX}"

make -j${CPU_COUNT}
make install
