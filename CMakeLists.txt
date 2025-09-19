cmake_minimum_required(VERSION 3.20)
project(PresburgerTemporalGame VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find required packages
find_package(Boost REQUIRED COMPONENTS graph)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/../ggg/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Create build directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR})

# Set the output directory for the executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Add all source files for original temporis
set(TEMPORIS_SOURCES
    src/main.cpp
    src/presburger_term.cpp
    src/presburger_formula.cpp
    src/temporal_game_manager.cpp
    src/dot_parser.cpp
    src/temporal_analyzer.cpp
    src/temporal_game_demo.cpp
    src/temporal_game_application.cpp
    src/reachability_objective.cpp
    src/temporal_reachability_solver.cpp
    src/time_bound_calculator.cpp
)

# Add source files for GGG-integrated version
set(TEMPORIS_GGG_SOURCES
    src/main_ggg.cpp
    src/presburger_term.cpp
    src/presburger_formula.cpp
    src/ggg_temporal_graph.cpp
    src/ggg_temporal_solver.cpp
    src/time_bound_calculator.cpp
    src/dot_parser.cpp  # Still needed for parsing compatibility
    src/temporal_game_manager.cpp  # Needed for old game manager
    src/reachability_objective.cpp  # Needed for objective classes
)

# Add original executable
add_executable(temporis ${TEMPORIS_SOURCES})

# Add GGG-integrated executable
add_executable(temporis-ggg ${TEMPORIS_GGG_SOURCES})

# Link libraries for both executables
target_link_libraries(temporis ${Boost_LIBRARIES})
target_link_libraries(temporis-ggg ${Boost_LIBRARIES})

# Set target properties for both
set_target_properties(temporis PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_target_properties(temporis-ggg PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Print some useful information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Binary output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Source directory: ${CMAKE_SOURCE_DIR}/src")
message(STATUS "Input files directory: ${CMAKE_SOURCE_DIR}/input-files")
