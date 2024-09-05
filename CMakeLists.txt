cmake_minimum_required(VERSION 3.12)
project(cpp_deps_analyzer)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_subdirectory(src)
add_subdirectory(app)
add_subdirectory(tests)
