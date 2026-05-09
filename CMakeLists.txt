cmake_minimum_required(VERSION 3.20)
project(SolarSystemProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(SolarSystem
    src/main.cpp
    src/glad.c
)

target_include_directories(SolarSystem PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(SolarSystem PRIVATE glfw glm::glm)