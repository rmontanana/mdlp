include(FetchContent)
include_directories(${GTEST_INCLUDE_DIRS})
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include_directories(
        ${TORCH_INCLUDE_DIRS}
        ${mdlp_SOURCE_DIR}/src
        ${mdlp_SOURCE_DIR}/tests/lib/Files
)

add_executable(Metrics_unittest ${mdlp_SOURCE_DIR}/src/Metrics.cpp Metrics_unittest.cpp)
target_link_libraries(Metrics_unittest GTest::gtest_main)
target_compile_options(Metrics_unittest PRIVATE --coverage)
target_link_options(Metrics_unittest PRIVATE --coverage)

add_executable(FImdlp_unittest FImdlp_unittest.cpp
${mdlp_SOURCE_DIR}/src/CPPFImdlp.cpp ${mdlp_SOURCE_DIR}/src/Metrics.cpp  ${mdlp_SOURCE_DIR}/src/Discretizer.cpp)
target_link_libraries(FImdlp_unittest GTest::gtest_main "${TORCH_LIBRARIES}")
target_compile_options(FImdlp_unittest PRIVATE --coverage)
target_link_options(FImdlp_unittest PRIVATE --coverage)

add_executable(BinDisc_unittest BinDisc_unittest.cpp ${mdlp_SOURCE_DIR}/src/BinDisc.cpp  ${mdlp_SOURCE_DIR}/src/Discretizer.cpp)
target_link_libraries(BinDisc_unittest GTest::gtest_main "${TORCH_LIBRARIES}")
target_compile_options(BinDisc_unittest PRIVATE --coverage)
target_link_options(BinDisc_unittest PRIVATE --coverage)

add_executable(Discretizer_unittest Discretizer_unittest.cpp
${mdlp_SOURCE_DIR}/src/BinDisc.cpp ${mdlp_SOURCE_DIR}/src/CPPFImdlp.cpp ${mdlp_SOURCE_DIR}/src/Metrics.cpp ${mdlp_SOURCE_DIR}/src/Discretizer.cpp )
target_link_libraries(Discretizer_unittest GTest::gtest_main "${TORCH_LIBRARIES}")
target_compile_options(Discretizer_unittest PRIVATE --coverage)
target_link_options(Discretizer_unittest PRIVATE --coverage)

include(GoogleTest)

gtest_discover_tests(Metrics_unittest)
gtest_discover_tests(FImdlp_unittest)
gtest_discover_tests(BinDisc_unittest)
gtest_discover_tests(Discretizer_unittest)