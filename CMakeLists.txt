cmake_minimum_required(VERSION 3.16)
project(ImguiExample LANGUAGES C CXX)
set (CMAKE_CXX_STANDARD 20)

# GLFW build arguments - disable tests, examples and documentation
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory("extern/glfw")
add_subdirectory("extern/imgui")
add_subdirectory("extern/glm")
add_subdirectory("extern/entt")

add_subdirectory("engine")

add_executable(ImguiExample
    src/main.cpp
    src/ui.cpp
)

target_link_libraries(ImguiExample PRIVATE imgui engine)

# Copy asset folder to executing directory
# TODO - Compress it
add_custom_command(
    TARGET ImguiExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:ImguiExample>/assets
)