# OpenBench-compatible Makefile for SeaJay Chess Engine
# =============================================================================
# This Makefile is used for both OpenBench server builds and local testing
# It auto-detects CPU capabilities using -march=native
# =============================================================================
#
# CPU Support:
# - Automatically detects and uses best available instruction sets
# - Works on any x86-64 CPU (Ryzen, Xeon, Core i7, etc.)
# - Will use SSE4.2, POPCNT, BMI2, AVX2 if available
# - Falls back gracefully on older CPUs
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
# COMPILER FLAGS - AUTO-DETECTED FOR ANY MACHINE
# =============================================================================
# Auto-detect the best instruction set for THIS machine
# -O3: Maximum optimization (faster than -O2)
# -march=native: Auto-detect CPU capabilities (SSE4.2, AVX2, BMI2, etc.)
# -flto: Link-time optimization for additional performance
BASE_FLAGS = -O3 -DNDEBUG -march=native -flto

# Use auto-detected flags for all builds
CXXFLAGS = $(BASE_FLAGS)
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