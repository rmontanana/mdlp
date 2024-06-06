cmake_minimum_required(VERSION 3.20)
project(mdlp)

set(CMAKE_CXX_STANDARD 11)

add_library(mdlp CPPFImdlp.cpp Metrics.cpp BinDisc.cpp)
add_subdirectory(sample)
add_subdirectory(tests)

