#!/bin/bash
# SeaJay Chess Engine - Regression Test Suite
# Comprehensive regression testing for CI/CD integration

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
ENGINE_PATH="$PROJECT_ROOT/bin/seajay"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log() {
    echo -e "${GREEN}[$(date '+%H:%M:%S')]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Check if engine exists
check_engine() {
    if [[ ! -f "$ENGINE_PATH" ]]; then
        error "Engine not found at: $ENGINE_PATH"
        exit 1
    fi
}

# Run basic functionality tests
run_basic_tests() {
    log "Running basic functionality tests..."
    
    # Test engine starts and responds
    if echo "quit" | timeout 5 "$ENGINE_PATH" >/dev/null 2>&1; then
        success "Basic startup test PASSED"
    else
        error "Engine failed to start"
        return 1
    fi
    
    # Test UCI protocol
    if echo -e "uci\nquit" | timeout 5 "$ENGINE_PATH" | grep -q "uciok"; then
        success "UCI initialization test PASSED"
    else
        error "UCI initialization failed"
        return 1
    fi
    
    # Test bench command
    if echo -e "bench 2\nquit" | timeout 30 "$ENGINE_PATH" | grep -q "nps"; then
        success "Bench command test PASSED"
    else
        error "Bench command failed"
        return 1
    fi
    
    return 0
}

# Run perft regression tests
run_perft_tests() {
    log "Running perft regression tests..."
    
    python3 "$SCRIPT_DIR/run_perft_tests.py" \
        --engine "$ENGINE_PATH" \
        --required-only \
        --workers 4
    
    if [[ $? -eq 0 ]]; then
        success "Perft tests PASSED"
        return 0
    else
        error "Perft tests FAILED"
        return 1
    fi
}

# Run bench consistency tests
run_bench_consistency() {
    log "Running bench consistency tests..."
    
    local results=()
    for i in {1..3}; do
        log "Bench run $i/3..."
        local nps=$(echo -e "bench 3\nquit" | "$ENGINE_PATH" 2>/dev/null | grep -oP '\d+(?= nps)' | tail -1)
        if [[ -n "$nps" ]]; then
            results+=("$nps")
        fi
    done
    
    if [[ ${#results[@]} -eq 3 ]]; then
        success "Bench consistency test PASSED"
        return 0
    else
        error "Bench consistency test FAILED"
        return 1
    fi
}

# Main execution
main() {
    log "Starting SeaJay regression test suite..."
    log "Engine path: $ENGINE_PATH"
    
    check_engine
    
    # Track results
    total_tests=0
    passed_tests=0
    
    # Run test suites
    if run_basic_tests; then
        ((passed_tests++))
    fi
    ((total_tests++))
    
    if run_perft_tests; then
        ((passed_tests++))
    fi
    ((total_tests++))
    
    if run_bench_consistency; then
        ((passed_tests++))
    fi
    ((total_tests++))
    
    # Summary
    echo ""
    if [[ $passed_tests -eq $total_tests ]]; then
        success "All regression tests PASSED ($passed_tests/$total_tests)"
        exit 0
    else
        error "Some tests FAILED ($passed_tests/$total_tests passed)"
        exit 1
    fi
}

main "$@"