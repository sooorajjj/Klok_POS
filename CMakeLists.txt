cmake_minimum_required(VERSION 2.8)

cmake_policy(SET CMP0015 NEW)
project(KlokPOS)

include_directories(
  "lib/visiontek/inc"
  "lib/SQLiteCpp/include"
  "include")

message("Linker Search Path " "${CMAKE_SOURCE_DIR}lib/visiontek/lib")

link_directories(
  "lib/visiontek/lib"
  )

add_subdirectory("lib/SQLiteCpp")
add_subdirectory("lib/visiontek")


set(CMAKE_C_COMPILER "/home/klok/GL14-SDK/TOOL_CHAIN/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin//arm-none-linux-gnueabi-gcc")
set(CMAKE_CXX_COMPILER "/home/klok/GL14-SDK/TOOL_CHAIN/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin//arm-none-linux-gnueabi-g++")

add_executable(firmware src/main.cpp src/Visiontek.cpp src/main.cpp src/POSDS.cpp)
target_link_libraries(firmware SQLiteCpp sqlite3 pthread dl GL14epos visiontekGlue)
