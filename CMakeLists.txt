cmake_minimum_required(VERSION 3.16)
project(GpuProgrammingProject LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")

# GLFW build arguments - disable tests, examples and documentation
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

cmake_minimum_required(VERSION 3.16)

set(SRC_PATH engine/src)
file(GLOB ENGINE_GLOB
    ${SRC_PATH}/asset_helper.cpp
    ${SRC_PATH}/shader.cpp
    ${SRC_PATH}/application.cpp
    ${SRC_PATH}/texture.cpp
    ${SRC_PATH}/impl_stb.cpp
    ${SRC_PATH}/light.cpp
    )

set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "Build Noise Tool" FORCE) # don't build the graph tool
add_subdirectory("extern/fastnoise2")

add_subdirectory("extern/glfw")
add_subdirectory("extern/glm")
add_subdirectory("extern/assimp")
add_subdirectory("extern/entt")
add_subdirectory("extern/imgui")

add_subdirectory("engine")

add_executable(GpuProgrammingProject
    src/main.cpp
    src/renderpasses.cpp
    src/terrain.cpp
    src/terrain_geometry.cpp
)

target_include_directories(GpuProgrammingProject PUBLIC
    engine/include
)

target_link_libraries(GpuProgrammingProject PRIVATE engine FastNoise)

# Copy asset folder to executing directory
# TODO - Compress it
add_custom_command(
    TARGET GpuProgrammingProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:GpuProgrammingProject>/assets
)