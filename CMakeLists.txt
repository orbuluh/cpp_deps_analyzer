cmake_minimum_required(VERSION 3.12)
project(cpp_deps_analyzer)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Add debug and release flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Print the build type
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

add_subdirectory(src)
add_subdirectory(app)


# Enable testing and build tests only for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  enable_testing()
  add_subdirectory(tests)
else()
  message(STATUS "Skipping tests in Release mode")
endif()
