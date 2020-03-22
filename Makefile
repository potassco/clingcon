BUILD_TYPE:=debug
CLINGO_DIR:=$(HOME)/git/clingo/install/$(BUILD_TYPE)/lib64/cmake/Clingo
CLANG_TIDY:=clang-tidy;-checks=clang-analyzer-*,readability-*,modernize-*,cppcoreguidelines-*,performance-*,bugprone-*,-modernize-use-trailing-return-type;-warnings-as-errors=*

.PHONY: all configure

all: build/$(BUILD_TYPE)/build.ninja
	@cmake --build "build/$(BUILD_TYPE)" --target all

%: build/$(BUILD_TYPE)/build.ninja
	@cmake --build "build/$(BUILD_TYPE)" --target "$@"

build/$(BUILD_TYPE)/build.ninja:
	cmake -G Ninja -H. -B"build/$(BUILD_TYPE)" -DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" -DCMAKE_CXX_CLANG_TIDY:STRING="$(CLANG_TIDY)" -DClingo_DIR="$(CLINGO_DIR)" -DCLINGCON_BUILD_TESTS=On

Makefile:
	:
