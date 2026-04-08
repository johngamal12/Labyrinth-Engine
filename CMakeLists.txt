# 1. Require a modern version of CMake
cmake_minimum_required(VERSION 3.15)

project(LabyrinthEngine VERSION 1.0.0 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable the broken audio module to fix the UCRT64 link error
set(SFML_BUILD_AUDIO OFF CACHE BOOL "Disable audio" FORCE)

# Block ImGui-SFML from finding the MSYS2 3.0.2 version
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "Use fetched SFML" FORCE)

include(FetchContent)

# ==========================================
# 1. Fetch SFML 2.6.1
# ==========================================
message(STATUS "Fetching SFML 2.6.1 (Audio Disabled)...")
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.1
)
FetchContent_MakeAvailable(SFML)

# ==========================================
# 2. Fetch Core Dear ImGui 
# ==========================================
message(STATUS "Fetching Core Dear ImGui...")
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.9
)
FetchContent_Populate(imgui)
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "Path to ImGui" FORCE)

# ==========================================
# 3. Fetch ImGui-SFML Bridge
# ==========================================
message(STATUS "Fetching ImGui-SFML...")
FetchContent_Declare(ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG v2.6.1
)
FetchContent_MakeAvailable(ImGui-SFML)

# ==========================================
# ENGINE BUILD INSTRUCTIONS
# ==========================================
add_executable(EatMeat 
    src/main.cpp
    src/MapManager.cpp
)

# NOTICE: sfml-audio has been removed to allow the engine to compile
target_link_libraries(EatMeat PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    ImGui-SFML::ImGui-SFML
)

target_include_directories(EatMeat PRIVATE src)