# vim: noexpandtab
.PHONY: all clean run

CXXFLAGS := -g -std=gnu++17 -fno-omit-frame-pointer -fsanitize=address
LDFLAGS := -g -framework Cocoa -fsanitize=address

all: build/DBATool

run: all
	@build/DBATool

build:
	@mkdir -p build

build/DBATool: build/main.o
	@c++ -o $@ $(LDFLAGS) $^

build/main.o: src/main.cpp build
	@c++ -c -o $@ $(CXXFLAGS) $<

clean:
	@rm -rf build compile_commands.json




