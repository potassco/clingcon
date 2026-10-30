BUILD_TYPE ?= debug
POTASSCO_PREFIX ?=
_BUILD_TYPE_UPPER := $(shell printf '%s' "$(BUILD_TYPE)" | tr '[:lower:]' '[:upper:]')

define cmake_options
-S . \
-B "build/$(BUILD_TYPE)" \
-DCLINGCON_BUILD_TESTS=On \
-DCMAKE_EXPORT_COMPILE_COMMANDS=On
endef

ifneq ($(CC_$(_BUILD_TYPE_UPPER)),)
	cmake_options += -DCMAKE_C_COMPILER="$(CC_$(_BUILD_TYPE_UPPER))"
endif

ifneq ($(CXX_$(_BUILD_TYPE_UPPER)),)
	cmake_options += -DCMAKE_CXX_COMPILER="$(CXX_$(_BUILD_TYPE_UPPER))"
endif

ifneq ($(CFLAGS_$(_BUILD_TYPE_UPPER)),)
	cmake_options += -DCMAKE_C_FLAGS="$(CFLAGS_$(_BUILD_TYPE_UPPER))"
endif

ifneq ($(CXXFLAGS_$(_BUILD_TYPE_UPPER)),)
	cmake_options += -DCMAKE_CXX_FLAGS="$(CXXFLAGS_$(_BUILD_TYPE_UPPER))"
endif

ifneq ($(POTASSCO_PREFIX),)
	cmake_options += -Dclingo_DIR="$(POTASSCO_PREFIX)/$(BUILD_TYPE)/lib/cmake/clingo"
	cmake_options += -DCMAKE_INSTALL_PREFIX="$(POTASSCO_PREFIX)/$(BUILD_TYPE)"
endif

ifeq ($(BUILD_TYPE),debug)
	cmake_options += -DCMAKE_BUILD_TYPE="Debug"
else ifeq ($(BUILD_TYPE),profile)
	cmake_options += -DCMAKE_BUILD_TYPE="RelWithDebInfo" -DCLINGCON_PROFILE=On
else
	cmake_options += -DCMAKE_BUILD_TYPE="Release"
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
