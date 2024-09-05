add_library(dependency_analyzer
    file_parser.cpp
    dependency_analyzer.cpp
)

target_include_directories(dependency_analyzer PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(dependency_analyzer PUBLIC stdc++fs)
