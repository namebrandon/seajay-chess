#!/bin/bash

# Script to analyze critical positions from human games

SEAJAY="/workspace/bin/seajay"
STOCKFISH="/workspace/external/engines/stockfish/stockfish"

echo "==================== GAME 1 ANALYSIS ===================="
echo ""

# Game 1, Position 1a: Before 13...Bxb2
echo "Position 1a: After 12...Bxc3 13.Bd6 (before 13...Bxb2)"
echo "FEN: r1b1k2r/pp3ppp/4p3/3p4/4q3/2b5/PPQ2PPP/R4RK1 b kq - 0 13"
echo ""
echo "SeaJay evaluation:"
echo -e "position fen r1b1k2r/pp3ppp/4p3/3p4/4q3/2b5/PPQ2PPP/R4RK1 b kq - 0 13\ngo depth 10\nquit" | $SEAJAY | grep -E "(bestmove|score|pv)"
echo ""
echo "Stockfish evaluation:"
echo -e "position fen r1b1k2r/pp3ppp/4p3/3p4/4q3/2b5/PPQ2PPP/R4RK1 b kq - 0 13\nsetoption name Threads value 1\ngo depth 15\nquit" | $STOCKFISH | grep -E "(bestmove|score|pv)" | head -5
echo ""
echo "----------------------------------------"
echo ""

# Game 1, Position 1b: Before 16...Qg4
echo "Position 1b: After 15.Qxb2 f6 16.Rfe1 (before 16...Qg4)"
echo "FEN: r1b1k2r/pp3p1p/4pp2/3p4/4q3/8/PQ3PPP/2R1R1K1 b kq - 1 16"
echo ""
echo "SeaJay evaluation:"
echo -e "position fen r1b1k2r/pp3p1p/4pp2/3p4/4q3/8/PQ3PPP/2R1R1K1 b kq - 1 16\ngo depth 10\nquit" | $SEAJAY | grep -E "(bestmove|score|pv)"
echo ""
echo "Stockfish evaluation:"
echo -e "position fen r1b1k2r/pp3p1p/4pp2/3p4/4q3/8/PQ3PPP/2R1R1K1 b kq - 1 16\nsetoption name Threads value 1\ngo depth 15\nquit" | $STOCKFISH | grep -E "(bestmove|score|pv)" | head -5
echo ""
echo "----------------------------------------"
echo ""

# Game 1, Position 1c: Before 17...Kf7
echo "Position 1c: After 16...Qg4 (before 17...Kf7)"
echo "FEN: r1b1k2r/pp3p1p/4pp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 2 17"
echo ""
echo "SeaJay evaluation:"
echo -e "position fen r1b1k2r/pp3p1p/4pp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 2 17\ngo depth 10\nquit" | $SEAJAY | grep -E "(bestmove|score|pv)"
echo ""
echo "Stockfish evaluation:"
echo -e "position fen r1b1k2r/pp3p1p/4pp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 2 17\nsetoption name Threads value 1\ngo depth 15\nquit" | $STOCKFISH | grep -E "(bestmove|score|pv)" | head -5
echo ""
echo "----------------------------------------"
echo ""

# Game 1, Position 1d: Before 19...Qf5
echo "Position 1d: After 18.Rc7+ Kg6 19.h3 (before 19...Qf5)"
echo "FEN: r1b4r/ppR3p1/4ppk1/3p4/6q1/7P/PQ2QPP1/4R1K1 b - - 0 19"
echo ""
echo "SeaJay evaluation:"
echo -e "position fen r1b4r/ppR3p1/4ppk1/3p4/6q1/7P/PQ2QPP1/4R1K1 b - - 0 19\ngo depth 10\nquit" | $SEAJAY | grep -E "(bestmove|score|pv)"
echo ""
echo "Stockfish evaluation:"
echo -e "position fen r1b4r/ppR3p1/4ppk1/3p4/6q1/7P/PQ2QPP1/4R1K1 b - - 0 19\nsetoption name Threads value 1\ngo depth 15\nquit" | $STOCKFISH | grep -E "(bestmove|score|pv)" | head -5
echo ""
echo "----------------------------------------"
echo ""

# Game 1, Position 1e: Before 21...Kh5
echo "Position 1e: After 20...e5 21.Rg3+ (before 21...Kh5)"
echo "FEN: r1b4r/ppR3p1/5p2/3pPq2/8/6RP/PQ2QPP1/6K1 b - - 2 21"
echo ""
echo "SeaJay evaluation:"
echo -e "position fen r1b4r/ppR3p1/5p2/3pPq2/8/6RP/PQ2QPP1/6K1 b - - 2 21\ngo depth 10\nquit" | $SEAJAY | grep -E "(bestmove|score|pv)"
echo ""
echo "Stockfish evaluation:"
echo -e "position fen r1b4r/ppR3p1/5p2/3pPq2/8/6RP/PQ2QPP1/6K1 b - - 2 21\nsetoption name Threads value 1\ngo depth 15\nquit" | $STOCKFISH | grep -E "(bestmove|score|pv)" | head -5
echo ""

echo "==================== GAME 2 ANALYSIS ===================="
echo ""

