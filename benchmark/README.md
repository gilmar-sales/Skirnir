# Benchmark Suite for IoC Container

This directory contains a comprehensive benchmark suite for measuring the performance characteristics of C++ IoC containers.

## Files

- **Benchmark.cpp** - Main benchmark file using Google Benchmark
- **measure_compile_time.py** - Python script to measure compile-time impact
- **CMakeLists.txt** - Build configuration (optional)

## Building

### Prerequisites

- C++20 compatible compiler
- Google Benchmark library installed
- CMake 3.14+ (optional)

### Build with CMake

```bash
mkdir build && cd build
cmake ..
make
```

### Build manually

```bash
# GCC/Clang
g++ -std=c++20 -O3 -isystem /usr/local/include \
    -L/usr/local/lib -lbenchmark -lpthread \
    Benchmark.cpp -o ioc_benchmark

# MSVC
cl /std:c++20 /O2 /I"C:\Program Files\benchmark\include" \
   Benchmark.cpp /link /LIBPATH:"C:\Program Files\benchmark\lib" benchmark.lib
```

## Running Benchmarks

### Runtime Benchmarks

```bash
./ioc_benchmark
```

Filter specific benchmarks:
```bash
./ioc_benchmark --benchmark_filter="Manual"
./ioc_benchmark --benchmark_filter="IoC/Depth"
```

### Compile-Time Benchmarks

Basic usage:
```bash
python3 measure_compile_time.py --container-header "Skirnir.hpp" \
    --compiler g++ --registrations "0,10,50,100,500"
```

Generate a standalone test file with 100 registrations:
```bash
python3 measure_compile_time.py --generate-100-regs-file
```

## Benchmark Categories

1. **Manual Injection** - Baseline measurements using `std::make_shared`
2. **IoC Simple** - Single service resolution
3. **IoC Deep Graph** - 3-level dependency hierarchy
4. **Depth Scaling** - Measuring overhead as dependency depth increases
5. **Batch Resolution** - Multiple resolutions in a loop
6. **Lifetime Comparison** - Transient vs Singleton performance

## Key Metrics

### Runtime Performance

- **Resolution Latency**: Time to resolve a service
- **Overhead Factor**: IoC time / Manual time
- **Scaling**: Performance vs dependency depth

### Compile-Time Performance

- **Base Compile Time**: Time with 0 registrations
- **Incremental Cost**: Additional time per registration
- **Binary Size**: Impact on executable size

## Interpreting Results

### Good Results

- IoC overhead < 2x manual injection
- Linear scaling with depth (not exponential)
- < 10ms compile time per 100 registrations

### Warning Signs

- > 5x overhead vs manual
- Exponential time increase with depth
- > 100ms compile time per 100 registrations
- Large binary bloat (> 10KB per 100 registrations)

## Customizing for Your Container

Replace the placeholder `IoCContainer` class in Benchmark.cpp with your actual container:

```cpp
// Replace this:
class IoCContainer {
    // ... placeholder implementation
};

// With your actual container:
#include "YourContainer.hpp"
using IoCContainer = Your::Actual::Container;
```

## Example Output

```
Run on (8 X 2400 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 2.34, 2.45, 2.12
------------------------------------------------------------------------------
Benchmark                                    Time             CPU   Iterations
------------------------------------------------------------------------------
Manual/Simple_Transient                     15 ns           15 ns     100000
IoC/Simple_Transient                        45 ns           45 ns     100000
Manual/Transient_3Level                     65 ns           65 ns     100000
IoC/DeepGraph_Transient_3Level             180 ns          180 ns     100000
IoC/DepthScaling/Level1                     42 ns           42 ns     100000
IoC/DepthScaling/Level2                     85 ns           85 ns     100000
IoC/DepthScaling/Level3                    175 ns          175 ns     100000
```

## Notes

- Use `benchmark::DoNotOptimize()` to prevent compiler optimizations from skewing results
- Run benchmarks multiple times for consistent results
- Consider CPU frequency scaling and thermal throttling
- Use `taskset` or similar to pin to specific cores for consistent results
