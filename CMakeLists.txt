cmake_minimum_required(VERSION 3.27.1)
project(auto-edger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(auto-edger main.cpp)

target_link_libraries(auto-edger gdi32 user32)
