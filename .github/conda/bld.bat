cmake -G "Ninja" -S . -B build ^
    -DPython_FIND_STRATEGY="LOCATION" ^
    -DPython_EXECUTABLE="%PYTHON%" ^
    -DPython_ROOT_DIR="%PREFIX%" ^
    -DPYCLINGCON_ENABLE="require" ^
    -DPYCLINGCON_INSTALL_DIR="%SP_DIR%" ^
    -DCMAKE_INSTALL_PREFIX="%LIBRARY_PREFIX%" ^
    -DCLINGCON_MANAGE_RPATH=Off ^
    -DCMAKE_BUILD_TYPE=Release || exit /b 1

cmake --build build || exit /b 1
cmake --build build --target install || exit /b 1
