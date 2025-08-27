#!/bin/bash

echo "=== DEEP ANALYSIS OF CRITICAL POSITIONS ==="
echo

# Game 1, Position before 13...Bxc3 (after 13.O-O)
echo "GAME 1: Position after 13.O-O (before 13...Bxc3?!)"
echo "FEN: r1b1k2r/pp3ppp/4p3/3p4/1b1qP3/2N2B2/PPP2PPP/1R3RK1 b kq - 1 13"
echo
echo "SeaJay analysis:"
echo -e "position fen r1b1k2r/pp3ppp/4p3/3p4/1b1qP3/2N2B2/PPP2PPP/1R3RK1 b kq - 1 13\neval\ngo depth 12\nquit" | /workspace/bin/seajay | grep -E "(eval|bestmove|score cp)" | head -15
echo
echo "Stockfish analysis:"
echo -e "position fen r1b1k2r/pp3ppp/4p3/3p4/1b1qP3/2N2B2/PPP2PPP/1R3RK1 b kq - 1 13\neval\ngo depth 20\nquit" | /workspace/external/engines/stockfish/stockfish 2>&1 | grep -E "(Final|bestmove)"
echo
echo "---"

# Game 1, Position before 17...Kf7 (after 17.Rbc1)
echo "GAME 1: Position after 17.Rbc1 (before 17...Kf7?!)"
echo "FEN: r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1PP2PPP/2R1R1K1 b kq - 5 17"
echo
echo "SeaJay analysis:"
echo -e "position fen r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1PP2PPP/2R1R1K1 b kq - 5 17\neval\ngo depth 12\nquit" | /workspace/bin/seajay | grep -E "(eval|bestmove|score cp)" | head -15
echo
echo "---"

# Game 2, Position before 10.Qg3 (after 9...Nd4)
echo "GAME 2: Position after 9...Nd4 (before 10.Qg3?!)"
echo "FEN: r2qk2r/ppp1bppp/3p1n2/4p3/2BnP3/2NP1Q1P/PPP2PP1/R1B2RK1 w kq - 1 10"
echo
echo "SeaJay analysis:"
echo -e "position fen r2qk2r/ppp1bppp/3p1n2/4p3/2BnP3/2NP1Q1P/PPP2PP1/R1B2RK1 w kq - 1 10\neval\ngo depth 12\nquit" | /workspace/bin/seajay | grep -E "(eval|bestmove|score cp)" | head -15
echo
echo "---"

