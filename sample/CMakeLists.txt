set(CMAKE_CXX_STANDARD 11)

set(CMAKE_BUILD_TYPE Debug)

add_executable(sample sample.cpp ../tests/ArffFiles.cpp ../Metrics.cpp ../CPPFImdlp.cpp)
