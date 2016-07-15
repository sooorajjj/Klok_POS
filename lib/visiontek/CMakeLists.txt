set(CMAKE_C_COMPILER "/home/klok/GL14-SDK/TOOL_CHAIN/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin//arm-none-linux-gnueabi-gcc")
set(CMAKE_CXX_COMPILER "/home/klok/GL14-SDK/TOOL_CHAIN/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin//arm-none-linux-gnueabi-g++")

include_directories("inc")
add_library(visiontekGlue STATIC src/scroll_menu.c)