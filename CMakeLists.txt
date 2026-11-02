cmake_minimum_required(VERSION 3.16)
project(House3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---------------------------------------------------------------------------
# Външни библиотеки, които се изтеглят автоматично (GLFW, GLM)
# ---------------------------------------------------------------------------
include(FetchContent)

# CMake 4.x (идва с Visual Studio 2026) отказва проекти, обявени за CMake < 3.5.
# Тази настройка позволява на по-старите външни библиотеки да се конфигурират безпроблемно.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
# На Linux използваме X11 (работи и под Wayland чрез XWayland) – по-малко зависимости.
set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(glfw glm)

# ---------------------------------------------------------------------------
# Библиотеки, включени в проекта (external/): GLAD, Dear ImGui, stb
# ---------------------------------------------------------------------------
add_library(glad STATIC external/glad/src/gl.c)
target_include_directories(glad PUBLIC external/glad/include)

add_library(imgui STATIC
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC external/imgui external/imgui/backends)
target_link_libraries(imgui PUBLIC glfw)

# ---------------------------------------------------------------------------
# Приложението
# ---------------------------------------------------------------------------
file(GLOB_RECURSE HOUSE_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)
add_executable(House3D ${HOUSE_SOURCES})

target_include_directories(House3D PRIVATE src)
target_include_directories(House3D SYSTEM PRIVATE external/stb)
target_link_libraries(House3D PRIVATE glad glfw glm::glm imgui)

# Пътят до папките shaders/ и assets/ се вгражда при компилация,
# за да работи програмата и когато се стартира от IDE.
target_compile_definitions(House3D PRIVATE
    RESOURCE_DIR="${CMAKE_SOURCE_DIR}"
    GLM_ENABLE_EXPERIMENTAL)

if(MSVC)
    target_compile_options(House3D PRIVATE /utf-8 /W3)
    target_compile_options(imgui PRIVATE /utf-8)
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT House3D)
    set_property(TARGET House3D PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
else()
    target_compile_options(House3D PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
