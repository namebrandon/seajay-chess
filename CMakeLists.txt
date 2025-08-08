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

# Source files (to be populated as we develop)
set(SOURCES
    # src/main.cpp will be added in Phase 1
)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Main executable (commented until we have source files)
# add_executable(seajay ${SOURCES})

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

# Install target
# install(TARGETS seajay DESTINATION bin)

# Testing setup (for later phases)
enable_testing()
# add_subdirectory(tests)

# Benchmark target (for later phases)
# add_custom_target(bench
#     COMMAND seajay bench
#     DEPENDS seajay
#     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
# )