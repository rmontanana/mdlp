cmake_minimum_required(VERSION 3.20)
project(mdlp)

set(CMAKE_CXX_STANDARD 14)

add_library(mdlp CPPFImdlp.cpp Metrics.cpp)

