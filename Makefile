SHELL := /bin/bash
.DEFAULT_GOAL := release
.PHONY: debug release install test conan-create viewcoverage
lcov := lcov

f_debug = build_debug
f_release = build_release
genhtml = genhtml
docscdir = docs

define build_target
	@echo ">>> Building the project for $(1)..."
	@if [ -d $(2) ]; then rm -fr $(2); fi
	@conan install . --build=missing -of $(2) -s build_type=$(1)
	@cmake -S . -B $(2) -DCMAKE_TOOLCHAIN_FILE=$(2)/build/$(1)/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=$(1) -D$(3)
	@cmake --build $(2) --config $(1) -j 8
endef

debug: ## Build Debug version of the library
	@$(call build_target,"Debug","$(f_debug)", "ENABLE_TESTING=ON")

release: ## Build Release version of the library
	@$(call build_target,"Release","$(f_release)", "ENABLE_TESTING=OFF")

install: ## Install the project
	@echo ">>> Installing the project..."
	@cmake --build $(f_release) --target install -j 8		

test: ## Build Debug version and run tests
	@echo ">>> Building Debug version and running tests..."
	@if [ ! -d $(f_debug) ]; then \
		$(MAKE) debug; \
	else \
		echo ">>> Debug build already exists, skipping build."; \
	fi
	@cp -r tests/datasets $(f_debug)/tests/datasets
	@cd $(f_debug)/tests && ctest --output-on-failure -j 8
	@cd $(f_debug)/tests && $(lcov) --capture --directory ../ --demangle-cpp --ignore-errors source,source --ignore-errors mismatch --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info '/usr/*' --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info 'lib/*' --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info 'libtorch/*' --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info 'tests/*' --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info 'gtest/*' --output-file coverage.info >/dev/null 2>&1; \
	$(lcov) --remove coverage.info '*/.conan2/*' --ignore-errors unused --output-file coverage.info >/dev/null 2>&1;
	@genhtml $(f_debug)/tests/coverage.info --demangle-cpp --output-directory $(f_debug)/tests/coverage --title "Discretizer mdlp Coverage Report" -s -k -f --legend
	@echo "* Coverage report is generated at $(f_debug)/tests/coverage/index.html"
	@which python || (echo ">>> Please install python"; exit 1)
	@if [ ! -f $(f_debug)/tests/coverage.info ]; then \
		echo ">>> No coverage.info file found!"; \
		exit 1; \
	fi
	@echo ">>> Updating coverage badge..."
	@env python update_coverage.py $(f_debug)/tests
	@echo ">>> Done"

viewcoverage: ## View the html coverage report
	@which $(genhtml) >/dev/null || (echo ">>> Please install lcov (genhtml not found)"; exit 1)
	@if [ ! -d $(docscdir)/coverage ]; then mkdir -p $(docscdir)/coverage; fi
	@if [ ! -f $(f_debug)/tests/coverage.info ]; then \
		echo ">>> No coverage.info file found. Run make coverage first!"; \
		exit 1; \
	fi
	@$(genhtml) $(f_debug)/tests/coverage.info --demangle-cpp --output-directory $(docscdir)/coverage --title "FImdlp Coverage Report" -s -k -f --legend >/dev/null 2>&1;
	@xdg-open $(docscdir)/coverage/index.html || open $(docscdir)/coverage/index.html 2>/dev/null
	@echo ">>> Done";

conan-create: ## Create the conan package
	@echo ">>> Creating the conan package..."
	conan create . --build=missing --pr:b=release -pr:h=release
	conan create . --build=missing -pr:b=debug -pr:h=debug



