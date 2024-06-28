# Request the minimum version of CMake, in case of lower version throws error.
# See #https://cmake.org/cmake/help/latest/command/cmake_minimum_required.html

cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(
    "emergencyNotification"
    VERSION 1.0.0
    DESCRIPTION "Library for setting up different types of sockets (TCP, UDP, etc.)"
    LANGUAGES C
)

# Define the C standard, we are going to use std17
# See https://cmake.org/cmake/help/latest/variable/CMAKE_CXX_STANDARD.html
set(CMAKE_C_STANDARD 17)

# Include the 'include' directory, where the header files are located.
# See https://cmake.org/cmake/help/latest/command/include_directories.html
include_directories(include)

# Add the 'src' directory, where the source files are located.
# See https://cmake.org/cmake/help/latest/command/file.html#glob
file(GLOB_RECURSE SOURCES "src/*.c")

# Add the compilation flags
# See https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_FLAGS.html#variable:CMAKE_%3CLANG%3E_FLAGS
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wextra -Werror -Wconversion -std=gnu11")

# Add the library to be linked
#See https://cmake.org/cmake/help/latest/command/add_library.html
add_library(${PROJECT_NAME} SHARED ${SOURCES})
