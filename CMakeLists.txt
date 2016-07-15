cmake_minimum_required(VERSION 2.8)


project(KlokPOS)

add_subdirectory("lib/SQLiteCpp")

set(CMAKE_C_COMPILER "/home/klok/GL14-SDK/TOOL_CHAIN/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin//arm-none-linux-gnueabi-gcc")
set(CMAKE_CXX_COMPILER "/home/klok/GL14-SDK/TOOL_CHAIN/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin//arm-none-linux-gnueabi-g++")

add_executable(firmware src/main.cpp)