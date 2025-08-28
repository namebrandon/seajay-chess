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
# REMEDIATED: USE_MAGIC_BITBOARDS removed - now controlled via UCI option (default: ON)
option(DEBUG_MAGIC "Enable magic bitboard debug output" OFF)

# Transposition Table development options (Stage 12)
option(TT_DEBUG "Enable transposition table debug checks" OFF)
option(TT_PARANOID "Enable paranoid TT validation (slow)" OFF)
option(TT_SHADOW "Enable shadow hashing for TT validation" OFF)

# Stage 14 Remediation: Quiescence node limits now controlled via UCI at runtime
# No compile-time modes needed

if(DEBUG_MAGIC)
    add_compile_definitions(DEBUG_MAGIC)
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

# Enable Quiescence Search
add_compile_definitions(ENABLE_QUIESCENCE)

# Stage 14 Remediation: Quiescence modes removed - all controlled via UCI

# =============================================================================
# COMPILER FLAGS - LOCAL DEVELOPMENT ONLY
# =============================================================================
# NOTE: This CMakeLists.txt is for LOCAL DEVELOPMENT builds via build.sh
# It uses conservative flags for maximum compatibility with older CPUs.
#
# OpenBench builds use the Makefile which adds:
# - AVX2 for vector operations
# - BMI2 for PEXT/PDEP (magic bitboards)
# - POPCNT and SSE4.2 for bitboard operations
#
# Local builds only get basic optimizations to ensure compatibility.
# =============================================================================

# Warning flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # For Debug builds, use strict warnings
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
        add_compile_options(-Wno-unused-parameter -Wno-unused-function)
    else()
        # For Release builds, use less strict warnings
        add_compile_options(-Wall -Wno-conversion -Wno-sign-conversion -Wno-maybe-uninitialized)
    endif()
    # Ensure 64-bit arithmetic for magic bitboards
    add_compile_options(-m64)
    
    # Conservative instruction set support for local development
    # Only add POPCNT and SSE4.2 if explicitly requested (not by default)
    # This ensures maximum compatibility with older development machines
endif()

# Core library sources
set(CORE_SOURCES
    src/core/board.cpp
    src/core/board_safety.cpp
    src/core/move_generation.cpp
    src/core/move_list.cpp
    src/core/perft.cpp
    src/core/transposition_table.cpp
    src/core/see.cpp
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
    src/core/see.h
)

# Evaluation sources
set(EVAL_SOURCES
    src/evaluation/evaluate.cpp
    src/evaluation/evaluate.h
    src/evaluation/material.h
    src/evaluation/types.h
    src/evaluation/pawn_structure.cpp
    src/evaluation/pawn_structure.h
    src/evaluation/king_safety.cpp
    src/evaluation/king_safety.h
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
    src/search/killer_moves.cpp
    src/search/killer_moves.h
    src/search/history_heuristic.cpp
    src/search/history_heuristic.h
    src/search/countermoves.cpp
    src/search/countermoves.h
    src/search/time_management.cpp
    src/search/time_management.h
    src/search/aspiration_window.cpp
    src/search/aspiration_window.h
    src/search/quiescence.cpp
    src/search/quiescence.h
    src/search/quiescence_performance.cpp
    src/search/quiescence_performance.h
    src/search/lmr.cpp
    src/search/lmr.h
    # src/search/quiescence_optimized.cpp  # Disabled: needs refactoring for thread-local SEE stats
    # src/search/quiescence_optimized.h
)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Core library
add_library(seajay_core ${CORE_SOURCES} ${EVAL_SOURCES} ${SEARCH_SOURCES})

# UCI and benchmark sources
set(UCI_SOURCES
    src/uci/uci.cpp
    src/uci/info_builder.cpp
    src/benchmark/benchmark.cpp
)

# Main executable
add_executable(seajay 
    src/main.cpp
    ${UCI_SOURCES}
)
target_link_libraries(seajay seajay_core)

# Copy seajay binary to bin directory after build
add_custom_command(TARGET seajay POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:seajay> ${CMAKE_BINARY_DIR}/../bin/seajay
    COMMENT "Copying seajay to bin directory"
)

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

