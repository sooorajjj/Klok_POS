cmake_minimum_required(VERSION 2.8)

cmake_policy(SET CMP0015 NEW)
project(KlokPOS)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv5te -mtune=arm926ej-s -msoft-float -mfloat-abi=soft")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv5te -mtune=arm926ej-s -msoft-float -mfloat-abi=soft")

include_directories(
  "lib/visiontek/inc"
  "lib/SQLiteCpp/include"
  "include")
if(NOT DEFINED ENV{VISIONTEK_CC})
    message(FATAL_ERROR "VISIONTEK_CC environment variable not defined")

else()
	message(STATUS "VISIONTEK_CC:" $ENV{VISIONTEK_CC})
endif()

if(NOT DEFINED ENV{VISIONTEK_CXX})
    message(FATAL_ERROR "VISIONTEK_CXX environment variable not defined")
else()
	message(STATUS "VISIONTEK_CXX:" $ENV{VISIONTEK_CXX})
endif()

message("Linker Search Path " "${CMAKE_SOURCE_DIR}lib/visiontek/lib")

link_directories(
  "lib/visiontek/lib"
  )

add_subdirectory("lib/SQLiteCpp")
add_subdirectory("lib/visiontek")

#export VISIONTEK_CC="/home/klok/GL14-SDK/TOOL_CHAIN/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin//arm-none-linux-gnueabi-gcc"
#export VISIONTEK_CXX="/home/klok/GL14-SDK/TOOL_CHAIN/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin//arm-none-linux-gnueabi-g++"


set(CMAKE_C_COMPILER $ENV{VISIONTEK_CC})
set(CMAKE_CXX_COMPILER $ENV{VISIONTEK_CXX})
set(CMAKE_BUILD_TYPE MinSizeRel)

# set(CMAKE_C_COMPILER "/home/klok/GL14-SDK/TOOL_CHAIN/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin//arm-none-linux-gnueabi-gcc")
# set(CMAKE_CXX_COMPILER "/home/klok/GL14-SDK/TOOL_CHAIN/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin//arm-none-linux-gnueabi-g++")

add_executable(firmware src/main.cpp src/Visiontek.cpp src/main.cpp src/POSDS.cpp)
target_link_libraries(firmware SQLiteCpp sqlite3 pthread dl GL14epos visiontekGlue)
