cmake_minimum_required(VERSION 3.30)
project(IoC)

set(CMAKE_CXX_STANDARD 23)

add_executable(IoC src/main.cpp
        src/ServiceCollection.hpp
        src/Reflection.hpp
        src/ServiceId.hpp
        src/ServiceProvider.hpp
        src/ServiceScope.hpp
        src/Common.hpp
        src/ServiceScope.cpp
        src/ServiceProvider.cpp)
