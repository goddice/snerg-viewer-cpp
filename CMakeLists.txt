# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("BackingRender")

find_package(OpenGL REQUIRED)
set(DEPENDENCIES OpenGL::GL)

set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glfw")
SET(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW example" FORCE)
SET(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW tests" FORCE)
SET(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW docs" FORCE)
add_subdirectory(${GLFW_DIR})
set(DEPENDENCIES ${DEPENDENCIES} glfw)
include_directories("${GLFW_DIR}/include")

set(GLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glad")
include_directories("${GLAD_DIR}/include")

set(GLM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glm")
include_directories("${GLM_DIR}")

set(NLOHMANN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/nlohmann/single_include")
include_directories(${NLOHMANN_PATH})

set(THIRD_PARTY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
include_directories(${THIRD_PARTY_PATH})

add_executable(app main.cpp ${GLAD_DIR}/src/glad.c shader.h  "constants.h" "Raymarch.cpp" "TrackballControls.h" "TrackballControls.cpp")
target_link_libraries(app ${DEPENDENCIES})