# Game 2, Position 2a: Before 10.Qg3
echo "Position 2a: After 9.Qxf3 Nd4 (before 10.Qg3)"
echo "FEN: r2qk2r/ppp1bppp/3p1n2/4p3/2BnP3/2NP1Q1P/PPP2PP1/R1B2RK1 w kq - 2 10"
echo ""
echo "SeaJay evaluation:"
echo -e "position fen r2qk2r/ppp1bppp/3p1n2/4p3/2BnP3/2NP1Q1P/PPP2PP1/R1B2RK1 w kq - 2 10\ngo depth 10\nquit" | $SEAJAY | grep -E "(bestmove|score|pv)"
echo ""
echo "Stockfish evaluation:"
echo -e "position fen r2qk2r/ppp1bppp/3p1n2/4p3/2BnP3/2NP1Q1P/PPP2PP1/R1B2RK1 w kq - 2 10\nsetoption name Threads value 1\ngo depth 15\nquit" | $STOCKFISH | grep -E "(bestmove|score|pv)" | head -5
echo ""
echo "----------------------------------------"
echo ""

# Game 2, Position 2b: Before 11.Bh6
echo "Position 2b: After 10.Qg3 Nxc2 (before 11.Bh6)"
echo "FEN: r2qk2r/ppp1bppp/3p1n2/4p3/2B1P3/2NP2QP/PPn2PP1/R1B2RK1 w kq - 0 11"
echo ""
echo "SeaJay evaluation:"
echo -e "position fen r2qk2r/ppp1bppp/3p1n2/4p3/2B1P3/2NP2QP/PPn2PP1/R1B2RK1 w kq - 0 11\ngo depth 10\nquit" | $SEAJAY | grep -E "(bestmove|score|pv)"
echo ""
echo "Stockfish evaluation:"
echo -e "position fen r2qk2r/ppp1bppp/3p1n2/4p3/2B1P3/2NP2QP/PPn2PP1/R1B2RK1 w kq - 0 11\nsetoption name Threads value 1\ngo depth 15\nquit" | $STOCKFISH | grep -E "(bestmove|score|pv)" | head -5
echo ""
echo "----------------------------------------"
echo ""

# Game 2, Position 2c: Before 17.a4
echo "Position 2c: After 16.Qd2 Qh4 (before 17.a4)"
echo "FEN: r4rk1/ppp2pbp/3p4/4p3/2B1P2q/2NP3P/PPP1QPP1/R5K1 w - - 3 17"
echo ""
echo "SeaJay evaluation:"
echo -e "position fen r4rk1/ppp2pbp/3p4/4p3/2B1P2q/2NP3P/PPP1QPP1/R5K1 w - - 3 17\ngo depth 10\nquit" | $SEAJAY | grep -E "(bestmove|score|pv)"
echo ""
echo "Stockfish evaluation:"
echo -e "position fen r4rk1/ppp2pbp/3p4/4p3/2B1P2q/2NP3P/PPP1QPP1/R5K1 w - - 3 17\nsetoption name Threads value 1\ngo depth 15\nquit" | $STOCKFISH | grep -E "(bestmove|score|pv)" | head -5
echo ""
echo "----------------------------------------"
echo ""

# Game 2, Position 2d: Before 24.b3
echo "Position 2d: After 23.Nb6 Rbd8 (before 24.b3)"
echo "FEN: 3r1rk1/1p2bpbp/pNNp4/P3p3/2B1P3/3P3P/PPPq1PP1/3Q2K1 w - - 2 24"
echo ""
echo "SeaJay evaluation:"
echo -e "position fen 3r1rk1/1p2bpbp/pNNp4/P3p3/2B1P3/3P3P/PPPq1PP1/3Q2K1 w - - 2 24\ngo depth 10\nquit" | $SEAJAY | grep -E "(bestmove|score|pv)"
echo ""
echo "Stockfish evaluation:"
echo -e "position fen 3r1rk1/1p2bpbp/pNNp4/P3p3/2B1P3/3P3P/PPPq1PP1/3Q2K1 w - - 2 24\nsetoption name Threads value 1\ngo depth 15\nquit" | $STOCKFISH | grep -E "(bestmove|score|pv)" | head -5
echo ""
echo "----------------------------------------"
echo ""

# Game 2, Position 2e: Before 34.Rf5
echo "Position 2e: After 33.Nf6+ Kg7 (before 34.Rf5)"
echo "FEN: 2r5/4rkp1/p2p1N2/P3p3/1bB1PR2/3P3P/PP4P1/7K w - - 0 34"
echo ""
echo "SeaJay evaluation:"
echo -e "position fen 2r5/4rkp1/p2p1N2/P3p3/1bB1PR2/3P3P/PP4P1/7K w - - 0 34\ngo depth 10\nquit" | $SEAJAY | grep -E "(bestmove|score|pv)"
echo ""
echo "Stockfish evaluation:"
echo -e "position fen 2r5/4rkp1/p2p1N2/P3p3/1bB1PR2/3P3P/PP4P1/7K w - - 0 34\nsetoption name Threads value 1\ngo depth 15\nquit" | $STOCKFISH | grep -E "(bestmove|score|pv)" | head -5
echo ""
echo "----------------------------------------"
echo ""

# Game 2, Position 2f: Before 47.Kh1
echo "Position 2f: After 46...Bc5+ (before 47.Kh1)"
echo "FEN: 8/8/1BB2p1p/2b5/4P2P/3P4/r5P1/6K1 w - - 1 47"
echo ""
echo "SeaJay evaluation:"
echo -e "position fen 8/8/1BB2p1p/2b5/4P2P/3P4/r5P1/6K1 w - - 1 47\ngo depth 10\nquit" | $SEAJAY | grep -E "(bestmove|score|pv)"
echo ""
echo "Stockfish evaluation:"
echo -e "position fen 8/8/1BB2p1p/2b5/4P2P/3P4/r5P1/6K1 w - - 1 47\nsetoption name Threads value 1\ngo depth 15\nquit" | $STOCKFISH | grep -E "(bestmove|score|pv)" | head -5
echo ""