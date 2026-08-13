SHELL := /bin/bash
.DEFAULT_GOAL := help
# Every target here is a command, not a file. `bench` collides with the bench/
# directory, so this is load-bearing rather than hygiene: without the entry, make
# would report the directory as up to date and run nothing. Keep this list in step
# with the targets below.
.PHONY: debug release install test bench bench-report sortbench sortbench-report \
        viewcoverage info conan-create conan-upload help
lcov := lcov

f_debug = build_debug
f_release = build_release
f_bench = build_bench
genhtml = genhtml
docscdir = docs

# The unbalanced parenthesis of "project(fimdlp" would break make's own paren
# matching inside $(shell ...), so the pattern is anchored on "project" instead.
version := $(shell grep -A2 "^project" CMakeLists.txt | sed -n 's/.*VERSION \([0-9.]*\).*/\1/p' | head -1)

# Set the number of parallel jobs to the number of available processors minus 7
CPUS := $(shell getconf _NPROCESSORS_ONLN 2>/dev/null \
                 || nproc --all 2>/dev/null \
                 || sysctl -n hw.ncpu)
JOBS := $(shell n=$(CPUS); [ $${n} -gt 7 ] && echo $$((n-7)) || echo 1)

# Colors for output
GREEN = \033[0;32m
YELLOW = \033[1;33m
RED = \033[0;31m
NC = \033[0m # No Color

define build_target
	@echo ">>> Building the project for $(1)..."
	@if [ -d $(2) ]; then rm -fr $(2); fi
	@conan install . --build=missing -of $(2) -s build_type=$(1) $(4)
	@cmake -S . -B $(2) -DCMAKE_TOOLCHAIN_FILE=$(2)/build/$(1)/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=$(1) -D$(3)
	@cmake --build $(2) --config $(1) -j $(JOBS)
endef

define status_file_folder
	@if [ -d $(1) ]; then \
		st1=" ✅ $(GREEN)"; \
	else \
		st1=" ❌ $(RED)"; \
	fi; \
	if [ -f $(1)/libfimdlp.a ]; then \
		st2=" ✅ $(GREEN)"; \
	else \
		st2=" ❌ $(RED)"; \
	fi; \
	printf "  $(YELLOW)$(2):$(NC) $$st1 Folder $(NC)  $$st2 Library $(NC)\n"
endef


debug: ## Build Debug version of the library
	@$(call build_target,"Debug","$(f_debug)", "ENABLE_TESTING=ON", "-o enable_testing=True")

release: ## Build Release version of the library
	@$(call build_target,"Release","$(f_release)", "ENABLE_TESTING=OFF", "-o enable_testing=False")

install: ## Install the library
	@echo ">>> Installing the project..."
	@cmake --build $(f_release) --target install -j $(JOBS)

test: ## Build Debug version and run tests
	@echo ">>> Building Debug version and running tests..."
	@$(MAKE) debug;
	@cp -r tests/datasets $(f_debug)/tests/datasets
	@cd $(f_debug)/tests && ctest --output-on-failure -j $(JOBS)
	@echo ">>> Generating coverage report..."
	@cd $(f_debug)/tests && $(lcov) --capture --directory ../ --demangle-cpp --ignore-errors source,source --ignore-errors mismatch,mismatch --ignore-errors inconsistent,inconsistent --ignore-errors gcov,gcov --output-file coverage.info >/dev/null; \
	$(lcov) --remove coverage.info '/usr/*' 'v1/*' 'lib/*' 'libtorch/*' 'tests/*' 'gtest/*' '*/.conan2/*' '/Applications/*' --ignore-errors unused,unused --output-file coverage.info >/dev/null
	@echo "--- Lcov generate coverage.info"
	@genhtml $(f_debug)/tests/coverage.info --demangle-cpp --output-directory $(f_debug)/tests/coverage --ignore-errors category,category --title "Discretizer mdlp Coverage Report" -s -k -f --legend >/dev/null
	@echo "* Coverage report is generated at $(f_debug)/tests/coverage/index.html"
	@which $(python3) >/dev/null || (echo ">>> Please install python3"; exit 1)
	@if [ ! -f $(f_debug)/tests/coverage.info ]; then \
		echo ">>> No coverage.info file found!"; \
		exit 1; \
	fi
	@echo ">>> Updating coverage badge..."
	@$(python3) scripts/update_coverage.py $(f_debug)/tests
	@echo ">>> Done"  

# Benchmarks
# ----------
# LEVEL=quick stops at n=10,000 (seconds); LEVEL=full adds n=100,000 (minutes).
# LABEL disambiguates machines with the same CPU, e.g. LABEL=studio.
LEVEL ?= full
LABEL ?=
python3 := python3

bench: ## Build and run the benchmarks, storing the result (LEVEL=quick|full, LABEL=name)
	@echo ">>> Building benchmarks (Release)..."
	@if [ -d $(f_bench) ]; then rm -fr $(f_bench); fi
	@conan install . --build=missing -of $(f_bench) -s build_type=Release -o enable_testing=False
	@cmake -S . -B $(f_bench) -DCMAKE_TOOLCHAIN_FILE=$(f_bench)/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_BENCHMARK=ON
	@cmake --build $(f_bench) --config Release -j $(JOBS)
	@echo ">>> Running benchmarks..."
	@$(python3) scripts/benchmarks.py run --level $(LEVEL) $(if $(LABEL),--label $(LABEL),)

bench-report: ## Regenerate the cross-platform benchmark comparison
	@$(python3) scripts/benchmarks.py report

sortbench: ## Toolchain diagnostic: compare std::sort/stable_sort across compilers and stdlibs
	@bash bench/sortbench/run.sh

sortbench-report: ## Regenerate the cross-machine toolchain comparison
	@$(python3) scripts/benchmarks.py sortbench-report

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

info: ## Show project information
	@printf "$(GREEN)FImdlp Library: $(YELLOW)ver. $(version)$(NC)\n"
	@echo ""
	@printf "$(GREEN)Project folders:$(NC)\n"
	$(call status_file_folder, $(f_release), "Build\ Release")
	$(call status_file_folder, $(f_debug), "Build\ Debug\ \ ")
	@echo ""
	@printf "$(GREEN)Build commands:$(NC)\n"
	@printf "   $(YELLOW)make release$(NC) - Build library for release\n"
	@printf "   $(YELLOW)make debug$(NC)   - Build library for debug\n"
	@printf "   $(YELLOW)make test$(NC)    - Run tests\n"
	@printf "   $(YELLOW)Usage:$(NC) make help\n"
	@echo ""
	@printf "   $(YELLOW)Parallel Jobs:   $(GREEN)$(JOBS)$(NC)\n"

# Release runs test_package, which compiles a consumer against the installed
# headers. That is the only check that the *packaged* library is usable, and it
# was being skipped with -tf "" while both it and the package were broken.
# Debug still skips it: one consumer check per create is enough.
conan-create: ## Create the conan packages (Release runs test_package)
	@echo ">>> Creating the conan package (Release)..."
	conan create . --build=missing -s:a build_type=Release
	@echo ">>> Creating the conan package (Debug)..."
	conan create . --build=missing -tf "" -s:a build_type=Debug -o "&:enable_testing=False"
	@echo ">>> Done"

conan-upload: ## Upload the created packages to the Cimmeria remote
	@if [ -z "$(version)" ]; then echo ">>> Could not read the version from CMakeLists.txt"; exit 1; fi; \
	if ! conan remote list | grep -q "Cimmeria"; then \
		echo ">>> Remote 'Cimmeria' is not configured. To set it up:"; \
		echo "    conan remote add Cimmeria https://conan.rmontanana.es/artifactory/api/conan/Cimmeria"; \
		echo "    conan remote login Cimmeria <username>"; \
		exit 1; \
	fi; \
	echo ">>> Uploading fimdlp/$(version) to Cimmeria..."; \
	conan upload fimdlp/$(version) --remote=Cimmeria; \
	echo ">>> Done"

help: ## Show help message
	@IFS=$$'\n' ; \
	help_lines=(`grep -Fh "##" $(MAKEFILE_LIST) | grep -Fv fgrep | sed -e 's/\\$$//' | sed -e 's/##/:/'`); \
	printf "%s\n" "FImdlp library v$(version)"; \
	printf "%s\n\n" "Discretization algorithms (MDLP, k-bins, PKID) for continuous attributes"; \
	printf "%s\n\n" "Usage: make [task]"; \
	printf "%-20s %s\n" "task" "help" ; \
	printf "%-20s %s\n" "------" "----" ; \
	for help_line in $${help_lines[@]}; do \
		IFS=$$':' ; \
		help_split=($$help_line) ; \
		help_command=`echo $${help_split[0]} | sed -e 's/^ *//' -e 's/ *$$//'` ; \
		help_info=`echo $${help_split[2]} | sed -e 's/^ *//' -e 's/ *$$//'` ; \
		printf '\033[36m'; \
		printf "%-20s %s" $$help_command ; \
		printf '\033[0m'; \
		printf "%s\n" $$help_info; \
	done
