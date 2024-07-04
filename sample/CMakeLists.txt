set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Debug)

include_directories(
    ${mdlp_SOURCE_DIR}/src
    ${mdlp_SOURCE_DIR}/tests/lib/Files
)

add_executable(sample sample.cpp )
target_link_libraries(sample mdlp "${TORCH_LIBRARIES}")
