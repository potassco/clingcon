BUILD_TYPE:=debug

.PHONY: all configure compdb

all: configure
	@MAKEFLAGS= MFLAGS= cmake --build "build/$(BUILD_TYPE)" --target all

%: configure
	@cmake --build "build/$(BUILD_TYPE)" --target "$@"

# compdb can be installed with pip
compdb: configure
	compdb -p "build/$(BUILD_TYPE)" list -1 > compile_commands.json

configure: build/$(BUILD_TYPE)/build.ninja

build/$(BUILD_TYPE)/build.ninja:
	cmake -G Ninja -H. -B"build/$(BUILD_TYPE)" -DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" -DCLINGCON_BUILD_TESTS=On -DCMAKE_EXPORT_COMPILE_COMMANDS=On

Makefile:
	:
