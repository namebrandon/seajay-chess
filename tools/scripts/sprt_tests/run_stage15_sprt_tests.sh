#!/bin/bash
# Stage 15 SEE SPRT Test Runner
# Runs both opening book and startpos tests

echo "=========================================="
echo "Stage 15 SEE SPRT Test Suite"
echo "=========================================="
echo ""
echo "This will run two SPRT tests:"
echo "1. With 4moves opening book (more variety)"
echo "2. From starting position (traditional)"
echo ""
echo "Expected improvement: +30-50 ELO"
echo ""

# Make scripts executable
chmod +x /workspace/tools/scripts/sprt_tests/stage15_see_test_with_book.sh
chmod +x /workspace/tools/scripts/sprt_tests/stage15_see_test_startpos.sh

# Ask which test to run
echo "Which test would you like to run?"
echo "1) Opening book test (recommended)"
echo "2) Starting position test"
echo "3) Both tests (parallel)"
echo "4) Both tests (sequential)"
echo ""
read -p "Enter choice [1-4]: " choice

case $choice in
    1)
        echo "Running opening book test..."
        /workspace/tools/scripts/sprt_tests/stage15_see_test_with_book.sh
        ;;
    2)
        echo "Running starting position test..."
        /workspace/tools/scripts/sprt_tests/stage15_see_test_startpos.sh
        ;;
    3)
        echo "Running both tests in parallel..."
        echo "Starting opening book test in background..."
        /workspace/tools/scripts/sprt_tests/stage15_see_test_with_book.sh &
        PID1=$!
        
        echo "Starting startpos test..."
        /workspace/tools/scripts/sprt_tests/stage15_see_test_startpos.sh &
        PID2=$!
        
        echo ""
        echo "Both tests running in parallel"
        echo "Book test PID: $PID1"
        echo "Startpos test PID: $PID2"
        echo ""
        echo "Waiting for tests to complete..."
        wait $PID1 $PID2
        echo "Both tests complete!"
        ;;
    4)
        echo "Running both tests sequentially..."
        echo ""
        echo "=== Test 1: Opening Book ==="
        /workspace/tools/scripts/sprt_tests/stage15_see_test_with_book.sh
        
        echo ""
        echo "=== Test 2: Starting Position ==="
        /workspace/tools/scripts/sprt_tests/stage15_see_test_startpos.sh
        
        echo ""
        echo "Both tests complete!"
        ;;
    *)
        echo "Invalid choice. Exiting."
        exit 1
        ;;
esac

echo ""
echo "To view results:"
echo "  Opening book test: cat /workspace/sprt_results/stage15_see_with_book/fastchess.log | tail -20"
echo "  Startpos test: cat /workspace/sprt_results/stage15_see_startpos/fastchess.log | tail -20"