# Unit tests - DISABLED (not needed for production)
# add_executable(test_board tests/unit/test_board.cpp)
# target_link_libraries(test_board seajay_core)

# add_executable(test_fen_comprehensive tests/unit/test_fen_comprehensive.cpp)
# target_link_libraries(test_fen_comprehensive seajay_core)

# add_executable(test_fen_edge_cases tests/unit/test_fen_edge_cases.cpp)
# target_link_libraries(test_fen_edge_cases seajay_core)

# Demo programs - DISABLED (not needed for production)
# add_executable(fen_demo tests/demo/fen_demo.cpp)
# target_link_libraries(fen_demo seajay_core)

# add_executable(sliding_attacks_demo tests/demo/sliding_attacks_demo.cpp)
# target_link_libraries(sliding_attacks_demo seajay_core)

# Stage 15: SEE Demo
# add_executable(see_demo tests/demo/see_demo.cpp)
# target_link_libraries(see_demo seajay_core)

# add_executable(move_generation_demo tests/demo/move_generation_demo.cpp)
# target_link_libraries(move_generation_demo seajay_core)

# Test executables - DISABLED (not needed for production)
# Safety test
# add_executable(test_board_safety tests/test_board_safety.cpp)
# target_link_libraries(test_board_safety seajay_core)

# En passant check evasion test (Bug #001 regression test)
# add_executable(test_en_passant_check_evasion tests/test_en_passant_check_evasion.cpp)
# target_link_libraries(test_en_passant_check_evasion seajay_core)

# Magic bitboard validator test
# add_executable(test_magic_validator tests/test_magic_validator.cpp)
# target_link_libraries(test_magic_validator seajay_core)

# Magic bitboards comprehensive test suite
# add_executable(test_magic_bitboards tests/magic_bitboards_test.cpp)
# target_link_libraries(test_magic_bitboards seajay_core)

# A/B testing framework test
# add_executable(test_ab_framework tests/test_ab_framework.cpp)
# target_link_libraries(test_ab_framework seajay_core)

# Magic mask validation test (Stage 10, Step 1A)
# add_executable(test_magic_masks tests/test_magic_masks.cpp)
# target_link_libraries(test_magic_masks seajay_core)

# Material evaluation test
# add_executable(test_material_evaluation tests/test_material_evaluation.cpp)
# target_link_libraries(test_material_evaluation seajay_core)

# Negamax search test
# add_executable(test_negamax tests/test_negamax.cpp)
# target_link_libraries(test_negamax seajay_core)

# add_executable(test_quiescence tests/search/test_quiescence.cpp)
# target_link_libraries(test_quiescence seajay_core)

# MVV-LVA move ordering test
# add_executable(test_mvv_lva tests/test_mvv_lva.cpp)
# target_link_libraries(test_mvv_lva seajay_core)

# MVV-LVA performance validation test
# add_executable(test_mvv_lva_performance tests/test_mvv_lva_performance.cpp)
# target_link_libraries(test_mvv_lva_performance seajay_core)

# MVV-LVA Phase 7 performance test
# add_executable(test_mvv_lva_phase7 tests/test_mvv_lva_phase7.cpp)
# target_link_libraries(test_mvv_lva_phase7 seajay_core)

# Stage 12: Transposition Table tests - DISABLED (not needed for production)
# add_executable(test_zobrist tests/unit/test_zobrist.cpp)
# target_link_libraries(test_zobrist seajay_core)

# add_executable(test_transposition_table tests/unit/test_transposition_table.cpp)
# target_link_libraries(test_transposition_table seajay_core)

# add_executable(test_tt_search tests/integration/test_tt_search.cpp)
# target_link_libraries(test_tt_search seajay_core)

# add_executable(test_tt_chaos tests/stress/test_tt_chaos.cpp)
# target_link_libraries(test_tt_chaos seajay_core)

# add_executable(test_perft_tt tests/unit/test_perft_tt.cpp)
# target_link_libraries(test_perft_tt seajay_core)

