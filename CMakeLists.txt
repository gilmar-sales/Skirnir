cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
set(CMAKE_CXX_MODULE_STD 1)

project(Skirnir LANGUAGES CXX VERSION 0.15.3)

set(CMAKE_CXX_STANDARD 23)

option(SKIRNIR_USE_FMT OFF CACHE)
option(BUILD_SKIRNIR_EXAMPLES OFF CACHE)
option(BUILD_SKIRNIR_TESTS OFF CACHE)

file(GLOB_RECURSE SKIRNIR_SOURCE_FILES src/**.cpp)
file(GLOB_RECURSE SKIRNIR_MODULE_FILES src/**.cpp)

if (NOT TARGET skirnir)

    include(FetchContent)

    add_library(skirnir)

    target_compile_features(skirnir
        PRIVATE cxx_std_23
        PUBLIC cxx_std_20)

    target_sources(skirnir
            PUBLIC
            FILE_SET all_my_modules TYPE CXX_MODULES FILES
            ${SKIRNIR_MODULE_FILES}
    )

    add_library(skirnir::skirnir ALIAS skirnir)

    if (${SKIRNIR_USE_FMT})
        target_compile_definitions(skirnir PUBLIC -DSKIRNIR_USE_FMT)

        FetchContent_Declare(
                fmt
                GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
                GIT_TAG "11.1.4"
        )
        FetchContent_MakeAvailable(fmt)

        target_link_libraries(skirnir fmt::fmt)
    elseif (CMAKE_COMPILER_IS_GNUCXX)
        target_link_libraries(skirnir stdc++exp)
    endif ()
endif ()

target_include_directories(skirnir PUBLIC include)

install(TARGETS skirnir)

install(DIRECTORY include/
        DESTINATION include
)

if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    set(BUILD_SKIRNIR_EXAMPLES ON)
    # set(BUILD_SKIRNIR_TESTS ON)
endif ()

if (${BUILD_SKIRNIR_EXAMPLES})
    add_subdirectory(examples)
endif ()

if (${BUILD_SKIRNIR_TESTS})
    add_subdirectory(test)
endif ()
