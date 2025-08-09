cmake_minimum_required(VERSION 3.16)
project(SeaJay VERSION 0.1.0 LANGUAGES CXX)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -march=native")

# Warning flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
    add_compile_options(-Wno-unused-parameter -Wno-unused-function)
endif()

# Core library sources
set(CORE_SOURCES
    src/core/board.cpp
    src/core/board_safety.cpp
    src/core/move_generation.cpp
    src/core/move_list.cpp
    src/core/bitboard.h
    src/core/board.h
    src/core/board_safety.h
    src/core/move_generation.h
    src/core/move_list.h
    src/core/types.h
)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Core library
add_library(seajay_core ${CORE_SOURCES})

# Main executable (to be added later)
# add_executable(seajay ${SOURCES})

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

# Install target
# install(TARGETS seajay DESTINATION bin)

# Testing setup
enable_testing()

# Unit tests
add_executable(test_board tests/unit/test_board.cpp)
target_link_libraries(test_board seajay_core)

add_executable(test_fen_comprehensive tests/unit/test_fen_comprehensive.cpp)
target_link_libraries(test_fen_comprehensive seajay_core)

add_executable(test_fen_edge_cases tests/unit/test_fen_edge_cases.cpp)
target_link_libraries(test_fen_edge_cases seajay_core)

# Demo programs
add_executable(fen_demo tests/demo/fen_demo.cpp)
target_link_libraries(fen_demo seajay_core)

add_executable(sliding_attacks_demo tests/demo/sliding_attacks_demo.cpp)
target_link_libraries(sliding_attacks_demo seajay_core)

add_executable(move_generation_demo tests/demo/move_generation_demo.cpp)
target_link_libraries(move_generation_demo seajay_core)

# Safety test
add_executable(test_board_safety tests/test_board_safety.cpp)
target_link_libraries(test_board_safety seajay_core)

# Add test cases
add_test(NAME board_tests COMMAND test_board)
add_test(NAME fen_comprehensive_tests COMMAND test_fen_comprehensive)
add_test(NAME fen_edge_cases_tests COMMAND test_fen_edge_cases)
add_test(NAME board_safety_tests COMMAND test_board_safety)

# Benchmark target (for later phases)
# add_custom_target(bench
#     COMMAND seajay bench
#     DEPENDS seajay
#     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
# )