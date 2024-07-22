cmake_minimum_required(VERSION 3.20)

project(fimdlp)
set(CMAKE_CXX_STANDARD 17)
cmake_policy(SET CMP0135 NEW)

find_package(Torch CONFIG REQUIRED)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}  -fno-elide-constructors")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-default-inline")
endif()

if (ENABLE_TESTING)
    MESSAGE("Debug mode")
    enable_testing()
    set(CODE_COVERAGE ON)
    SET(GCC_COVERAGE_LINK_FLAGS " ${GCC_COVERAGE_LINK_FLAGS} -lgcov --coverage")
    add_subdirectory(tests)
else(ENABLE_TESTING)
    MESSAGE("Release mode")
endif(ENABLE_TESTING)

if (ENABLE_SAMPLE)
    message("Building sample")
    add_subdirectory(sample)
endif(ENABLE_SAMPLE)

include_directories(
    ${TORCH_INCLUDE_DIRS}
    ${fimdlp_SOURCE_DIR}/src
)

add_library(fimdlp src/CPPFImdlp.cpp src/Metrics.cpp src/BinDisc.cpp src/Discretizer.cpp)
target_link_libraries(fimdlp "${TORCH_LIBRARIES}")