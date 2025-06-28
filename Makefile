SHELL := /bin/bash
.DEFAULT_GOAL := build
.PHONY: build install test
lcov := lcov

f_debug = build_debug
f_release = build_release

build: ## Build the project for Release
	@echo ">>> Building the project for Release..."
	@if [ -d $(f_release) ]; then rm -fr $(f_release); fi
	@conan install . --build=missing -of $(f_release) -s build_type=Release --profile:build=default --profile:host=default
	cmake -S . -B $(f_release) -DCMAKE_TOOLCHAIN_FILE=$(f_release)/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=OFF -DENABLE_SAMPLE=OFF
	@cmake --build $(f_release) -j 8

install: ## Install the project
	@echo ">>> Installing the project..."
	@cmake --build build_release --target install -j 8		

test: ## Build Debug version and run tests
	@echo ">>> Building Debug version and running tests..."
	@if [ -d $(f_debug) ]; then rm -fr $(f_debug); fi
	@conan install . --build=missing -of $(f_debug) -s build_type=Debug
	@cmake -B $(f_debug) -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=$(f_debug)/build/Debug/generators/conan_toolchain.cmake -DENABLE_TESTING=ON -DENABLE_SAMPLE=ON
	@cmake --build $(f_debug) -j 8
	@cp -r tests/datasets $(f_debug)/tests/datasets
	@cd $(f_debug)/tests && ctest --output-on-failure -j 8
	@cd $(f_debug)/tests && $(lcov) --capture --directory ../ --demangle-cpp --ignore-errors source,source --ignore-errors mismatch --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info '/usr/*' --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info 'lib/*' --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info 'libtorch/*' --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info 'tests/*' --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info 'gtest/*' --output-file coverage.info >/dev/null 2>&1;
	@genhtml $(f_debug)/tests/coverage.info --demangle-cpp --output-directory $(f_debug)/tests/coverage --title "Discretizer mdlp Coverage Report" -s -k -f --legend
	@echo "* Coverage report is generated at $(f_debug)/tests/coverage/index.html"
	@which python || (echo ">>> Please install python"; exit 1)
	@if [ ! -f $(f_debug)/tests/coverage.info ]; then \
		echo ">>> No coverage.info file found!"; \
		exit 1; \
	fi
	@echo ">>> Updating coverage badge..."
	@env python update_coverage.py $(f_debug)/tests