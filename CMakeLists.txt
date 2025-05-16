cmake_minimum_required(VERSION 3.10)

# ------------------ Project setup ---------------------------------
project(AntNetBackend VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

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
include_directories(
    ${CMAKE_SOURCE_DIR}/include/algo
    ${CMAKE_SOURCE_DIR}/include/algo/cpu
    ${CMAKE_SOURCE_DIR}/include/algo/gpu
    ${CMAKE_SOURCE_DIR}/include/consts
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/managers
    ${CMAKE_SOURCE_DIR}/include/rendering
    ${CMAKE_SOURCE_DIR}/include/types
)

# ------------------ Source files ----------------------------------
set(SOURCE_FILES
    src/c/algo/cpu/cpu_ACOv1.c
    src/c/algo/cpu/cpu_brute_force.c
    src/c/algo/cpu/cpu_random_algo.c
    src/c/core/backend.c
    src/c/core/backend_topology.c
    src/c/heatmap_renderer.c
    src/c/heatmap_renderer_async.c
    src/c/hop_map_manager.c
    src/c/managers/config_manager.c
    third_party/ini.c
)

# ------------------ OpenGL ES / EGL -------------------------------
find_library(EGL_LIB EGL REQUIRED)
find_library(GLESv2_LIB GLESv2 REQUIRED)

# ------------------ Build shared library --------------------------
add_library(antnet_backend SHARED ${SOURCE_FILES})

# ------------------ Linking ---------------------------------------
if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(antnet_backend
        PRIVATE
            m
            Threads::Threads
            ${EGL_LIB}
            ${GLESv2_LIB}
    )
endif()

# ------------------ Install targets -------------------------------
install(TARGETS antnet_backend
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)

install(DIRECTORY include/ DESTINATION include)

# ------------------ Final Summary -----------------------------------
message(STATUS "✅ AntNet backend build setup complete.")
message(STATUS "📦 Output: shared library 'antnet_backend' for CFFI loading.")
