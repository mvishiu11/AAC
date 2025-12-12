cmake_minimum_required(VERSION 3.20)

# Explicitly require MSVC compiler (Visual Studio)
set(CMAKE_C_COMPILER cl.exe)
set(CMAKE_CXX_COMPILER cl.exe)

project(AAC-FEAT-IMPLEMENTATION LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directory
include_directories(include)

# Source files
set(SOURCES
    src/Graph.cpp
    src/Hungarian.cpp
    src/main.cpp
)

# Executable
add_executable(main ${SOURCES})