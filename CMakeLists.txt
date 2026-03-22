cmake_minimum_required(VERSION 3.21)

project(Voxonaut C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fix CMake policy warning for older minimum required versions
set(CMAKE_POLICY_VERSION_MINIMUM "3.5")

# Fetch raylib
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG master # Use master to avoid older CMake issues
    GIT_PROGRESS TRUE
)

# Disable building raylib examples
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib)

# Add our executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Link raylib
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# If on Windows, we might want to avoid the console window popping up 
# but for development, console is good for debugging.
