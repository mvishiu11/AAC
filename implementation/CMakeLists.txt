cmake_minimum_required(VERSION 3.20)

# Explicitly require MSVC compiler (Visual Studio)
# set(CMAKE_C_COMPILER cl.exe)
# set(CMAKE_CXX_COMPILER cl.exe)

project(AAC LANGUAGES CXX)

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
    src/TUI.cpp
    src/GedTUI.cpp
)
set(FTXUI_MICROSOFT_TERMINAL_FALLBACK ON)
include(FetchContent)
FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
    GIT_TAG v6.1.9
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(ftxui)

# Executable
add_executable(main ${SOURCES})
target_link_libraries(main PRIVATE
    ftxui::component
    ftxui::dom
    ftxui::screen)