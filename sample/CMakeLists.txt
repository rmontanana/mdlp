set(CMAKE_CXX_STANDARD 17)

find_package(arff-files REQUIRED)

include_directories(
    ${fimdlp_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/configured_files/include
    ${arff-files_INCLUDE_DIRS}
)

add_executable(sample sample.cpp)
target_link_libraries(sample PRIVATE fimdlp torch::torch arff-files::arff-files)
