cmake_minimum_required(VERSION 3.24)
project(OpenCLWindow VERSION 1.0.0 DESCRIPTION "OpenCL Rendering Library")

include(FetchContent)

add_link_options(-static-libgcc -static-libstdc++ -static)

set(CMAKE_CXX_STANDARD 23)
add_subdirectory(libs/glad)

add_library(${PROJECT_NAME} STATIC
        src/cpp/OpenClWindow.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        PUBLIC_HEADER include/OpenClWindow.h
        )

find_package(OpenCL REQUIRED)
find_package(OpenGL REQUIRED)
FetchContent_Declare(
        glfw3
        GIT_REPOSITORY https://github.com/glfw/glfw.git
)
FetchContent_MakeAvailable(glfw3)



target_link_libraries(${PROJECT_NAME}
        PUBLIC OpenCL::OpenCL
        PRIVATE OpenGL::GL
        PUBLIC glfw
        PUBLIC glad
        )

target_include_directories(${PROJECT_NAME} PUBLIC include)