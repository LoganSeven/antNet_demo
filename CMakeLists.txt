cmake_minimum_required(VERSION 3.10)

# ------------------ Project setup ---------------------------------
project(AntNetBackend VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Default to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

message(STATUS "⛏️  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "💻 Target platform: ${CMAKE_SYSTEM_NAME}")

# ------------------ Compiler warnings/options -----------------------------
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# ------------------ Include directories ---------------------------
# Ensure this covers error_codes.h, random_algo.h, etc.
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party
)

# ------------------ Source files ----------------------------------
set(SOURCE_FILES
    src/c/hop_map_manager.c
    src/c/backend.c
    src/c/backend_topology.c
    src/c/random_algo.c
    src/c/cpu_brute_force.c
    src/c/cpu_ACOv1.c
    src/c/config_manager.c             
    third_party/ini.c                  
)

# ------------------ Build shared library --------------------------
add_library(antnet_backend SHARED ${SOURCE_FILES})

# ------------------ Linking ---------------------------------------
# Link math and pthread on Unix-like systems if needed
if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(antnet_backend PRIVATE m Threads::Threads)
endif()

# ------------------ Install targets (optional) --------------------
install(TARGETS antnet_backend
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)

install(DIRECTORY include/ DESTINATION include)

# ------------------ Final Summary -----------------------------------
message(STATUS "✅ AntNet backend build setup complete.")
message(STATUS "📦 Output: shared library 'antnet_backend' for CFFI loading.")
