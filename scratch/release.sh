#!/bin/bash




rm -rf release
mkdir release
cd release

VERSION=3.3.0
git clone https://github.com/potassco/clingcon.git .
#git checkout v${VERSION}
git submodule update --init --recursive

GRINGO="clingcon-${VERSION}"
GRINGO_LIN64="${GRINGO}-linux-x86_64"
SRC="${GRINGO}-source"
CMAKE=/home/wv/bin/linux/64/cmake-3.5.2/bin/cmake
#CMAKE=cmake

${CMAKE} -H. -Bbuild -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -DCLINGO_REQUIRE_LUA=OFF -DCLINGO_BUILD_WITH_LUA=ON -DCLINGO_BUILD_WITH_PYTHON=OFF -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=release -DCLINGO_BUILD_STATIC=ON -DCLINGO_MANAGE_RPATH=Off -DCMAKE_EXE_LINKER_FLAGS="-pthread -static -s -Wl,-u,pthread_cond_broadcast,-u,pthread_cond_destroy,-u,pthread_cond_signal,-u,pthread_cond_timedwait,-u,pthread_cond_wait,-u,pthread_create,-u,pthread_detach,-u,pthread_equal,-u,pthread_getspecific,-u,pthread_join,-u,pthread_key_create,-u,pthread_key_delete,-u,pthread_mutex_lock,-u,pthread_mutex_unlock,-u,pthread_once,-u,pthread_setspecific"
${CMAKE} --build build -- -j4


