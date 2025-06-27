cmake_minimum_required(VERSION 3.20)
project(test_fimdlp)

set(CMAKE_CXX_STANDARD 17)

find_package(fimdlp REQUIRED)

add_executable(test_fimdlp test_fimdlp.cpp)
target_link_libraries(test_fimdlp fimdlp::fimdlp)