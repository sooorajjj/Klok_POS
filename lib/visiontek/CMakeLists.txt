#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv5te -mtune=arm926ej-s -msoft-float -mfloat-abi=soft")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv5te -mtune=arm926ej-s -msoft-float -mfloat-abi=soft")

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


set(CMAKE_C_COMPILER $ENV{VISIONTEK_CC})
set(CMAKE_CXX_COMPILER $ENV{VISIONTEK_CXX})
set(CMAKE_BUILD_TYPE MinSizeRel)
include_directories("inc")
add_library(visiontekGlue STATIC src/scroll_menu.c)