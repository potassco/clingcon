mkdir build

cmake -G "%CMAKE_GENERATOR%" -H. -Bbuild ^
    -DCMAKE_CXX_COMPILER="%CXX%" ^
    -DPython_ROOT_DIR="%PREFIX%" ^
    -DCMAKE_INSTALL_PREFIX="%PREFIX%" ^
    -DCMAKE_INSTALL_BINDIR="." ^
    -DCLINGCON_MANAGE_RPATH=Off ^
    -DPYCLINGCON_ENABLE="require"

cmake --build build --config Release
cmake --build build --config Release --target install
