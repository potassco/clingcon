BUILD_TYPE:=debug
POTASSCO_PREFIX:=${HOME}/.local/opt/potassco/$(BUILD_TYPE)
CXX_FLAGS := -stdlib=libc++ -Wall -Wextra -Wpedantic -Werror
C_COMPILER := $(shell for c in clang-22 clang-21 clang-20 clang; do command -v $$c >/dev/null 2>&1 && { command -v $$c; break; }; done)
CXX_COMPILER := $(shell for c in clang++-22 clang++-21 clang++-20 clang++; do command -v $$c >/dev/null 2>&1 && { command -v $$c; break; }; done)

define cmake_options
-S . \
-B "build/$(BUILD_TYPE)" \
-DCMAKE_CXX_COMPILER="$(CXX_COMPILER)" \
-DCMAKE_C_COMPILER="$(C_COMPILER)" \
-DCMAKE_INSTALL_PREFIX="$(POTASSCO_PREFIX)" \
-DCMAKE_CXX_FLAGS="$(CXX_FLAGS)" \
-DCLINGCON_BUILD_TESTS=On \
-DCMAKE_EXPORT_COMPILE_COMMANDS=On
endef

ifeq ($(BUILD_TYPE),profile)
	cmake_options += -DCMAKE_BUILD_TYPE="RelWithDebInfo" -DCLINGCON_PROFILE=On
else
	cmake_options += -DCMAKE_BUILD_TYPE="$(BUILD_TYPE)"
endif


.PHONY: all test compdb configure

all: configure
	$(MAKE) -C "build/$(BUILD_TYPE)"

test: all
	ctest --test-dir "build/$(BUILD_TYPE)" --output-on-failure

%: configure
	$(MAKE) -C "build/$(BUILD_TYPE)" "$@"

compdb: configure
	compdb -p "build/$(BUILD_TYPE)" list -1 > compile_commands.json

configure: build/$(BUILD_TYPE)/Makefile

build/$(BUILD_TYPE)/Makefile:
	cmake $(cmake_options)

Makefile:
	:
