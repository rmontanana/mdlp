cmake_minimum_required(VERSION 3.20)
project(test_fimdlp)

find_package(fimdlp REQUIRED)

add_executable(test_fimdlp src/test_fimdlp.cpp)
target_link_libraries(test_fimdlp fimdlp::fimdlp)
target_compile_features(test_fimdlp PRIVATE cxx_std_17)