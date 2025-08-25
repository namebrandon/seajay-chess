# OpenBench-compatible Makefile for SeaJay Chess Engine
# =============================================================================
# IMPORTANT: This Makefile is ONLY for OpenBench server builds!
# For local development, use: ./build.sh or cmake directly
# =============================================================================
#
# OpenBench Server Specs (as of 2024):
# - CPU: AMD Ryzen 9 5950X (Zen 3 architecture)
# - Supports: SSE4.2, POPCNT, BMI2, AVX, AVX2
# - Does NOT support: AVX512 (only in Ryzen 7000+/Zen 4)
#
# OpenBench expects:
# - make with CXX=compiler EXE=output_name
# - Binary placed at $(EXE) in the root directory

.PHONY: all clean

# OpenBench variables
CXX ?= g++
EXE ?= seajay
EVALFILE ?=

# Build configuration
BUILD_DIR = openbench-build
CMAKE_BUILD_TYPE = Release

# =============================================================================
# COMPILER FLAGS - OPTIMIZED FOR OPENBENCH (Ryzen 9 5950X)
# =============================================================================
# Base flags - universally supported
BASE_FLAGS = -O2 -DNDEBUG -mpopcnt -msse4.2

# Advanced flags for OpenBench's Ryzen 9 5950X
# AVX2 - 256-bit vector operations (critical for competing with other engines)
# BMI2 - PEXT/PDEP instructions for magic bitboards
ADVANCED_FLAGS = -mavx2 -mbmi2

# Combined flags for OpenBench builds
# NOTE: These will NOT work on older CPUs in local development!
CXXFLAGS = $(BASE_FLAGS) $(ADVANCED_FLAGS)
CMAKE_CXX_FLAGS = $(CXXFLAGS)

all:
	@echo "Building SeaJay for OpenBench..."
	@echo "Compiler: $(CXX)"
	@echo "Output: $(EXE)"
	@mkdir -p $(BUILD_DIR)
	
	# Configure with CMake (only build engine, not tests)
	cmake -S . -B $(BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DCMAKE_CXX_COMPILER=$(CXX) \
		-DCMAKE_CXX_FLAGS="$(CMAKE_CXX_FLAGS)" \
		-DEVALFILE="$(EVALFILE)" \
		-DBUILD_TESTING=OFF
	
	# Build ONLY the engine binary (not tests or utilities)
	$(MAKE) -C $(BUILD_DIR) -j seajay
	
	# Copy to expected location for OpenBench
	@if [ -f "$(BUILD_DIR)/seajay" ]; then \
		cp -f "$(BUILD_DIR)/seajay" "$(EXE)"; \
		strip "$(EXE)"; \
		echo "Successfully built $(EXE)"; \
	else \
		echo "Error: Binary not found at $(BUILD_DIR)/seajay" >&2; \
		exit 1; \
	fi

clean:
	@echo "Cleaning OpenBench build..."
	@rm -rf "$(BUILD_DIR)"
	@rm -f "$(EXE)" seajay test-binary
	@echo "Clean complete"