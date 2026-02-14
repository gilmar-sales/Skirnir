# Find Google Benchmark
SET(BENCHMARK_ENABLE_TESTING OFF)
find_package(benchmark QUIET)

if(NOT benchmark_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.5
    )
    FetchContent_MakeAvailable(googlebenchmark)
endif()

# Create benchmark executable
add_executable(ioc_benchmark Benchmark.cpp)

# Link against Google Benchmark
target_link_libraries(ioc_benchmark benchmark::benchmark skirnir::skirnir)

# Enable link-time optimization if available
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT error)
if(ipo_supported)
    set_property(TARGET ioc_benchmark PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Install target
install(TARGETS ioc_benchmark DESTINATION bin)
