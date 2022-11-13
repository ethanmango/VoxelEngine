cmake_minimum_required (VERSION 2.6)
project(OpenGLVoxel)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_executable(
        ${PROJECT_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Shader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/voxelMainGL.cpp src/controls.cpp)

target_include_directories(
        ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include
)

target_link_libraries(
        ${PROJECT_NAME} glfw GL dl GLEW
)