cmake_minimum_required(VERSION 3.30)

project(Skirnir)

set(CMAKE_CXX_STANDARD 23)

find_package(tsl-hopscotch-map REQUIRED)

add_library(Skirnir
        src/ServiceScope.cpp
        src/ServiceProvider.cpp)

target_include_directories(Skirnir PUBLIC include)

target_link_libraries(Skirnir PUBLIC tsl::hopscotch_map)

install(TARGETS Skirnir)

install(DIRECTORY include/
        DESTINATION include
)

option(BUILD_SKIRNIR_EXAMPLES OFF CACHE)

if (${BUILD_SKIRNIR_EXAMPLES})
    add_subdirectory(examples)
endif ()