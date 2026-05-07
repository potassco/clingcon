cmake -S . -B build -G "Visual Studio 18 2026" ^
    -DCMAKE_INSTALL_PREFIX="%LIBRARY_PREFIX%" ^
    -DCMAKE_INSTALL_LIBDIR="lib" ^
    -DPython_ROOT_DIR="%PREFIX%" ^
    -DPython_EXECUTABLE="%PYTHON%" ^
    -DCLINGCON_MANAGE_RPATH=OFF ^
    -DCLINGCON_BUILD_TESTS=ON ^
    -DPYCLINGCON_INSTALL_DIR="%SP_DIR%"

cmake --build build --config Release
ctest --test-dir build -C Release
cmake --build build --target install --config Release
