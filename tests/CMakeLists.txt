# Check if we should use Catch2 from Conan or GoogleTest via FetchContent
find_package(Catch2 3 QUIET)

if(Catch2_FOUND)
    message(STATUS "Using Catch2 from Conan")
    set(TEST_FRAMEWORK "Catch2")
else()
    message(STATUS "Using GoogleTest via FetchContent")
    set(TEST_FRAMEWORK "GoogleTest")
    include(FetchContent)
    include_directories(${GTEST_INCLUDE_DIRS})
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

include_directories(
        ${libtorch_INCLUDE_DIRS_DEBUG}
        ${fimdlp_SOURCE_DIR}/src
        ${fimdlp_SOURCE_DIR}/tests/lib/Files    
        ${CMAKE_BINARY_DIR}/configured_files/include
)

add_executable(Metrics_unittest ${fimdlp_SOURCE_DIR}/src/Metrics.cpp Metrics_unittest.cpp)
target_link_libraries(Metrics_unittest GTest::gtest_main)
target_compile_options(Metrics_unittest PRIVATE --coverage)
target_link_options(Metrics_unittest PRIVATE --coverage)

add_executable(FImdlp_unittest FImdlp_unittest.cpp
${fimdlp_SOURCE_DIR}/src/CPPFImdlp.cpp ${fimdlp_SOURCE_DIR}/src/Metrics.cpp  ${fimdlp_SOURCE_DIR}/src/Discretizer.cpp)
target_link_libraries(FImdlp_unittest GTest::gtest_main "${TORCH_LIBRARIES}")
target_compile_options(FImdlp_unittest PRIVATE --coverage)
target_link_options(FImdlp_unittest PRIVATE --coverage)

add_executable(BinDisc_unittest BinDisc_unittest.cpp ${fimdlp_SOURCE_DIR}/src/BinDisc.cpp  ${fimdlp_SOURCE_DIR}/src/Discretizer.cpp)
target_link_libraries(BinDisc_unittest GTest::gtest_main "${TORCH_LIBRARIES}")
target_compile_options(BinDisc_unittest PRIVATE --coverage)
target_link_options(BinDisc_unittest PRIVATE --coverage)

add_executable(Discretizer_unittest Discretizer_unittest.cpp
${fimdlp_SOURCE_DIR}/src/BinDisc.cpp ${fimdlp_SOURCE_DIR}/src/CPPFImdlp.cpp ${fimdlp_SOURCE_DIR}/src/Metrics.cpp ${fimdlp_SOURCE_DIR}/src/Discretizer.cpp )
target_link_libraries(Discretizer_unittest GTest::gtest_main "${TORCH_LIBRARIES}")
target_compile_options(Discretizer_unittest PRIVATE --coverage)
target_link_options(Discretizer_unittest PRIVATE --coverage)

include(GoogleTest)

gtest_discover_tests(Metrics_unittest)
gtest_discover_tests(FImdlp_unittest)
gtest_discover_tests(BinDisc_unittest)
gtest_discover_tests(Discretizer_unittest)