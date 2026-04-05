cmake_minimum_required(VERSION 3.16)
project(SimpleAdamVisualization LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake where to find SFML 3
list(APPEND CMAKE_PREFIX_PATH "/usr/local")

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

add_executable(app src/main.cpp)

target_link_libraries(app PRIVATE SFML::Graphics SFML::Window SFML::System)