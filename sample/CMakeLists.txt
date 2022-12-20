cmake_minimum_required(VERSION 3.20)
project(main)

set(CMAKE_CXX_STANDARD 14)

add_executable(sample sample.cpp ArffFiles.cpp ../Metrics.cpp ../CPPFImdlp.cpp)
add_executable(test test.cpp ArffFiles.cpp ../Metrics.cpp ../CPPFImdlp.cpp)
