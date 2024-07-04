SHELL := /bin/bash
.DEFAULT_GOAL := build
.PHONY: build test
lcov := lcov

build: 
	@if [ -d build_release ]; then rm -fr build_release; fi
	@mkdir build_release
	@cmake -B build_release -S . -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=OFF
	@cmake --build build_release -j 8

test:
	@if [ -d build_debug ]; then rm -fr build_debug; fi
	@mkdir build_debug
	@cmake -B build_debug -S . -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=ON
	@cmake --build build_debug -j 8
	@cd build_debug/tests && ctest --output-on-failure -j 8
	@cd build_debug/tests && $(lcov) --capture --directory ../ --demangle-cpp --ignore-errors source,source --ignore-errors mismatch --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info '/usr/*' --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info 'lib/*' --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info 'libtorch/*' --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info 'tests/*' --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info 'gtest/*' --output-file coverage.info >/dev/null 2>&1;
	@genhtml build_debug/tests/coverage.info --demangle-cpp --output-directory build_debug/tests/coverage --title "Discretizer mdlp Coverage Report" -s -k -f --legend
	@echo "* Coverage report is generated at build_debug/tests/coverage/index.html"
	@which python || (echo ">>> Please install python"; exit 1)
	@if [ ! -f build_debug/tests/coverage.info ]; then \
		echo ">>> No coverage.info file found!"; \
		exit 1; \
	fi
	@echo ">>> Updating coverage badge..."
	@env python update_coverage.py build_debug/tests