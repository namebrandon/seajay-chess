#pragma once

#include "../core/types.h"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace seajay::eval {

// Structure to trace all evaluation components
struct EvalTrace {
    // Material and PST
    Score material;
    Score pst;
    Score pstMg;  // Middlegame PST component
    Score pstEg;  // Endgame PST component
    int phase256;  // Phase value 0-256 (0=endgame, 256=middlegame)
    Score materialMg;  // Middlegame material component (for interpolation visibility)
    Score materialEg;  // Endgame material component
    
    // Pawn structure
    Score passedPawns;
    Score candidatePawns;
    Score isolatedPawns;
    Score doubledPawns;
    Score backwardPawns;
    Score semiOpenLiability;
    Score loosePawns;
    Score pawnIslands;

    // Piece evaluation
    Score bishopPair;
    Score bishopColor;
    Score pawnTension;
    Score pawnPushThreats;
    Score pawnInfiltration;
    Score mobility;
    Score knightOutposts;
    Score rookFiles;
    Score rookKingProximity;
    Score threats;

    // King safety
    Score kingSafety;
    int qs3DangerPenalty[2] = {0, 0};
    int threatSuppressedQueenPenalty[2] = {0, 0};
    
    // Pawn hash metadata
    uint64_t pawnKey = 0;        // Zobrist pawn hash key at evaluation time
    bool pawnCacheHit = false;   // Whether pawn cache provided the entry

    // Individual component tracking for passed pawns
    struct PassedPawnDetail {
        int whiteCount = 0;
        int blackCount = 0;
        Score whiteBonus;
        Score blackBonus;
        bool whiteHasProtected = false;
        bool blackHasProtected = false;
        bool whiteHasConnected = false;
        bool blackHasConnected = false;
        bool whiteHasBlockaded = false;
        bool blackHasBlockaded = false;
        bool whiteHasUnstoppable = false;
        bool blackHasUnstoppable = false;
        bool whitePathFree = false;
        bool blackPathFree = false;
        bool whiteStopDefended = false;
        bool blackStopDefended = false;
        bool whiteRookSupport = false;
        bool blackRookSupport = false;
        int whiteMaxRank = 0;
        int blackMaxRank = 0;
        int whiteFriendlyKingDist = 0;
        int whiteEnemyKingDist = 0;
        int blackFriendlyKingDist = 0;
        int blackEnemyKingDist = 0;
    } passedDetail;
    
    // Individual component tracking for mobility
    struct MobilityDetail {
        int whiteKnightMoves = 0;
        int whiteBishopMoves = 0;
        int whiteRookMoves = 0;
        int whiteQueenMoves = 0;
        int blackKnightMoves = 0;
        int blackBishopMoves = 0;
        int blackRookMoves = 0;
        int blackQueenMoves = 0;
    } mobilityDetail;

    struct BishopColorDetail {
        int lightBishops[2] = {0, 0};
        int darkBishops[2] = {0, 0};
        int lightPawns[2] = {0, 0};
        int darkPawns[2] = {0, 0};
        int harmonyPairs[2] = {0, 0};
        int tensionPairs[2] = {0, 0};
        int blockedCentralSameRaw[2] = {0, 0};
    } bishopColorDetail;

    struct PawnSpanDetail {
        int tension[2] = {0, 0};
        int pushReady[2] = {0, 0};
        int infiltration[2] = {0, 0};
    } pawnSpanDetail;
    
    // Reset all values
    void reset() {
        material = Score(0);
        pst = Score(0);
        pstMg = Score(0);
        pstEg = Score(0);
        phase256 = 0;
        materialMg = Score(0);
        materialEg = Score(0);

        passedPawns = Score(0);
        candidatePawns = Score(0);
        isolatedPawns = Score(0);
        doubledPawns = Score(0);
        backwardPawns = Score(0);
        semiOpenLiability = Score(0);
        loosePawns = Score(0);
        pawnIslands = Score(0);
        
        bishopPair = Score(0);
        bishopColor = Score(0);
        pawnTension = Score(0);
        pawnPushThreats = Score(0);
        pawnInfiltration = Score(0);
        mobility = Score(0);
        knightOutposts = Score(0);
        rookFiles = Score(0);
        rookKingProximity = Score(0);
        threats = Score(0);
        
        kingSafety = Score(0);
        
        passedDetail = PassedPawnDetail();
        mobilityDetail = MobilityDetail();
        bishopColorDetail = BishopColorDetail();
        pawnSpanDetail = PawnSpanDetail();
        pawnKey = 0ULL;
        pawnCacheHit = false;
        qs3DangerPenalty[WHITE] = qs3DangerPenalty[BLACK] = 0;
        threatSuppressedQueenPenalty[WHITE] = threatSuppressedQueenPenalty[BLACK] = 0;
    }
    
