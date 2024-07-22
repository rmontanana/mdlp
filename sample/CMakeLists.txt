set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Debug)

include_directories(
    ${fimdlp_SOURCE_DIR}/src
    ${fimdlp_SOURCE_DIR}/tests/lib/Files
)

add_executable(sample sample.cpp )
target_link_libraries(sample fimdlp "${TORCH_LIBRARIES}")
