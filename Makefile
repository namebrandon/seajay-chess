# OpenBench-compatible Makefile for SeaJay Chess Engine
# This wraps the CMake build system for OpenBench integration
# 
# OpenBench expects:
# - make with CXX=compiler EXE=output_name
# - Binary placed at $(EXE) in the root directory
# - Support for EVALFILE if using neural networks

.PHONY: all clean

# OpenBench variables
CXX ?= g++
EXE ?= seajay
EVALFILE ?=

# Build configuration
BUILD_DIR = openbench-build
CMAKE_BUILD_TYPE = Release

# Compiler flags for optimization
# Enable POPCNT and SSE4.2 for hardware acceleration (critical for chess engines)
BASE_FLAGS = -O2 -DNDEBUG -mpopcnt -msse4.2

# Advanced instruction sets for modern CPUs
# BMI2 provides PEXT/PDEP for magic bitboards
# AVX2 provides 256-bit vector operations
# AVX512 provides 512-bit vector operations (if OpenBench server supports it)
ADVANCED_FLAGS = -mavx2 -mbmi2 -mavx512f -mavx512bw

# Combine all flags - unsupported instructions will cause compilation to fail
# which OpenBench will handle by falling back to a different binary
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