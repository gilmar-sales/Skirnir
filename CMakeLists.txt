cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


project(Skirnir LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)

if (NOT TARGET skirnir)
    add_subdirectory(vendor/fmt)

    add_library(skirnir
            src/ServiceScope.cpp
            src/ServiceProvider.cpp)

    add_library(skirnir::skirnir ALIAS skirnir)

    target_link_libraries(skirnir fmt::fmt)
endif ()

target_include_directories(skirnir PUBLIC include)

install(TARGETS skirnir)

install(DIRECTORY include/
        DESTINATION include
)

option(BUILD_SKIRNIR_EXAMPLES OFF CACHE)

if (${BUILD_SKIRNIR_EXAMPLES})
    add_subdirectory(examples)
endif ()

option(BUILD_SKIRNIR_TESTS OFF CACHE)

if (${BUILD_SKIRNIR_TESTS})
    add_subdirectory(test)
endif ()
