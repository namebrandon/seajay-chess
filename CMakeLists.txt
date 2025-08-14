cmake_minimum_required(VERSION 3.16)
project(SeaJay VERSION 2.9.1 LANGUAGES CXX)

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
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")

# Magic bitboards development options (Stage 10)
option(USE_MAGIC_BITBOARDS "Use magic bitboards instead of ray-based" OFF)
option(DEBUG_MAGIC "Enable magic bitboard debug output" OFF)
option(SANITIZE_MAGIC "Enable all sanitizers for magic development" OFF)

# Transposition Table development options (Stage 12)
option(TT_DEBUG "Enable transposition table debug checks" OFF)
option(TT_PARANOID "Enable paranoid TT validation (slow)" OFF)
option(TT_SHADOW "Enable shadow hashing for TT validation" OFF)

if(DEBUG_MAGIC)
    add_compile_definitions(DEBUG_MAGIC)
endif()

if(USE_MAGIC_BITBOARDS)
    add_compile_definitions(USE_MAGIC_BITBOARDS)
endif()

if(SANITIZE_MAGIC)
    add_compile_options(-fsanitize=address,undefined,leak)
    add_link_options(-fsanitize=address,undefined,leak)
endif()

# Transposition Table compilation modes
if(TT_PARANOID)
    add_compile_definitions(TT_PARANOID)
    add_compile_definitions(TT_DEBUG)  # Paranoid implies debug
    message(STATUS "TT Paranoid mode enabled (includes debug checks)")
elseif(TT_DEBUG)
    add_compile_definitions(TT_DEBUG)
    message(STATUS "TT Debug mode enabled")
endif()

if(TT_SHADOW)
    add_compile_definitions(TT_SHADOW)
    message(STATUS "TT Shadow hashing enabled")
endif()

# Warning flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
    add_compile_options(-Wno-unused-parameter -Wno-unused-function)
    # Ensure 64-bit arithmetic for magic bitboards
    add_compile_options(-m64)
endif()

# Core library sources
set(CORE_SOURCES
    src/core/board.cpp
    src/core/board_safety.cpp
    src/core/move_generation.cpp
    src/core/move_list.cpp
    src/core/perft.cpp
    src/core/transposition_table.cpp
    src/core/bitboard.h
    src/core/board.h
    src/core/board_safety.h
    src/core/move_generation.h
    src/core/move_list.h
    src/core/perft.h
    src/core/types.h
    src/core/attack_wrapper.h
    src/core/magic_validator.h
    src/core/magic_bitboards.h
    src/core/magic_constants.h
    src/core/transposition_table.h
)

# Evaluation sources
set(EVAL_SOURCES
    src/evaluation/evaluate.cpp
    src/evaluation/evaluate.h
    src/evaluation/material.h
    src/evaluation/types.h
)

# Search sources  
set(SEARCH_SOURCES
    src/search/search.cpp
    src/search/search.h
    src/search/negamax.cpp
    src/search/negamax.h
    src/search/types.h
    src/search/move_ordering.cpp
    src/search/move_ordering.h
    src/search/time_management.cpp
    src/search/time_management.h
    src/search/aspiration_window.cpp
    src/search/aspiration_window.h
)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Core library
add_library(seajay_core ${CORE_SOURCES} ${EVAL_SOURCES} ${SEARCH_SOURCES})

# UCI and benchmark sources
set(UCI_SOURCES
    src/uci/uci.cpp
    src/benchmark/benchmark.cpp
)

# Main executable
add_executable(seajay 
    src/main.cpp
    ${UCI_SOURCES}
)
target_link_libraries(seajay seajay_core)

# Perft tool executable
add_executable(perft_tool
    src/tools/perft_tool.cpp
)
target_link_libraries(perft_tool seajay_core)

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

# Install target
install(TARGETS seajay DESTINATION bin)

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

# En passant check evasion test (Bug #001 regression test)
add_executable(test_en_passant_check_evasion tests/test_en_passant_check_evasion.cpp)
target_link_libraries(test_en_passant_check_evasion seajay_core)

# Magic bitboard validator test
add_executable(test_magic_validator tests/test_magic_validator.cpp)
target_link_libraries(test_magic_validator seajay_core)

# Magic bitboards comprehensive test suite
add_executable(test_magic_bitboards tests/magic_bitboards_test.cpp)
target_link_libraries(test_magic_bitboards seajay_core)

