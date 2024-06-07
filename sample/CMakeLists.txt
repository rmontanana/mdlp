set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Debug)

add_executable(sample sample.cpp ../tests/ArffFiles.cpp)
target_link_libraries(sample mdlp "${TORCH_LIBRARIES}")
