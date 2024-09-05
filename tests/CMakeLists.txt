# Add this line to find the system-wide Google Test On Ubuntu/Debian-based
# systems, through `sudo apt-get install libgtest-dev`

find_package(GTest REQUIRED)

add_executable(test_${PROJECT_NAME}
    test_main.cpp
)

target_include_directories(test_${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(test_${PROJECT_NAME} PRIVATE ${GTEST_INCLUDE_DIRS})

target_link_libraries(
    test_${PROJECT_NAME}
    PRIVATE
    dependency_analyzer
    ${GTEST_BOTH_LIBRARIES}
    pthread
)

add_test(NAME test_${PROJECT_NAME} COMMAND test_${PROJECT_NAME})
set_tests_properties(test_${PROJECT_NAME} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
