SHELL := /bin/bash
.DEFAULT_GOAL := build
.PHONY: build test

build: 
	@if [ -d build_release ]; then rm -fr build_release; fi
	@mkdir build_release
	@cmake -B build_release -S . -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=OFF
	@cmake --build build_release

test:
	@echo "Testing..."
	@cd tests && ./test
