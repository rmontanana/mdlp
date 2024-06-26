cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 17)
cmake_policy(SET CMP0135 NEW)
include(FetchContent)
include_directories(${GTEST_INCLUDE_DIRS})
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

find_package(Torch REQUIRED)

enable_testing()

include_directories(${TORCH_INCLUDE_DIRS})

add_executable(Metrics_unittest ../Metrics.cpp Metrics_unittest.cpp)
target_link_libraries(Metrics_unittest GTest::gtest_main)
target_compile_options(Metrics_unittest PRIVATE --coverage)
target_link_options(Metrics_unittest PRIVATE --coverage)

add_executable(FImdlp_unittest ../CPPFImdlp.cpp ArffFiles.cpp ../Metrics.cpp FImdlp_unittest.cpp ../Discretizer.cpp)
target_link_libraries(FImdlp_unittest GTest::gtest_main "${TORCH_LIBRARIES}")
target_compile_options(FImdlp_unittest PRIVATE --coverage)
target_link_options(FImdlp_unittest PRIVATE --coverage)

add_executable(BinDisc_unittest ../BinDisc.cpp ArffFiles.cpp BinDisc_unittest.cpp ../Discretizer.cpp)
target_link_libraries(BinDisc_unittest GTest::gtest_main "${TORCH_LIBRARIES}")
target_compile_options(BinDisc_unittest PRIVATE --coverage)
target_link_options(BinDisc_unittest PRIVATE --coverage)

add_executable(Discretizer_unittest ../BinDisc.cpp ../CPPFImdlp.cpp ArffFiles.cpp ../Metrics.cpp ../Discretizer.cpp Discretizer_unittest.cpp)
target_link_libraries(Discretizer_unittest GTest::gtest_main "${TORCH_LIBRARIES}")
target_compile_options(Discretizer_unittest PRIVATE --coverage)
target_link_options(Discretizer_unittest PRIVATE --coverage)

include(GoogleTest)

gtest_discover_tests(Metrics_unittest)
gtest_discover_tests(FImdlp_unittest)
gtest_discover_tests(BinDisc_unittest)
gtest_discover_tests(Discretizer_unittest)