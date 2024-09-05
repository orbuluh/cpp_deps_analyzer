cmake_minimum_required(VERSION 3.10)
project(CppDependencyAnalyzer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

enable_testing()  # Move this line before adding subdirectories

add_subdirectory(src)
add_subdirectory(app)
add_subdirectory(tests)
