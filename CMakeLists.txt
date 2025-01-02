cmake_minimum_required(VERSION 3.30)

project(Skirnir)

set(CMAKE_CXX_STANDARD 23)

add_library(skirnir
        src/ServiceScope.cpp
        src/ServiceProvider.cpp)

target_include_directories(skirnir PUBLIC include)

install(TARGETS skirnir)

install(DIRECTORY include/
        DESTINATION include
)

option(BUILD_SKIRNIR_EXAMPLES OFF CACHE)

if (${BUILD_SKIRNIR_EXAMPLES})
    add_subdirectory(examples)
endif ()