cmake_minimum_required(VERSION 3.30)

project(SkirnirTest LANGUAGES C CXX VERSION 0.15.2)

set(CMAKE_CXX_STANDARD 26)

FetchContent_Declare(
        gtest
        GIT_REPOSITORY "https://github.com/google/googletest"
        GIT_TAG "v1.17.0"
)
FetchContent_MakeAvailable(gtest)

include(GoogleTest)

file(GLOB_RECURSE TESTS_SRC
        "src/*.cpp"
)

add_executable(SkirnirTest_run ${TESTS_SRC})

target_link_libraries(SkirnirTest_run skirnir::skirnir gtest gtest_main)

if(SKIRNIR_ENABLE_ASIO)
  target_compile_definitions(SkirnirTest_run PRIVATE SKIRNIR_ASIO_ENABLED=1)
endif()

gtest_discover_tests(SkirnirTest_run)
