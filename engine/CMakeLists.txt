cmake_minimum_required(VERSION 3.16)

set(SRC_PATH ${CMAKE_CURRENT_LIST_DIR}/src)
file(GLOB ENGINE_GLOB
    ${SRC_PATH}/asset_helper.cpp
    ${SRC_PATH}/shader.cpp
    ${SRC_PATH}/application.cpp
    ${SRC_PATH}/texture.cpp
    ${SRC_PATH}/impl_stb.cpp
    )
add_library(engine STATIC ${ENGINE_GLOB} ../extern/glad/src/glad.c)

target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ../extern/glad/include
    ../extern/stb
    )
target_link_libraries(engine PUBLIC glfw glm assimp)