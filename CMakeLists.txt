# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("BackingRender")
set (CMAKE_CXX_STANDARD 11)

find_package(OpenGL REQUIRED)
set(DEPENDENCIES OpenGL::GL)

# add glfw
set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glfw")
SET(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW example" FORCE)
SET(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW tests" FORCE)
SET(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW docs" FORCE)
add_subdirectory(${GLFW_DIR})
set(DEPENDENCIES ${DEPENDENCIES} glfw)
include_directories("${GLFW_DIR}/include")

# add glad
# set(GLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glad")
# include_directories("${GLAD_DIR}/include")
set(GLAD_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad)
add_subdirectory("${GLAD_SOURCES_DIR}/cmake" glad_cmake)
glad_add_library(glad_gl_core_mx_43 API gl:core=4.3)
set(DEPENDENCIES ${DEPENDENCIES} glad_gl_core_mx_43)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/gladsources/glad_gl_core_mx_43/include")
message("${CMAKE_CURRENT_BINARY_DIR}/gladsources/glad_gl_core_mx_43/include")

# add glm
set(GLM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glm")
include_directories("${GLM_DIR}")

# add nlohmann json
set(NLOHMANN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/nlohmann/single_include")
include_directories(${NLOHMANN_PATH})

# add stb image
set(STB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stb")
include_directories(${STB_PATH})

# add source files
file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.*)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# build and link
add_executable(app main.cpp ${SOURCE_FILES})
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_compile_options(app PRIVATE "/WX")
else()
	target_compile_options(app PRIVATE "-Werror")
endif()
target_link_libraries(app PUBLIC ${DEPENDENCIES})