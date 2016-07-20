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
include_directories("inc")
add_library(visiontekGlue STATIC src/scroll_menu.c)