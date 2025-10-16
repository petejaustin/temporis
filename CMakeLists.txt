cmake_minimum_required(VERSION 3.20)
project(GGGTemporis VERSION 1.0.0 LANGUAGES CXX)

# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Boost policy for modern CMake (check if policy exists)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
find_package(Boost REQUIRED COMPONENTS graph)

# GGG-integrated temporis executable
add_executable(temporis
    src/main_ggg.cpp
    src/presburger_term.cpp
    src/presburger_formula.cpp
    src/ggg_temporal_graph.cpp
    src/ggg_temporal_solver.cpp
)

# Static expansion temporis executable (for research)
add_executable(temporis_static_expansion
    src/main_static_expansion.cpp
    src/presburger_term.cpp
    src/presburger_formula.cpp
    src/ggg_temporal_graph.cpp
    src/static_expansion_solver.cpp
)

# Common configuration for both executables
foreach(target temporis temporis_static_expansion)
    target_include_directories(${target} PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/../ggg/include
    )
    
    target_link_libraries(${target} PRIVATE ${Boost_LIBRARIES})
    
    target_compile_features(${target} PRIVATE cxx_std_20)
endforeach()

# Build information
message(STATUS "GGG Temporis - Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Standard temporis: ${CMAKE_BINARY_DIR}/temporis")
message(STATUS "Static expansion temporis: ${CMAKE_BINARY_DIR}/temporis_static_expansion")