# A/B testing framework test
add_executable(test_ab_framework tests/test_ab_framework.cpp)
target_link_libraries(test_ab_framework seajay_core)

# Magic mask validation test (Stage 10, Step 1A)
add_executable(test_magic_masks tests/test_magic_masks.cpp)
target_link_libraries(test_magic_masks seajay_core)

# Material evaluation test
add_executable(test_material_evaluation tests/test_material_evaluation.cpp)
target_link_libraries(test_material_evaluation seajay_core)

# Negamax search test
add_executable(test_negamax tests/test_negamax.cpp)
target_link_libraries(test_negamax seajay_core)

# MVV-LVA move ordering test
add_executable(test_mvv_lva tests/test_mvv_lva.cpp)
target_link_libraries(test_mvv_lva seajay_core)

# MVV-LVA performance validation test
add_executable(test_mvv_lva_performance tests/test_mvv_lva_performance.cpp)
target_link_libraries(test_mvv_lva_performance seajay_core)

# MVV-LVA Phase 7 performance test
add_executable(test_mvv_lva_phase7 tests/test_mvv_lva_phase7.cpp)
target_link_libraries(test_mvv_lva_phase7 seajay_core)

# Stage 12: Transposition Table tests
add_executable(test_zobrist tests/unit/test_zobrist.cpp)
target_link_libraries(test_zobrist seajay_core)

add_executable(test_transposition_table tests/unit/test_transposition_table.cpp)
target_link_libraries(test_transposition_table seajay_core)

add_executable(test_tt_search tests/integration/test_tt_search.cpp)
target_link_libraries(test_tt_search seajay_core)

add_executable(test_tt_chaos tests/stress/test_tt_chaos.cpp)
target_link_libraries(test_tt_chaos seajay_core)

add_executable(test_perft_tt tests/unit/test_perft_tt.cpp)
target_link_libraries(test_perft_tt seajay_core)

# Alpha-beta validation test with GoogleTest
# Detect platform and use appropriate GoogleTest libraries
if(APPLE)
    # Check for macOS-specific GoogleTest first
    if(EXISTS "${CMAKE_SOURCE_DIR}/external/googletest-macos/lib/libgtest.a")
        set(GTEST_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/googletest-macos/include")
        set(GTEST_LIBRARIES 
            "${CMAKE_SOURCE_DIR}/external/googletest-macos/lib/libgtest.a"
            "${CMAKE_SOURCE_DIR}/external/googletest-macos/lib/libgtest_main.a")
        message(STATUS "Using macOS-specific GoogleTest libraries")
    else()
        message(WARNING "macOS GoogleTest not found. Run tools/scripts/setup_googletest_macos.sh to build it.")
        message(WARNING "Skipping test_alphabeta_validation build")
        set(SKIP_ALPHABETA_TEST ON)
    endif()
else()
    # Use Linux GoogleTest libraries
    set(GTEST_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/googletest/googletest/include")
    set(GTEST_LIBRARIES 
        "${CMAKE_SOURCE_DIR}/external/googletest/lib/libgtest.a"
        "${CMAKE_SOURCE_DIR}/external/googletest/lib/libgtest_main.a")
endif()

# Only build the test if GoogleTest is available
if(NOT SKIP_ALPHABETA_TEST)
    add_executable(test_alphabeta_validation tests/alphabeta_validation_test.cpp)
    target_include_directories(test_alphabeta_validation PRIVATE ${GTEST_INCLUDE_DIR})
    target_link_libraries(test_alphabeta_validation seajay_core ${GTEST_LIBRARIES} pthread)
    
    # Stage 13: Iterative Deepening regression tests
    add_executable(test_iterative_regression tests/test_iterative_regression.cpp)
    target_include_directories(test_iterative_regression PRIVATE ${GTEST_INCLUDE_DIR})
    target_link_libraries(test_iterative_regression seajay_core ${GTEST_LIBRARIES} pthread)
endif()

# Add test cases
add_test(NAME board_tests COMMAND test_board)
add_test(NAME fen_comprehensive_tests COMMAND test_fen_comprehensive)
add_test(NAME fen_edge_cases_tests COMMAND test_fen_edge_cases)
add_test(NAME board_safety_tests COMMAND test_board_safety)
add_test(NAME en_passant_check_evasion_tests COMMAND test_en_passant_check_evasion)

# Benchmark target
add_custom_target(bench
    COMMAND seajay bench
    DEPENDS seajay
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)