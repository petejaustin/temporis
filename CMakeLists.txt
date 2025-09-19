cmake_minimum_required(VERSION 3.20)
project(GGGTemporis VERSION 1.0.0 LANGUAGES CXX)

# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Boost policy for modern CMake
cmake_policy(SET CMP0167 NEW)
find_package(Boost REQUIRED COMPONENTS graph)

# GGG-integrated temporis executable
add_executable(temporis
    src/main_ggg.cpp
    src/presburger_term.cpp
    src/presburger_formula.cpp
    src/ggg_temporal_graph.cpp
    src/ggg_temporal_solver.cpp
)

# Modern CMake target configuration
target_include_directories(temporis PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../ggg/include
)

target_link_libraries(temporis PRIVATE ${Boost_LIBRARIES})

target_compile_features(temporis PRIVATE cxx_std_20)

# Build information
message(STATUS "GGG Temporis - Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output: ${CMAKE_BINARY_DIR}/temporis")
