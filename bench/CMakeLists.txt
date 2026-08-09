set(CMAKE_CXX_STANDARD 17)

include_directories(
    ${fimdlp_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/configured_files/include
)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PRIVATE fimdlp torch::torch)
