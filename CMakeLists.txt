cmake_minimum_required(VERSION 3.20)

project(fimdlp
    VERSION 2.1.2
    LANGUAGES CXX
    DESCRIPTION "Discretization algorithm based on the paper by Fayyad & Irani Multi-Interval Discretization of Continuous-Valued Attributes for Classification Learning."
    HOMEPAGE_URL "https://github.com/rmontanana/mdlp"
)
set(CMAKE_CXX_STANDARD 17)
cmake_policy(SET CMP0135 NEW)

# Find dependencies
find_package(Torch CONFIG REQUIRED)

# Options
# -------
option(ENABLE_TESTING OFF)
option(COVERAGE       OFF)

add_subdirectory(config)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-elide-constructors")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-default-inline")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug mode")
else()
    message(STATUS "Release mode")
endif()

if (ENABLE_TESTING)
    message(STATUS "Testing is enabled")
    enable_testing()
    set(CODE_COVERAGE ON)
    set(GCC_COVERAGE_LINK_FLAGS "${GCC_COVERAGE_LINK_FLAGS} -lgcov --coverage")
    add_subdirectory(tests)
else()
    message(STATUS "Testing is disabled")
endif()

message(STATUS "Building sample")
add_subdirectory(sample)

include_directories(
    ${fimdlp_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/configured_files/include
)

add_library(fimdlp src/CPPFImdlp.cpp src/Metrics.cpp src/BinDisc.cpp src/Discretizer.cpp)
target_link_libraries(fimdlp PRIVATE torch::torch)

# Installation
# ------------
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/fimdlpConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(TARGETS fimdlp
        EXPORT fimdlpTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)

install(DIRECTORY src/ DESTINATION include/fimdlp FILES_MATCHING PATTERN "*.h")
install(FILES ${CMAKE_BINARY_DIR}/configured_files/include/config.h DESTINATION include/fimdlp)

install(EXPORT fimdlpTargets
        FILE fimdlpTargets.cmake
        NAMESPACE fimdlp::
        DESTINATION lib/cmake/fimdlp)

configure_file(fimdlpConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/fimdlpConfig.cmake" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fimdlpConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/fimdlpConfigVersion.cmake"
        DESTINATION lib/cmake/fimdlp)

