cmake_minimum_required(VERSION 3.30)

project(SkirnirTest CXX)

set(CMAKE_CXX_STANDARD 23)

FetchContent_Declare(
  gtest
  GIT_REPOSITORY "https://github.com/google/googletest"
  GIT_TAG        "v1.16.0"
)
FetchContent_MakeAvailable(gtest)

enable_testing()

file(GLOB_RECURSE TESTS_SRC
        "src/*.cpp"
)

add_executable(SkirnirTest_run ${TESTS_SRC}
        src/ServiceProviderSpec.cpp
        src/ServiceScopeSpec.cpp)

target_link_libraries(SkirnirTest_run skirnir::skirnir gtest gtest_main)
