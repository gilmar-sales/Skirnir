cmake_minimum_required(VERSION 3.30)

project(Skirnir)

set(CMAKE_CXX_STANDARD 23)

find_package(tsl-hopscotch-map REQUIRED)

add_library(Skirnir
        include/ServiceCollection.hpp
        include/Reflection.hpp
        include/ServiceId.hpp
        include/ServiceProvider.hpp
        include/ServiceScope.hpp
        include/Common.hpp
        src/ServiceScope.cpp
        src/ServiceProvider.cpp
        include/Skirnir.hpp)

target_include_directories(Skirnir PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(Skirnir tsl::hopscotch_map)

option(BUILD_SKIRNIR_EXAMPLES OFF CACHE)

if (${BUILD_SKIRNIR_EXAMPLES})
    add_subdirectory(examples)
endif ()