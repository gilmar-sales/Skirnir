cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


project(Skirnir LANGUAGES CXX VERSION 0.16.1)

set(CMAKE_CXX_STANDARD 26)

option(SKIRNIR_USE_FMT "Use the fmt library for logging" OFF)
option(SKIRNIR_ENABLE_ASIO "Enable the Asio adapter and asio-based examples/tests" ON)

if(NOT TARGET skirnir)

  include(FetchContent)

  file(GLOB_RECURSE SKIRNIR_SRC src/**.cpp)

  add_library(skirnir ${SKIRNIR_SRC})

  add_library(skirnir::skirnir ALIAS skirnir)

  target_compile_definitions(skirnir PUBLIC -DSKIRNIR_USE_SIMDJSON)

  FetchContent_Declare(
    simdjson
    GIT_REPOSITORY "https://github.com/simdjson/simdjson.git"
    GIT_TAG "812f8823d233a3189a06d6c117e1794d9c34fced"
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

  if(${SKIRNIR_USE_FMT})
    target_compile_definitions(skirnir PUBLIC -DSKIRNIR_USE_FMT)

    FetchContent_Declare(
      fmt
      GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
      GIT_TAG "12.1.0"
    )
    FetchContent_MakeAvailable(fmt)

    target_link_libraries(skirnir fmt::fmt)
  elseif(CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries(skirnir stdc++exp)
  endif()

  if(SKIRNIR_ENABLE_ASIO)
    FetchContent_Declare(
      asiocmake
      GIT_REPOSITORY "https://github.com/gilmar-sales/asio.cmake.git"
      GIT_TAG "main"
    )
    set(ASIO_STANDALONE ON CACHE BOOL "" FORCE)
    set(ASIO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASIO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(asiocmake)
    target_link_libraries(skirnir asio::asio)
    if(WIN32 AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
      target_link_libraries(skirnir ws2_32 wsock32 Bcrypt)
    endif()
  endif()
endif()

target_include_directories(skirnir PUBLIC include)

target_precompile_headers(skirnir PUBLIC <Skirnir/Common.hpp>)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(skirnir PUBLIC -freflection)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(skirnir PUBLIC /experimental:reflection)
endif()

install(TARGETS skirnir)

install(DIRECTORY include/
  DESTINATION include
)

option(BUILD_SKIRNIR_EXAMPLES OFF CACHE)
option(BUILD_SKIRNIR_TESTS OFF CACHE)

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  set(BUILD_SKIRNIR_EXAMPLES ON)
  set(BUILD_SKIRNIR_TESTS ON)
endif()

if(${BUILD_SKIRNIR_EXAMPLES})
  add_subdirectory(examples)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/async/CMakeLists.txt")
    add_subdirectory(examples/async)
  endif()
endif()

if(${BUILD_SKIRNIR_TESTS})
  enable_testing()
  add_subdirectory(test)
endif()
