cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 26)

add_executable(SkirnirBench LoggingBench.cpp)
target_link_libraries(SkirnirBench skirnir::skirnir)
