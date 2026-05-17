# ==========================================
# PROJECT CONFIGURATION
# ==========================================
cmake_minimum_required(VERSION 3.15)
project(LabyrinthEngine VERSION 1.0.0 LANGUAGES CXX)

# ==========================================
# BUILD TYPE
# ==========================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ==========================================
# C++17 STANDARD
# ==========================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==========================================
# STATIC LINKING (NFR-8.4.2)
# ==========================================
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libs" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "Disable audio" FORCE)
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "Use fetched SFML" FORCE)

include(FetchContent)

# ==========================================
# 1. SFML 2.6.1
# ==========================================
message(STATUS "Fetching SFML 2.6.1...")
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        2.6.1
)
FetchContent_MakeAvailable(SFML)

# ==========================================
# 2. Dear ImGui
# ==========================================
message(STATUS "Fetching Dear ImGui...")
cmake_policy(SET CMP0169 OLD)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.89.9
)
FetchContent_Populate(imgui)
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "Path to ImGui" FORCE)

# ==========================================
# 3. ImGui-SFML Bridge
# ==========================================
message(STATUS "Fetching ImGui-SFML...")
FetchContent_Declare(ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG        v2.6.1
)
FetchContent_MakeAvailable(ImGui-SFML)

# ==========================================
# 4. yaml-cpp
# ==========================================
message(STATUS "Fetching yaml-cpp...")
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(YAML_CPP_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB  OFF CACHE BOOL "" FORCE)
FetchContent_Declare(yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG       master
)
FetchContent_MakeAvailable(yaml-cpp)

# ==========================================
# 5. SQLite3 (single-file, no server needed)
# ==========================================
message(STATUS "Fetching SQLite3...")
FetchContent_Declare(sqlite3
    GIT_REPOSITORY https://github.com/alex85k/sqlite3-cmake.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(sqlite3)

# ==========================================
# ENGINE EXECUTABLE
# Uncomment source files as you implement them
# ==========================================
add_executable(EatMeat
    src/main.cpp
    src/core/Engine.cpp
    src/core/AssetManager.cpp
    src/core/DatabaseManager.cpp
    # src/core/Registry.cpp
    # src/core/MapManager.cpp
    # src/systems/MovementSystem.cpp
    # src/systems/CombatSystem.cpp
    # src/systems/AISystem.cpp
    # src/systems/PhysicsSystem.cpp
    # src/systems/RenderSystem.cpp
)

# ==========================================
# COMPILER WARNINGS (NFR-4.4.2)
# ==========================================
if(MSVC)
    target_compile_options(EatMeat PRIVATE /W4)
else()
    target_compile_options(EatMeat PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# ==========================================
# LINK LIBRARIES
# ==========================================
target_link_libraries(EatMeat PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    ImGui-SFML::ImGui-SFML
    yaml-cpp
    sqlite3
)

# ==========================================
# INCLUDE DIRECTORIES
# ==========================================
target_include_directories(EatMeat PRIVATE
    src
    src/core
    src/systems
    src/components
)

# ==========================================
# POST-BUILD: CREATE ASSET DIRECTORIES
# Ensures data/ and rooms/ sit next to .exe
# ==========================================
add_custom_command(TARGET EatMeat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:EatMeat>/data"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:EatMeat>/rooms"
    COMMENT "Creating asset directories next to executable"
)

# ==========================================
# BUILD SUMMARY
# ==========================================
message(STATUS "---------------------------------------")
message(STATUS "Project   : ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Std   : ${CMAKE_CXX_STANDARD}")
message(STATUS "---------------------------------------")