# Stage 15: SEE tests - DISABLED (not needed for production)
# if(NOT SKIP_ALPHABETA_TEST)
#     add_executable(test_see_basic tests/unit/test_see_basic.cpp)
#     target_include_directories(test_see_basic PRIVATE ${GTEST_INCLUDE_DIR})
#     target_link_libraries(test_see_basic seajay_core ${GTEST_LIBRARIES} pthread)
#     
#     add_executable(test_see_special tests/unit/test_see_special.cpp)
#     target_include_directories(test_see_special PRIVATE ${GTEST_INCLUDE_DIR})
#     target_link_libraries(test_see_special seajay_core ${GTEST_LIBRARIES} pthread)
# else()
#     add_executable(test_see_basic tests/unit/test_see_basic.cpp)
#     target_link_libraries(test_see_basic seajay_core)
#     
#     add_executable(test_see_special tests/unit/test_see_special.cpp)
#     target_link_libraries(test_see_special seajay_core)
# endif()

# Quiescence Performance Benchmark Tool (Phase 2.3 - Missing Item 3)
# NOTE: Source files don't exist - commented out
# add_executable(qsearch_benchmark tools/qsearch_benchmark.cpp)
# target_link_libraries(qsearch_benchmark seajay_core)

# Quiescence Memory Optimization Analysis Tool (Phase 2.3 - Missing Item 4)
# NOTE: Source files don't exist - commented out
# add_executable(qsearch_memory_analysis tools/qsearch_memory_analysis.cpp)
# target_link_libraries(qsearch_memory_analysis seajay_core)

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

# GoogleTest-based tests - DISABLED (not needed for production)
# if(NOT SKIP_ALPHABETA_TEST)
#     add_executable(test_alphabeta_validation tests/alphabeta_validation_test.cpp)
#     target_include_directories(test_alphabeta_validation PRIVATE ${GTEST_INCLUDE_DIR})
#     target_link_libraries(test_alphabeta_validation seajay_core ${GTEST_LIBRARIES} pthread)
#     
#     # Stage 13: Iterative Deepening regression tests
#     add_executable(test_iterative_regression tests/test_iterative_regression.cpp)
#     target_include_directories(test_iterative_regression PRIVATE ${GTEST_INCLUDE_DIR})
#     target_link_libraries(test_iterative_regression seajay_core ${GTEST_LIBRARIES} pthread)
#     
#     # Stage 14: Quiescence search limit tests
#     add_executable(test_quiescence_limits tests/search/test_quiescence_limits.cpp)
#     target_include_directories(test_quiescence_limits PRIVATE ${GTEST_INCLUDE_DIR})
#     target_link_libraries(test_quiescence_limits seajay_core ${GTEST_LIBRARIES} pthread)
#     
#     # Stage 14: Delta pruning tests
#     add_executable(test_delta_pruning_simple tests/search/test_delta_pruning_simple.cpp)
#     target_link_libraries(test_delta_pruning_simple seajay_core)
#     
#     # Stage 18: LMR tests
#     add_executable(test_lmr tests/test_lmr.cpp)
#     target_include_directories(test_lmr PRIVATE ${GTEST_INCLUDE_DIR})
#     target_link_libraries(test_lmr seajay_core ${GTEST_LIBRARIES} pthread)
# endif()

# Test cases - DISABLED (test executables removed)
# add_test(NAME board_tests COMMAND test_board)
# add_test(NAME fen_comprehensive_tests COMMAND test_fen_comprehensive)
# add_test(NAME fen_edge_cases_tests COMMAND test_fen_edge_cases)
# add_test(NAME board_safety_tests COMMAND test_board_safety)
# add_test(NAME en_passant_check_evasion_tests COMMAND test_en_passant_check_evasion)

# Tactical tests - DISABLED (not needed for production)
# add_executable(tactical_test tests/tactical_test.cpp)
# target_link_libraries(tactical_test seajay_core)

# Simple tactical test
# add_executable(tactical_simple tests/tactical_simple.cpp)
# target_link_libraries(tactical_simple seajay_core)

# Stage 15: Simple SEE test (no GoogleTest dependency)
# add_executable(test_see_simple tests/test_see_simple.cpp)
# target_link_libraries(test_see_simple seajay_core)

# Benchmark target
add_custom_target(bench
    COMMAND seajay bench
    DEPENDS seajay
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)