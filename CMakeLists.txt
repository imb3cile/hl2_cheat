cmake_minimum_required(VERSION 3.5)

project(hl2_cheat)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Hl2 was builded in the x32 arch. Use -A Win32 param when configuring")
endif()
add_subdirectory(hake)