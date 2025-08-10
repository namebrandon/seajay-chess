# Makefile for Bug #003 testing

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -g -I../src
LDFLAGS = ../build/libseajay_core.a

.PHONY: all clean test debug

all: test_promotion_bug debug_promotion

test_promotion_bug: test_promotion_bug.cpp
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) -o $@

debug_promotion: debug_promotion.cpp
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) -o $@

test: test_promotion_bug
	./test_promotion_bug

debug: debug_promotion
	./debug_promotion

clean:
	rm -f test_promotion_bug debug_promotion

# Quick test of the bug position with Stockfish
stockfish-test:
	@echo "Testing bug position with Stockfish:"
	@echo -e "position fen r3k3/P7/8/8/8/8/8/4K3 w - - 0 1\ngo perft 1\nquit" | ../external/engines/stockfish/stockfish 2>/dev/null | grep "Nodes searched"
	@echo ""
	@echo "Testing valid promotion position with Stockfish:"
	@echo -e "position fen 4k3/P7/8/8/8/8/8/4K3 w - - 0 1\ngo perft 1\nquit" | ../external/engines/stockfish/stockfish 2>/dev/null | grep "Nodes searched"