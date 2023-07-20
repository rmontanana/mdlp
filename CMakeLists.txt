cmake_minimum_required(VERSION 3.20)
project(mdlp)

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

set(CMAKE_CXX_STANDARD 11)

add_library(mdlp CPPFImdlp.cpp Metrics.cpp)
add_subdirectory(sample)
add_subdirectory(tests)

