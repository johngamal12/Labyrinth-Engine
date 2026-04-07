# 1. Require a modern version of CMake
cmake_minimum_required(VERSION 3.15)

project(LabyrinthEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ==========================================
# 1. Fetch SFML
# ==========================================
message(STATUS "Fetching SFML 2.6.1...")
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.1
)
FetchContent_MakeAvailable(SFML)

# Block ImGui-SFML from finding global MSYS2 versions
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "Use fetched SFML" FORCE)

# ==========================================
# 2. Fetch Core ImGui (The missing piece!)
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
add_executable(EatMeat src/main.cpp)

target_link_libraries(EatMeat PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
    ImGui-SFML::ImGui-SFML
)

target_include_directories(EatMeat PRIVATE src)