    // Calculate total score
    Score total() const {
        return material + pst + passedPawns + candidatePawns + isolatedPawns + doubledPawns + 
               backwardPawns + semiOpenLiability + loosePawns + pawnIslands + bishopPair + bishopColor + pawnTension + pawnPushThreats + pawnInfiltration + mobility + 
               knightOutposts + rookFiles + rookKingProximity + threats + kingSafety;
    }
    
    // Print formatted evaluation breakdown
    void print(Color sideToMove) const {
        for (const auto& line : toStructuredLines(sideToMove)) {
            std::cout << line << std::endl;
        }
    }

    std::vector<std::string> toStructuredLines(Color sideToMove) const {
        std::vector<std::string> lines;
        lines.reserve(16);

        auto pushLine = [&lines](std::string payload) {
            lines.emplace_back("info eval " + std::move(payload));
        };

        pushLine("header version=1");

        Score totalWhite = total();
        Score finalScore = (sideToMove == WHITE) ? totalWhite : -totalWhite;

        {
            std::ostringstream oss;
            oss << "summary side=" << (sideToMove == WHITE ? "white" : "black")
                << " total_white_cp=" << totalWhite.value()
                << " final_cp=" << finalScore.value();
            pushLine(oss.str());
        }

        {
            std::ostringstream oss;
            oss << "phase value=" << phase256
                << " mg_pct=" << (phase256 * 100 / 256)
                << " eg_pct=" << ((256 - phase256) * 100 / 256);
            pushLine(oss.str());
        }

        {
            std::ostringstream oss;
            oss << std::hex;
            oss << "pawn_cache key=0x" << pawnKey;
            oss << std::dec;
            oss << " hit=" << (pawnCacheHit ? 1 : 0);
            pushLine(oss.str());
        }

        auto emitTerm = [&pushLine](const std::string& name, Score value) {
            std::ostringstream oss;
            oss << "term name=" << name << " cp=" << value.value();
            pushLine(oss.str());
        };

        // Material and PST exposure
        {
            std::ostringstream oss;
            oss << "term name=material cp=" << material.value()
                << " mg=" << materialMg.value()
                << " eg=" << materialEg.value();
            pushLine(oss.str());
        }

        {
            std::ostringstream oss;
            oss << "term name=pst cp=" << pst.value()
                << " mg=" << pstMg.value()
                << " eg=" << pstEg.value();
            pushLine(oss.str());
        }

        {
            std::ostringstream oss;
            oss << "term name=passed_pawns cp=" << passedPawns.value()
                << " white=" << passedDetail.whiteCount
                << " black=" << passedDetail.blackCount;
            if (passedDetail.whiteBonus.value() != 0) {
                oss << " white_cp=" << passedDetail.whiteBonus.value();
            }
            if (passedDetail.blackBonus.value() != 0) {
                oss << " black_cp=" << passedDetail.blackBonus.value();
            }
            if (passedDetail.whiteHasProtected) oss << " white_protected=1";
            if (passedDetail.blackHasProtected) oss << " black_protected=1";
            if (passedDetail.whiteHasConnected) oss << " white_connected=1";
            if (passedDetail.blackHasConnected) oss << " black_connected=1";
            if (passedDetail.whiteHasBlockaded) oss << " white_blockaded=1";
            if (passedDetail.blackHasBlockaded) oss << " black_blockaded=1";
            if (passedDetail.whiteHasUnstoppable) oss << " white_unstoppable=1";
            if (passedDetail.blackHasUnstoppable) oss << " black_unstoppable=1";
            if (passedDetail.whitePathFree) oss << " white_path_free=1";
            if (passedDetail.blackPathFree) oss << " black_path_free=1";
            if (passedDetail.whiteStopDefended) oss << " white_stop_defended=1";
            if (passedDetail.blackStopDefended) oss << " black_stop_defended=1";
            if (passedDetail.whiteRookSupport) oss << " white_rook_support=1";
            if (passedDetail.blackRookSupport) oss << " black_rook_support=1";
            if (passedDetail.whiteMaxRank) oss << " white_max_rank=" << passedDetail.whiteMaxRank;
            if (passedDetail.blackMaxRank) oss << " black_max_rank=" << passedDetail.blackMaxRank;
            if (passedDetail.whiteFriendlyKingDist) oss << " white_king_dist=" << passedDetail.whiteFriendlyKingDist;
            if (passedDetail.whiteEnemyKingDist) oss << " white_enemy_king_dist=" << passedDetail.whiteEnemyKingDist;
            if (passedDetail.blackFriendlyKingDist) oss << " black_king_dist=" << passedDetail.blackFriendlyKingDist;
            if (passedDetail.blackEnemyKingDist) oss << " black_enemy_king_dist=" << passedDetail.blackEnemyKingDist;
            pushLine(oss.str());
        }

        emitTerm("candidate_pawns", candidatePawns);

        emitTerm("isolated_pawns", isolatedPawns);
        emitTerm("doubled_pawns", doubledPawns);
        emitTerm("backward_pawns", backwardPawns);
        emitTerm("semi_open_liability", semiOpenLiability);
        emitTerm("loose_pawns", loosePawns);

        emitTerm("pawn_islands", pawnIslands);
        emitTerm("bishop_pair", bishopPair);
        emitTerm("bishop_color", bishopColor);
        emitTerm("pawn_tension", pawnTension);
        emitTerm("pawn_push_threats", pawnPushThreats);
        emitTerm("pawn_infiltration", pawnInfiltration);

        {
            std::ostringstream oss;
            oss << "term name=mobility cp=" << mobility.value()
                << " wn=" << mobilityDetail.whiteKnightMoves
                << " wb=" << mobilityDetail.whiteBishopMoves
                << " wr=" << mobilityDetail.whiteRookMoves
                << " wq=" << mobilityDetail.whiteQueenMoves
                << " bn=" << mobilityDetail.blackKnightMoves
                << " bb=" << mobilityDetail.blackBishopMoves
                << " br=" << mobilityDetail.blackRookMoves
                << " bq=" << mobilityDetail.blackQueenMoves;
            pushLine(oss.str());
        }

        emitTerm("knight_outposts", knightOutposts);
        emitTerm("rook_files", rookFiles);
        emitTerm("rook_king_proximity", rookKingProximity);
        emitTerm("threats", threats);
        emitTerm("king_safety", kingSafety);

        {
            const auto& detail = bishopColorDetail;
            std::ostringstream oss;
            oss << "detail name=bishop_color"
                << " white_light_bishops=" << detail.lightBishops[WHITE]
                << " white_dark_bishops=" << detail.darkBishops[WHITE]
                << " white_light_pawns=" << detail.lightPawns[WHITE]
                << " white_dark_pawns=" << detail.darkPawns[WHITE]
                << " white_harmony_pairs=" << detail.harmonyPairs[WHITE]
                << " white_tension_pairs=" << detail.tensionPairs[WHITE]
                << " white_blocked_same=" << detail.blockedCentralSameRaw[WHITE]
                << " black_light_bishops=" << detail.lightBishops[BLACK]
                << " black_dark_bishops=" << detail.darkBishops[BLACK]
                << " black_light_pawns=" << detail.lightPawns[BLACK]
                << " black_dark_pawns=" << detail.darkPawns[BLACK]
                << " black_harmony_pairs=" << detail.harmonyPairs[BLACK]
                << " black_tension_pairs=" << detail.tensionPairs[BLACK]
                << " black_blocked_same=" << detail.blockedCentralSameRaw[BLACK];
            pushLine(oss.str());
        }

        {
            const auto& span = pawnSpanDetail;
            std::ostringstream oss;
            oss << "detail name=pawn_span"
                << " white_tension=" << span.tension[WHITE]
                << " white_push_ready=" << span.pushReady[WHITE]
                << " white_infiltration=" << span.infiltration[WHITE]
                << " black_tension=" << span.tension[BLACK]
                << " black_push_ready=" << span.pushReady[BLACK]
                << " black_infiltration=" << span.infiltration[BLACK];
            pushLine(oss.str());
        }

        {
            std::ostringstream oss;
            oss << "total white_cp=" << totalWhite.value()
                << " final_cp=" << finalScore.value();
            pushLine(oss.str());
        }

        if (qs3DangerPenalty[WHITE] != 0 || qs3DangerPenalty[BLACK] != 0) {
            std::ostringstream oss;
            oss << "detail name=qs3_king_danger"
                << " white_cp=" << qs3DangerPenalty[WHITE]
                << " black_cp=" << qs3DangerPenalty[BLACK];
            pushLine(oss.str());
        }

        if (threatSuppressedQueenPenalty[WHITE] != 0 || threatSuppressedQueenPenalty[BLACK] != 0) {
            std::ostringstream oss;
            oss << "detail name=threat_suppression"
                << " white_cp=" << threatSuppressedQueenPenalty[WHITE]
                << " black_cp=" << threatSuppressedQueenPenalty[BLACK];
            pushLine(oss.str());
        }

        return lines;
    }
};

} // namespace seajay::eval
