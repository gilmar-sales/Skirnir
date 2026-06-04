cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


project(Skirnir LANGUAGES CXX VERSION 0.16.1)

set(CMAKE_CXX_STANDARD 26)

option(SKIRNIR_USE_FMT "Use the fmt library for logging" OFF)
option(SKIRNIR_USE_SIMDJSON "Use simdjson for configuration parsing" ON)

if (NOT TARGET skirnir)

  include(FetchContent)

  add_library(skirnir
            src/Configuration.cpp
            src/Logger.cpp
            src/ServiceScope.cpp
            src/ServiceProvider.cpp)

  add_library(skirnir::skirnir ALIAS skirnir)

  if (${SKIRNIR_USE_SIMDJSON})
    target_compile_definitions(skirnir PUBLIC -DSKIRNIR_USE_SIMDJSON)

    FetchContent_Declare(
                simdjson
                GIT_REPOSITORY "https://github.com/simdjson/simdjson.git"
                GIT_TAG "v4.6.4"
                GIT_SHALLOW TRUE
        )
    set(SIMDJSON_DEVELOPER_MODE OFF CACHE BOOL "" FORCE)
    set(SIMDJSON_BUILD_STATIC_LIB OFF CACHE BOOL "" FORCE)
    set(SIMDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(SIMDJSON_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
    set(SIMDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(simdjson)

    target_link_libraries(skirnir simdjson::simdjson)
    target_include_directories(skirnir PRIVATE ${simdjson_SOURCE_DIR}/include)
  endif ()

  if (${SKIRNIR_USE_FMT})
    target_compile_definitions(skirnir PUBLIC -DSKIRNIR_USE_FMT)

    FetchContent_Declare(
                fmt
                GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
                GIT_TAG "12.1.0"
        )
    FetchContent_MakeAvailable(fmt)

    target_link_libraries(skirnir fmt::fmt)
  elseif (CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries(skirnir stdc++exp)
  endif ()
endif ()

target_include_directories(skirnir PUBLIC include)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(skirnir PUBLIC -freflection)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(skirnir PUBLIC /experimental:reflection)
endif ()

install(TARGETS skirnir)

install(DIRECTORY include/
        DESTINATION include
)

option(BUILD_SKIRNIR_EXAMPLES OFF CACHE)
option(BUILD_SKIRNIR_TESTS OFF CACHE)

if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  set(BUILD_SKIRNIR_EXAMPLES ON)
  set(BUILD_SKIRNIR_TESTS ON)
endif ()

if (${BUILD_SKIRNIR_EXAMPLES})
  add_subdirectory(examples)
endif ()

if (${BUILD_SKIRNIR_TESTS})
  add_subdirectory(test)
endif ()
