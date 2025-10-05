static eval::Score negamax_legacy(Board& board, 
                   int depth, 
                   int ply,
                   eval::Score alpha,
                   eval::Score beta,
                   SearchInfo& searchInfo,
                   SearchData& info,
                   const SearchLimits& limits,
                   TranspositionTable* tt,
                   TriangularPV* pv,
                   bool isPvNode) {
    
    
    // Debug output at root for deeper searches
    if (ply == 0 && depth >= 4 && !limits.suppressDebugOutput) {
        std::cerr << "Negamax: Starting search at depth " << depth << std::endl;
    }
    
    // Debug assertions
    assert(depth >= 0);
    assert(ply >= 0);
    assert(ply < 128);  // Prevent stack overflow
    
    // Debug assertion for valid search window
    assert(alpha < beta && "Alpha must be less than beta in negamax search");
    
    // In release mode, still handle invalid window gracefully
    if (alpha >= beta) {
        // This shouldn't happen in correct negamax, but handle it gracefully
        return alpha;  // Return alpha as a fail-soft bound
    }
    
    // Time check - only check periodically to reduce overhead
    if ((info.nodes & (SearchData::TIME_CHECK_INTERVAL - 1)) == 0) {
        // Check external stop flag first (UCI stop command or LazySMP global stop)
        if (limits.stopFlag && limits.stopFlag->load(std::memory_order_relaxed)) {
            info.stopped = true;
            return eval::Score::zero();
        }
        // Then check time limit
        if (info.checkTime()) {
            info.stopped = true;
            return eval::Score::zero();
        }
    }
    
    // Increment node counter
    info.nodes++;
    
    // Node explosion diagnostics: Track depth distribution
    if (limits.nodeExplosionDiagnostics) {
        g_nodeExplosionStats.recordNodeAtDepth(ply);
    }
    
    
    // Phase 1 & 2, enhanced in Phase 6: Check for periodic UCI info updates with smart throttling
    // Check periodically regardless of ply depth (since we spend most time at deeper plies)
    if (info.nodes > 0 && (info.nodes & 0xFFF) == 0) {
        // Use virtual method instead of expensive dynamic_cast
        // This eliminates RTTI overhead in the hot path
        if (info.isIterativeSearch()) {
            auto* iterativeInfo = static_cast<IterativeSearchData*>(&info);
            if (iterativeInfo->shouldSendInfo(true)) {  // Phase 6: Check with score change flag
                // UCI Score Conversion FIX: Use root side-to-move stored in SearchData
                sendCurrentSearchInfo(*iterativeInfo, info.rootSideToMove, tt);
                iterativeInfo->recordInfoSent(iterativeInfo->bestScore);  // Phase 6: Pass current score
            }
        }
    }
    
    // Phase P2: Store PV status in search stack
    searchInfo.setPvNode(ply, isPvNode);

    // Stage 0: Keep extension totals in sync for current node (parent sets applied count)
    searchInfo.setExtensionApplied(ply, searchInfo.extensionApplied(ply));
    
    // Update selective depth (maximum depth reached)
    if (ply > info.seldepth) {
        info.seldepth = ply;
    }

    // Phase 4.1: Default history context to none before ordering decisions
    info.clearHistoryContext(ply);

    // Terminal node - enter quiescence search or return static evaluation
    if (depth <= 0) {
        // Stage 14: Quiescence search - ALWAYS compiled in, controlled by UCI option
        // This is a CORE FEATURE and should never be behind compile flags!
        if (info.useQuiescence) {
            // Candidate 9: Detect time pressure for panic mode
            // Panic mode activates when remaining time < 100ms
            bool inPanicMode = false;
            if (info.timeLimit != std::chrono::milliseconds::max()) {
                auto remainingTime = info.timeLimit - info.elapsed();
                inPanicMode = (remainingTime < std::chrono::milliseconds(100));
            }
            // Node explosion diagnostics: Track quiescence entry
            if (limits.nodeExplosionDiagnostics) {
                g_nodeExplosionStats.recordQuiescenceEntry(ply);
            }
            // Use quiescence search to resolve tactical sequences
            return quiescence(board, ply, 0, alpha, beta, searchInfo, info, limits, *tt, 0, inPanicMode);
        }
        // Fallback: return static evaluation (only if quiescence disabled via UCI)
        return board.evaluate();
    }
    
    // Sub-phase 4B: Draw Detection Order
    // CRITICAL ORDER:
    // 1. Check for checkmate/stalemate FIRST (terminal conditions)
    // 2. Check for repetition SECOND
    // 3. Check for fifty-move rule THIRD
    // 4. Only THEN probe TT
    
    // Initialize TT move and info for singular extensions
    Move ttMove = NO_MOVE;
    eval::Score ttScore = eval::Score::zero();
    Bound ttBound = Bound::NONE;
    int ttDepth = -1;
    
    // Phase 4.2.c: Compute static eval early and preserve it for TT storage
    // This will be the true static evaluation, not the search score
    eval::Score staticEval = eval::Score::zero();
    bool staticEvalComputed = false;
    
    // Check if we're in check (needed for null move and other decisions)
    bool weAreInCheck = inCheck(board);
    
    // Check extension: Extend search by 1 ply when in check
    // This is a fundamental search extension present in all competitive engines
    // It helps the engine see through forcing sequences and avoid horizon effects
    if (weAreInCheck) {
        depth++;
    }
    
    // Phase 3.2: We'll generate pseudo-legal moves later, after TT/null pruning
    // Obtain per-ply scratch MoveList to avoid deep recursion stack growth
    MoveList& moves = info.acquireMoveList(ply);
    
    // Sub-phase 4B: Establish correct probe order
    // 1. Check repetition FIRST (fastest, most common draw in search)
    // 2. Check fifty-move rule SECOND (less common)
    // 3. Only THEN probe TT (after all draw conditions)
    
    // Never check draws below was guarding ply>0.
    // For robustness, do a minimal ROOT draw check too, before probing TT, to avoid
    // returning a TT score in a terminal draw while not having a best move.
    if (ply > 0) {
        // PERFORMANCE OPTIMIZATION: Strategic draw checking
        bool shouldCheckDraw = false;
        
        // Get info about last move from search stack (if available)
        bool lastMoveWasCapture = false;
        if (ply > 0 && ply - 1 < 128) {
            Move lastMove = searchInfo.getStackEntry(ply - 1).move;
            if (lastMove != NO_MOVE) {
                lastMoveWasCapture = isCapture(lastMove);
            }
        }
        
        shouldCheckDraw = 
            weAreInCheck ||                         // Always after checks (use cached value)
            lastMoveWasCapture ||                   // After captures (50-move reset)
            (ply >= 4 && (ply & 3) == 0);          // Every 4th ply for repetitions
        
        // Check for draws when necessary
        if (shouldCheckDraw && board.isDrawInSearch(searchInfo, ply)) {
            return eval::Score::draw();  // Draw score
        }
    }

    // Minimal root draw handling (Option B support): if root position is a draw,
    // return draw score but also ensure a legal move is available for UCI output.
    if (ply == 0) {
        if (board.isDrawInSearch(searchInfo, ply)) {
            MoveList legalRoot;
            MoveGenerator::generateLegalMoves(board, legalRoot);
            if (!legalRoot.empty()) {
                info.bestMove = legalRoot[0];
                info.bestScore = eval::Score::draw();
            }
            return eval::Score::draw();
        }
    }

    // Probe TT even at root (after draw detection if any)
    // IMPORTANT: At root (ply==0) we DO NOT ALLOW hard returns from TT.
    // We only use TT info to tighten bounds and seed ordering, then we still
    // generate moves and search at least one move to establish bestMove/PV.
    {
        TTEntry* ttEntry = nullptr;
        if (tt && tt->isEnabled()) {
            Hash zobristKey = board.zobristKey();
            tt->prefetch(zobristKey);
            ttEntry = tt->probe(zobristKey);
            info.ttProbes++;

            if (ttEntry && ttEntry->key32 == (zobristKey >> 32)) {
                info.ttHits++;

                if (ttEntry->depth >= depth) {
                    eval::Score tts(ttEntry->score);
                    ttBound = ttEntry->bound();

                    if (tts.value() >= MATE_BOUND)      tts = eval::Score(tts.value() - ply);
                    else if (tts.value() <= -MATE_BOUND) tts = eval::Score(tts.value() + ply);

                    if (ttBound == Bound::EXACT) {
                        if (ply > 0) {
                            info.ttCutoffs++;
                            return tts;  // non-root: safe to cutoff
                        } else {
                            // root: tighten alpha around exact score but do not return
                            if (tts > alpha) alpha = tts;
                            if (tts < beta)  beta = tts;
                            // keep window valid at root
                            if (alpha >= beta) alpha = eval::Score(beta.value() - 1);
                        }
                    } else if (ttBound == Bound::LOWER) {
                        if (ply > 0 && tts >= beta) {
                            info.ttCutoffs++;
                            return tts;  // non-root: beta cutoff
                        }
                        if (tts > alpha) alpha = tts;
                    } else if (ttBound == Bound::UPPER) {
                        if (ply > 0 && tts <= alpha) {
                            info.ttCutoffs++;
                            return tts;  // non-root: alpha cutoff
                        }
                        if (tts < beta) beta = tts;
                        if (alpha >= beta) {
                            if (ply > 0) {
                                info.ttCutoffs++;
                                return alpha;  // non-root: window closed
                            } else {
                                // root: keep window valid
                                alpha = eval::Score(beta.value() - 1);
                            }
                        }
                    }
                }

                // Extract TT move for ordering
                ttMove = static_cast<Move>(ttEntry->move);
                if (ttMove != NO_MOVE) {
                    Square from = moveFrom(ttMove);
                    Square to = moveTo(ttMove);
                    if (from < 64 && to < 64 && from != to) {
                        info.ttMoveHits++;
                        ttScore = eval::Score(ttEntry->score);
                        ttBound = ttEntry->bound();
                        ttDepth = ttEntry->depth;
                    } else {
                        ttMove = NO_MOVE;
                        info.ttCollisions++;
                        std::cerr << "WARNING: Corrupted TT move detected: "
                                  << std::hex << ttEntry->move << std::dec << std::endl;
                    }
                }

                if (!staticEvalComputed && ttEntry->evalScore != TT_EVAL_NONE) {
                    staticEval = eval::Score(ttEntry->evalScore);
                    staticEvalComputed = true;
                }
            }
        }
    }
    
    // Phase 4.2.c: Compute static eval if not in check and haven't gotten it from TT
    // We need this for pruning decisions and to store in TT later
    if (!staticEvalComputed && !weAreInCheck) {
        staticEval = board.evaluate();
        staticEvalComputed = true;
        // Cache it in search stack for potential reuse
        searchInfo.setStaticEval(ply, staticEval);
    }
    
    // Stage 21 Phase A4: Null Move Pruning with Static Null Move and Tuning
    // Constants for null move pruning
    constexpr eval::Score ZUGZWANG_THRESHOLD = eval::Score(500 + 330);  // Rook + Bishop value (original)
    
    // Phase 1.2: Enhanced reverse futility pruning (static null move)
    // Extended to depth 8 with more aggressive shallow margins
    if (!isPvNode && depth <= 8 && depth > 0 && !weAreInCheck && std::abs(beta.value()) < MATE_BOUND - MAX_PLY) {
        // Phase 4.2.c: Use our pre-computed static eval
        if (staticEvalComputed) {
            // More aggressive margin at shallow depths, conservative at deeper
            // depth 1: 85, 2: 170, 3: 255, 4: 320, 5: 375, 6: 420, 7: 455, 8: 480
            int baseMargin = 85;
            eval::Score margin;
            if (depth <= 3) {
                margin = eval::Score(baseMargin * depth);
            } else {
                // Slower growth for deeper depths
                margin = eval::Score(baseMargin * 3 + (depth - 3) * (baseMargin / 2));
            }
            
            if (staticEval - margin >= beta) {
                info.nullMoveStats.staticCutoffs++;
                
                // TT remediation Phase 2.2: Add TT store for static-null pruning
                // TT pollution fix: Only store at depth >= 2 to reduce low-value writes
                if (tt && tt->isEnabled() && depth >= 2) {
                    uint64_t zobristKey = board.zobristKey();
                    // FIX: Use beta for fail-high, consistent with null-move
                    int16_t scoreToStore = beta.value();
                    
                    // Adjust mate scores before storing
                    if (beta.value() >= MATE_BOUND - MAX_PLY) {
                        scoreToStore = beta.value() + ply;
                    } else if (beta.value() <= -MATE_BOUND + MAX_PLY) {
                        scoreToStore = beta.value() - ply;
                    }
                    
                    // Store with NO_MOVE since this is a static pruning decision
                    // FIX: Use LOWER bound for fail-high (score >= beta)
                    // FIX: Use depth 0 since static-null is heuristic, not searched
                    int16_t evalToStore = staticEvalComputed ? staticEval.value() : TT_EVAL_NONE;
                    tt->store(zobristKey, NO_MOVE, scoreToStore, evalToStore,
                             0, Bound::LOWER);  // Depth 0 for heuristic bound
                    info.ttStores++;
                    // No longer track as missing store
                } else if (!tt || !tt->isEnabled()) {
                    // Only track as missing if TT is disabled
                    info.nullMoveStats.staticNullNoStore++;
                } else {
                    // Skipped due to depth gating (depth < 2)
                    info.nullMoveStats.staticNullNoStore++;
                }
                
                // Node explosion diagnostics: Track reverse futility pruning
                if (limits.nodeExplosionDiagnostics) {
                    g_nodeExplosionStats.recordStaticNullMovePrune(depth);
                }
                return staticEval - margin;  // Return reduced score for safety
            }
        }
    }
    
    // Regular null move pruning
    // Check if we can do null move
    const auto& config = seajay::getConfig();

    bool canDoNull = !isPvNode                                  // Phase P2: No null in PV nodes!
                    && !weAreInCheck                              // Not in check
                    && depth >= limits.nullMoveMinDepth          // Minimum depth (UCI configurable)
                    && ply > 0                                   // Not at root
                    && !searchInfo.wasNullMove(ply - 1)         // No consecutive nulls
                    && board.nonPawnMaterial(board.sideToMove()) > ZUGZWANG_THRESHOLD  // Original detection
                    && std::abs(beta.value()) < MATE_BOUND - MAX_PLY;  // Not near mate

    if (canDoNull && staticEvalComputed && config.nullMoveDesperationMargin > 0) {
        if (staticEval <= alpha - eval::Score(config.nullMoveDesperationMargin)) {
            canDoNull = false;
        }
    }
    
    if (canDoNull && limits.useNullMove) {
        info.nullMoveStats.attempts++;
        
        // Use UCI-configurable reduction values
        int nullMoveReduction = limits.nullMoveReductionBase;
        if (depth >= 6) nullMoveReduction = limits.nullMoveReductionDepth6;
        if (depth >= 12) nullMoveReduction = limits.nullMoveReductionDepth12;
        
        // Dynamic adjustment based on eval margin (UCI configurable)
        if (staticEvalComputed && staticEval - beta > eval::Score(limits.nullMoveEvalMargin)) {
            nullMoveReduction++;
        }
        
        // Phase 4.2: Optional extra reduction when eval margin is very large
        bool aggressiveReductionActive = false;
        if (limits.useAggressiveNullMove && staticEvalComputed && depth >= 10) {
            const eval::Score evalGap = staticEval - beta;
            const int aggressiveThreshold = limits.nullMoveEvalMargin + AGGRESSIVE_NULL_MARGIN_OFFSET;
            if (evalGap.value() > aggressiveThreshold) {
                info.nullMoveStats.aggressiveCandidates++;

                bool allowAggressive = true;
                if (limits.aggressiveNullMinEval > 0 && staticEval.value() < limits.aggressiveNullMinEval) {
                    allowAggressive = false;
                }
                if (allowAggressive && limits.aggressiveNullRequirePositiveBeta && beta.value() <= 0) {
                    allowAggressive = false;
                }
                // Avoid aggressive reductions when we would immediately pay the
                // verification cost anyway (depth close to verify threshold).
                if (allowAggressive && depth >= limits.nullMoveVerifyDepth) {
                    allowAggressive = false;
                }
                if (allowAggressive && limits.aggressiveNullMaxApplications > 0 &&
                    info.nullMoveStats.aggressiveApplied >= static_cast<uint64_t>(limits.aggressiveNullMaxApplications)) {
                    allowAggressive = false;
                    info.nullMoveStats.aggressiveCapHits++;
                }

                bool ttBlocksExtra = false;
                if (allowAggressive && ttDepth >= depth - 1) {
                    if ((ttBound == Bound::UPPER || ttBound == Bound::EXACT) && ttScore < beta) {
                        ttBlocksExtra = true;
                    }
                }

                if (!allowAggressive) {
                    info.nullMoveStats.aggressiveSuppressed++;
                } else if (ttBlocksExtra) {
                    info.nullMoveStats.aggressiveBlockedByTT++;
                } else {
                    int proposedReduction = std::min(nullMoveReduction + 1, depth - 1);
                    if (proposedReduction > nullMoveReduction) {
                        nullMoveReduction = proposedReduction;
                        aggressiveReductionActive = true;
                        info.nullMoveStats.aggressiveApplied++;
                    } else {
                        info.nullMoveStats.aggressiveSuppressed++;
                    }
                }
            }
        }

        // Ensure we don't reduce too much
        nullMoveReduction = std::min(nullMoveReduction, depth - 1);
        
        // Make null move
        Board::UndoInfo nullUndo;
        board.makeNullMove(nullUndo);
        searchInfo.setNullMove(ply, true);
        
        // Search with adaptive reduction (null window around beta)
        eval::Score nullScore = -negamax(
            board,
            depth - 1 - nullMoveReduction,
            ply + 1,
            -beta,
            -beta + eval::Score(1),
            searchInfo,
            info,
            limits,
            tt,
            nullptr,  // Phase PV1: Pass nullptr for now
            false  // Phase P2: Null move searches are never PV nodes
        );
        
        // Unmake null move
        board.unmakeNullMove(nullUndo);
        searchInfo.setNullMove(ply, false);
        
        // Check for cutoff
        if (nullScore >= beta) {
            info.nullMoveStats.cutoffs++;
            if (aggressiveReductionActive) {
                info.nullMoveStats.aggressiveCutoffs++;
            }
            
            // Phase 1.5b: Deeper verification search at configurable depth
            if (depth >= limits.nullMoveVerifyDepth) {  // UCI configurable threshold
                // Verification search at depth - R (one ply deeper than null move)
                eval::Score verifyScore = negamax(
                    board,
                    depth - nullMoveReduction,  // depth - R (matches Stockfish/Laser)
                    ply,
                    beta - eval::Score(1),
                    beta,
                    searchInfo,
                    info,
                    limits,
                    tt,
                    nullptr,  // Phase PV1: Pass nullptr for now
                    false
                );
                
                if (verifyScore < beta) {
                    // Verification failed, don't trust null move
                    info.nullMoveStats.verificationFails++;
                    if (aggressiveReductionActive) {
                        info.nullMoveStats.aggressiveVerifyFails++;
                    }
                    // Continue with normal search instead of returning
                } else {
                    // Verification passed, null move cutoff is valid
                    // TT remediation Phase 2.1: Add TT store for null-move cutoff
                    if (tt && tt->isEnabled()) {
                        uint64_t zobristKey = board.zobristKey();
                        int16_t scoreToStore = beta.value();  // Store beta for fail-high
                        
                        // Adjust mate scores before storing
                        if (beta.value() >= MATE_BOUND - MAX_PLY) {
                            scoreToStore = beta.value() + ply;
                        } else if (beta.value() <= -MATE_BOUND + MAX_PLY) {
                            scoreToStore = beta.value() - ply;
                        }
                        
                        // Store with NO_MOVE since this is a null-move cutoff
                        // Use LOWER bound since we're failing high (score >= beta)
                        tt->store(zobristKey, NO_MOVE, scoreToStore, TT_EVAL_NONE,
                                 static_cast<uint8_t>(depth), Bound::LOWER);
                        info.ttStores++;
                        // No longer track as missing store
                    } else {
                        // Only track as missing if TT is disabled
                        info.nullMoveStats.nullMoveNoStore++;
                    }
                    
                    if (std::abs(nullScore.value()) < MATE_BOUND - MAX_PLY) {
                        if (aggressiveReductionActive) {
                            info.nullMoveStats.aggressiveVerifyPasses++;
                        }
                        return nullScore;
                    } else {
                        // Mate score, return beta instead
                        if (aggressiveReductionActive) {
                            info.nullMoveStats.aggressiveVerifyPasses++;
                        }
                        return beta;
                    }
                }
            } else {
                // Shallow depth, trust null move without verification
                // TT remediation Phase 2.1: Add TT store for null-move cutoff
                if (tt && tt->isEnabled()) {
                    uint64_t zobristKey = board.zobristKey();
                    int16_t scoreToStore = beta.value();  // Store beta for fail-high
                    
                    // Adjust mate scores before storing
                    if (beta.value() >= MATE_BOUND - MAX_PLY) {
                        scoreToStore = beta.value() + ply;
                    } else if (beta.value() <= -MATE_BOUND + MAX_PLY) {
                        scoreToStore = beta.value() - ply;
                    }
                    
                    // Store with NO_MOVE since this is a null-move cutoff
                    // Use LOWER bound since we're failing high (score >= beta)
                    tt->store(zobristKey, NO_MOVE, scoreToStore, TT_EVAL_NONE,
                             static_cast<uint8_t>(depth), Bound::LOWER);
                    info.ttStores++;
                    // No longer track as missing store
                } else {
                    // Only track as missing if TT is disabled
                    info.nullMoveStats.nullMoveNoStore++;
                }
                
                if (std::abs(nullScore.value()) < MATE_BOUND - MAX_PLY) {
                    return nullScore;
                } else {
                    // Mate score, return beta instead
                    return beta;
                }
            }
        }
    } else if (!canDoNull && ply > 0 && board.nonPawnMaterial(board.sideToMove()) <= ZUGZWANG_THRESHOLD) {
        // Track when we avoid null move due to zugzwang
        info.nullMoveStats.zugzwangAvoids++;
    }
    
    // Phase R2.1: Razoring with Enhanced Safety Guards
    // Apply razoring at shallow depths (1-2) when static eval is far below alpha
    // Phase R2.1: Complete skip when tactical, endgame guard, TT context guard
    if (limits.useRazoring && 
        !isPvNode && 
        !weAreInCheck && 
        depth <= 2 && 
        depth >= 1 &&
        std::abs(alpha.value()) < MATE_BOUND - MAX_PLY) {
        
        // Increment attempt counter
        info.razoring.attempts++;
        
        // Phase R2.1: TT context guard - skip if TT suggests we won't fail low
        if (ttBound == Bound::LOWER && ttScore > alpha - eval::Score(50)) {
            info.razoring.ttContextSkips++;
            // Don't razor - TT suggests this position won't fail low
        }
        // Phase R2.1: Endgame guard - 1300cp NPM threshold (matches quiescence)
        else if (board.nonPawnMaterial(board.sideToMove()).value() < 1300 || 
                 board.nonPawnMaterial(~board.sideToMove()).value() < 1300) {
            info.razoring.endgameSkips++;
            // Don't razor in endgames - zugzwang risk
        }
        else {
            // Phase R2.1: Check for tactical moves (SEE >= 0 captures)
            bool hasTacticalMoves = false;
            MoveList captures;
            MoveGenerator::generateCaptures(board, captures);
            
            for (size_t i = 0; i < captures.size(); ++i) {
                Move move = captures[i];
                if (isCapture(move) && seeGE(board, move, 0)) {
                    hasTacticalMoves = true;
                    info.razoring.tacticalSkips++;  // Track when tactical guard triggers
                    break;
                }
            }
            
            // Phase R2.1: Option A - Skip razoring entirely if tactical moves exist
            if (hasTacticalMoves) {
                // Skip razoring at this node - too dangerous
            }
            else {
                // Select margin based on depth
                int razorMargin = (depth == 1) ? limits.razorMargin1 : limits.razorMargin2;
                
                // Check if static eval + margin is still below alpha
                if (staticEval + eval::Score(razorMargin) <= alpha) {
                    // Run quiescence search with scout window
                    eval::Score qScore = quiescence(
                        board, 
                        ply, 
                        0,                    // qsearch depth starts at 0
                        alpha,                // alpha
                        alpha + eval::Score(1), // alpha+1 (scout window)
                        searchInfo, 
                        info, 
                        limits, 
                        *tt, 
                        0,                    // initialAlpha (not used in scout)
                        false                 // panicMode
                    );
                    
                    // If quiescence still fails low, we can return
                    if (qScore <= alpha) {
                        // Update telemetry
                        info.razoring.cutoffs++;
                        info.razoring.depthBuckets[depth - 1]++;
                        info.razoringCutoffs++;  // Legacy counter
                        
                        // Phase R2: Improved TT storage with mate score adjustment
                        if (tt && tt->isEnabled()) {
                            uint64_t zobristKey = board.zobristKey();
                            int16_t scoreToStore = qScore.value();
                            
                            // Adjust mate scores before storing
                            if (qScore.value() >= MATE_BOUND - MAX_PLY) {
                                scoreToStore = qScore.value() + ply;  // Adjust mate score
                            } else if (qScore.value() <= -MATE_BOUND + MAX_PLY) {
                                scoreToStore = qScore.value() - ply;  // Adjust mated score
                            }
                            
                            tt->store(zobristKey, NO_MOVE, scoreToStore, TT_EVAL_NONE, 
                                     static_cast<uint8_t>(depth), Bound::UPPER);
                            info.ttStores++;
                        }
                        
                        return qScore;
                    }  // if (qScore <= alpha)
                }  // if (staticEval + razorMargin <= alpha)
            }  // else (not tactical)
        }  // else (not endgame, not TT skip)
    }  // if razoring enabled
    
    // Phase 4.1: Razoring - DISABLED (OLD VERSION)
    // Testing showed -39.45 Elo loss, likely due to:
    // 1. Interaction with our existing pruning techniques
    // 2. SeaJay's evaluation may not be accurate enough for razoring
    // 3. Our quiescence search implementation may differ from Laser's
    // Razoring requires very careful tuning and may not be suitable for SeaJay's current architecture
    
    // In quiescence search (depth <= 0), handle differently
    if (depth <= 0) {
        // Get info about last move
        bool lastMoveWasCapture = false;
        if (ply > 0 && ply - 1 < 128) {
            Move lastMove = searchInfo.getStackEntry(ply - 1).move;
            if (lastMove != NO_MOVE) {
                lastMoveWasCapture = isCapture(lastMove);
            }
        }
        
        // Only check insufficient material after captures in qsearch
        if (lastMoveWasCapture && board.isInsufficientMaterial()) {
            return eval::Score::draw();
        }
    }
    
    // Get previous move for countermove lookup (CM3.2)
    Move prevMove = NO_MOVE;
    if (ply > 0) {
        prevMove = searchInfo.getStackEntry(ply - 1).move;
    }
    
    // Phase 2a: Use RankedMovePicker if enabled (skip at root for safety)
    // Use optional to avoid dynamic allocation (stack allocation instead)
    std::optional<RankedMovePicker> rankedPicker;
    if (limits.useRankedMovePicker && ply > 0) {
        // Predict whether history/countermove history ordering will be active
        auto historyCtx = SearchData::HistoryContext::None;
        const bool hasHistoryInfra = info.killers && info.history && info.counterMoves;
        if (hasHistoryInfra) {
            if (info.counterMoveHistory && depth >= HISTORY_GATING_DEPTH) {
                historyCtx = SearchData::HistoryContext::Counter;
            } else {
                historyCtx = SearchData::HistoryContext::Basic;
            }
        }
        if (historyCtx != SearchData::HistoryContext::None) {
            info.registerHistoryApplication(ply, historyCtx);
        }
        rankedPicker.emplace(
            board, ttMove, info.killers, info.history, 
            info.counterMoves, info.counterMoveHistory,
            prevMove, ply, depth, info.countermoveBonus, &limits,
            &info
        );
    } else {
        // Generate pseudo-legal moves now that early exits are handled
        MoveGenerator::generateMovesForSearch(board, moves, false);
        
        // CM4.1: Track countermove availability (removed from here to avoid double-counting)
        // Hit tracking now happens only when countermove causes a cutoff or is tried first
        
        // Order moves for better alpha-beta pruning
        // TT move first, then promotions (especially queen), then captures (MVV-LVA), then killers, then quiet moves
        // CM3.3: Pass prevMove and bonus for countermove ordering
        // Phase 4.3.a-fix2: Pass depth for CMH gating
        orderMoves(board, moves, ttMove, &info, ply, prevMove, info.countermoveBonus, &limits, depth);
    }
    
    // Node explosion diagnostics: Track TT move ordering effectiveness
    if (limits.nodeExplosionDiagnostics && ttMove != NO_MOVE) {
        // Check if TT move is in the move list
        bool ttMoveValid = std::find(moves.begin(), moves.end(), ttMove) != moves.end();
        // Check if it's first
        bool ttMoveFirst = (!moves.empty() && moves[0] == ttMove);
        g_nodeExplosionStats.recordTTMoveFound(ttMoveValid, ttMoveFirst);
    }
    
    // Debug output at root for deeper searches
    if (ply == 0 && depth >= 4) {
        if (!limits.suppressDebugOutput) {
            std::cerr << "Root: generated " << moves.size() << " moves, depth=" << depth << "\n";
        }
    }
    
    
    // Debug: Validate board state before search
#ifdef DEBUG
    Hash hashBefore = board.zobristKey();
    int pieceCountBefore = __builtin_popcountll(board.occupied());
#endif
    
    // Sub-phase 5A-5E: Store preparation
    // Store original alpha for bound determination
    eval::Score alphaOrig = alpha;
    
    // Initialize best score
    eval::Score bestScore = eval::Score::minus_infinity();
    Move bestMove = NO_MOVE;  // Track best move for all plies (needed for TT storage later)
    
    // Search all moves
    int moveCount = 0;
    int legalMoveCount = 0;  // Phase 3.2: Track legal moves for checkmate/stalemate
    
    // Stage 20 Phase B3 Fix: Track quiet moves for butterfly history update
    std::vector<Move> quietMoves;
    quietMoves.reserve(moves.size());
    
    // Phase 2a: Iterate using RankedMovePicker or traditional move list
    Move move;
    size_t moveIndex = 0;
    while (true) {
        bool pendingMoveCountPrune = false;
        int pendingMoveCountLimit = 0;
        int pendingMoveCountDepthBucket = 0;
#ifdef SEARCH_STATS
        int pendingMoveCountRankBucket = -1;
#endif
        int pendingMoveCountValue = moveCount;  // Current legal moves searched before this move
        int pendingRankValue = -1;
        if (rankedPicker) {
            move = rankedPicker->next();
            if (move == NO_MOVE) break;
        } else {
            if (moveIndex >= moves.size()) break;
            move = moves[moveIndex++];
        }

        std::string debugMoveUci;
        bool debugTrackedMove = false;
        auto logTrackedEvent = [&](const std::string& event, const std::string& extra) {
            if (!debugTrackedMove) {
                return;
            }
            std::ostringstream oss;
            oss << "info string DebugMove " << debugMoveUci
                << " depth=" << depth
                << " ply=" << ply
                << " event=" << event;
            if (!extra.empty()) {
                oss << ' ' << extra;
            }
            const int extHere = searchInfo.extensionApplied(ply);
            const int extTotal = searchInfo.totalExtensions(ply);
            oss << " extHere=" << extHere
                << " extTotal=" << extTotal;
            std::cout << oss.str() << std::endl;
        };
        if (!limits.debugTrackedMoves.empty()) {
            debugMoveUci = SafeMoveExecutor::moveToString(move);
            for (const auto& candidate : limits.debugTrackedMoves) {
                if (debugMoveUci == candidate) {
                    debugTrackedMove = true;
                    break;
                }
            }
            if (debugTrackedMove) {
                std::ostringstream extra;
                extra << "moveIndex=" << moveCount << " legal=" << legalMoveCount;
                logTrackedEvent("consider", extra.str());
            }
        }
        // Skip excluded move (for singular extension search)
        if (searchInfo.isExcluded(ply, move)) {
            continue;
        }
        
        // Phase B1: Don't increment moveCount here - wait until we know move is legal
        info.totalMoves++;  // Track total moves examined
        
        // Phase 2: Track current root move for UCI currmove output
        if (ply == 0) {
            info.currentRootMove = move;
            info.currentRootMoveNumber = moveCount;
        }
        
        // Track quiet moves for history update
        if (!isCapture(move) && !isPromotion(move)) {
            quietMoves.push_back(move);
        }
        
        // NOTE: Futility pruning moved to AFTER legality check
        // See implementation after tryMakeMove() succeeds
        
        // Phase 3.1 CONSERVATIVE: Move Count Pruning (Late Move Pruning)
        // Only prune at depths 3+ to avoid tactical blindness at shallow depths
        // Much more conservative limits to avoid over-pruning
        bool parentGaveCheck = (ply > 0) ? searchInfo.moveGaveCheck(ply - 1) : false;

        if (limits.useMoveCountPruning && !isPvNode && !weAreInCheck && !parentGaveCheck
            && depth >= 3 && depth <= limits.moveCountMaxDepth && moveCount > 1
            && !isCapture(move) && !isPromotion(move) && !info.killers->isKiller(ply, move)) {
            
            // Phase 3.3: Countermove Consideration
            // Don't prune countermoves - they're often good responses
            if (prevMove != NO_MOVE && move == info.counterMoves->getCounterMove(prevMove)) {
                // This is a countermove, don't prune it
                // Skip the entire move count pruning block
            } else {
            
            // MODERATELY CONSERVATIVE depth-based move count limits
            // Starting at depth 3 to avoid shallow tactical issues
            // 25% less conservative than previous version for more pruning
            // Now configurable via UCI for SPSA tuning
            int moveCountLimit[9] = {
                999, // depth 0 (not used)
                999, // depth 1 (not used - too shallow)
                999, // depth 2 (not used - too shallow)  
                limits.moveCountLimit3,  // depth 3 - moderately conservative
                limits.moveCountLimit4,  // depth 4 - moderately conservative
                limits.moveCountLimit5,  // depth 5 - moderately conservative
                limits.moveCountLimit6,  // depth 6 - moderately conservative
                limits.moveCountLimit7,  // depth 7 - moderately conservative
                limits.moveCountLimit8   // depth 8 - moderately conservative
            };
            
            // Check if we're improving (compare to previous ply's eval)
            bool improving = false;
            if (ply >= 2) {
                int prevEval = searchInfo.getStackEntry(ply - 2).staticEval;
                int currEval = searchInfo.getStackEntry(ply).staticEval;
                if (prevEval != 0 && currEval != 0) {
                    improving = (currEval > prevEval);
                }
            }
            
            // Adjust limit based on improvement
            int limit = moveCountLimit[std::min(depth, 8)];
            if (!improving) {
                limit = (limit * limits.moveCountImprovingRatio) / 100;  // Configurable reduction ratio
            }
            
            // History-based adjustment (moderately conservative)
            int historyScore = info.history->getScore(board.sideToMove(), moveFrom(move), moveTo(move));
            if (historyScore > limits.moveCountHistoryThreshold) {  // Configurable threshold
                limit += limits.moveCountHistoryBonus;  // Configurable bonus
            }
            
            // Phase 2b.3: LMP rank gating - adjust limit based on move rank
            if (limits.useRankAwareGates && !isPvNode && ply > 0 && !weAreInCheck && depth >= 4 && depth <= 8) {
                // Get rank from picker if available, otherwise use moveCount as fallback
                // Note: Using moveCount before legality check is an approximation
                const int rank = rankedPicker ? rankedPicker->currentYieldIndex() : (moveCount + 1);
                const int K = 5;  // Protected window size
                
                // Phase 2b.7: Check if PVS re-search smoothing should disable rank-based LMP
                bool skipRankLMP = false;
                if (depth >= 4) {  // Smoothing only applies at depth >= 4
                    // Check if smoothing applies for this move
                    bool isTTMove = (move == ttMove);
                    bool isKillerMove = info.killers->isKiller(ply, move);
                    bool isCounterMove = (prevMove != NO_MOVE && 
                                          info.counterMoves->getCounterMove(prevMove) == move);
                    bool isRecapture = (prevMove != NO_MOVE && isCapture(prevMove) && 
                                        moveTo(prevMove) == moveTo(move));
                    bool isCheckOrPromo = isPromotion(move);
                    
                    if (!isTTMove && !isKillerMove && !isCounterMove && !isRecapture && !isCheckOrPromo) {
                        int depthBucket = SearchData::PVSReSearchSmoothing::depthBucket(depth);
                        int rankBucket = SearchData::PVSReSearchSmoothing::rankBucket(rank);
                        if (info.pvsReSearchSmoothing.shouldApplySmoothing(depthBucket, rankBucket)) {
                            // Smoothing: disable rank-based early pruning for this move
                            skipRankLMP = true;
                        }
                    }
                }
                
                if (!skipRankLMP) {
                    if (rank == 1) {
                        // Rank 1: disable LMP for this move (make limit very high)
                        limit = 999;
                    } else if (rank >= 2 && rank <= K) {
                        // Ranks 2-5: make prune less aggressive
                        limit += 2;
                    } else if (rank >= 6 && rank <= 10) {
                        // Ranks 6-10: leave limit unchanged
                        // (no adjustment needed)
                    } else if (rank >= 11) {
                        // Ranks 11+: make prune more aggressive
                        limit = std::max(1, limit - 4);
                    }
                }
                // If skipRankLMP is true, use baseline LMP for depth (limit stays unchanged)
            }
            
            if (moveCount > limit) {
                pendingMoveCountPrune = true;
                pendingMoveCountLimit = limit;
                pendingMoveCountDepthBucket = SearchData::PruneBreakdown::bucketForDepth(depth);
#ifdef SEARCH_STATS
                if (limits.useRankAwareGates && ply > 0) {
                    const int rank = rankedPicker ? rankedPicker->currentYieldIndex() : (moveCount + 1);
                    pendingMoveCountRankBucket = SearchData::RankGateStats::bucketForRank(rank);
                    pendingRankValue = rank;
                }
#endif
            }
            } // End of else block for countermove check
        }
        
        // Singular Extension: DISABLED - Implementation needs redesign
        // Extension calculation moved earlier for effective-depth futility
        // int extension = 0;  // Now calculated before futility pruning
        
        // TODO: Reimplement singular extensions properly:
        // 1. Loop through all moves except TT move
        // 2. Search each at reduced depth with narrow window
        // 3. If all fail low, extend the TT move
        // See Laser engine for reference implementation
        
        #if 0  // DISABLED - needs proper implementation
        if (depth >= 7 && move == ttMove && ttBound == Bound::LOWER && 
            ttDepth >= depth - 3 && ply > 0 && !isPvNode &&
            std::abs(ttScore.value()) < MATE_BOUND - MAX_PLY) {
            // Implementation removed - was not working correctly
        }
        #endif
        
        // Track QxP and RxP attempts before making the move
        if (isCapture(move) && !isPromotion(move)) {
            Square fromSq = moveFrom(move);
            Square toSq = moveTo(move);
            Piece attacker = board.pieceAt(fromSq);
            Piece victim = board.pieceAt(toSq);
            PieceType attackerType = typeOf(attacker);
            PieceType victimType = typeOf(victim);
            
            if (victimType == PAWN) {
                if (attackerType == QUEEN) {
                    info.moveOrderingStats.qxpAttempts++;
                } else if (attackerType == ROOK) {
                    info.moveOrderingStats.rxpAttempts++;
                }
            }
        }
        
        // Track game phase
        int pieceCount = __builtin_popcountll(board.occupied());
        if (pieceCount > 28) {
            info.moveOrderingStats.openingNodes++;
        } else if (pieceCount >= 16) {
            info.moveOrderingStats.middlegameNodes++;
        } else {
            info.moveOrderingStats.endgameNodes++;
        }
        
        // Phase 2b.5: Capture SEE gating by rank
        if (limits.useRankAwareGates && !isPvNode && ply > 0 && !weAreInCheck && depth >= 4
            && isCapture(move) && !isPromotion(move)) {
            
            // Check exemptions: TT move, killers, countermoves, recaptures
            bool isTTMove = (move == ttMove);
            bool isKillerMove = info.killers->isKiller(ply, move);
            bool isCounterMove = (prevMove != NO_MOVE && 
                                  info.counterMoves->getCounterMove(prevMove) == move);
            bool isRecapture = (prevMove != NO_MOVE && isCapture(prevMove) && 
                                moveTo(prevMove) == moveTo(move));
            
            if (!isTTMove && !isKillerMove && !isCounterMove && !isRecapture) {
                // Get rank from picker if available, otherwise use moveCount+1 (pre-legality)
                const int rank = rankedPicker ? rankedPicker->currentYieldIndex() : (moveCount + 1);
                const int K = 5;  // Protected window size
                
                // Only gate late-ranked captures (rank >= 11)
                if (rank >= 11) {
                    // Require non-losing SEE for late captures
                    if (!seeGE(board, move, 0)) {
                        // Track telemetry
#ifdef SEARCH_STATS
                        const int bucket = SearchData::RankGateStats::bucketForRank(rank);
                        info.rankGates.pruned[bucket]++;
#endif
                        if (debugTrackedMove) {
                            std::ostringstream extra;
                            extra << "rank=" << rank << " reason=see";
                            logTrackedEvent("prune_capture_rank", extra.str());
                        }
                        continue;  // Skip this capture
                    }
                }
                // Ranks 1-10: no SEE gate (conservative)
            }
        }
        
        bool futilitySeeGuard = false;
        if (!isCapture(move) && !isPromotion(move) && config.futilitySeeMargin > 0
            && depth <= config.futilityMaxDepth + 1) {
            futilitySeeGuard = seeGE(board, move, -config.futilitySeeMargin);
        }

        // Phase 3.2: Try to make the move with lazy legality checking
        Board::UndoInfo undo;
        if (!board.tryMakeMove(move, undo)) {
            // B0: Count illegal pseudo-legal moves
            if (legalMoveCount == 0) {
                info.illegalPseudoBeforeFirst++;
            }
            info.illegalPseudoTotal++;
            if (debugTrackedMove) {
                logTrackedEvent("illegal", "reason=king_in_check");
            }
            // Move is illegal (leaves king in check) - skip it
            continue;
        }

        bool givesCheckMove = inCheck(board);
        bool wasCaptureMove = (undo.capturedPiece != NO_PIECE);

        if (pendingMoveCountPrune && !givesCheckMove && !wasCaptureMove) {
            info.moveCountPruned++;
            info.pruneBreakdown.moveCount[pendingMoveCountDepthBucket]++;

#ifdef SEARCH_STATS
            if (pendingMoveCountRankBucket >= 0) {
                info.rankGates.pruned[pendingMoveCountRankBucket]++;
            }
#endif
            if (limits.nodeExplosionDiagnostics) {
                g_nodeExplosionStats.recordMoveCountPrune(depth, pendingMoveCountValue);
            }
            if (debugTrackedMove) {
                std::ostringstream extra;
                extra << "limit=" << pendingMoveCountLimit << " moveCount=" << pendingMoveCountValue
                      << " rank=" << pendingRankValue
                      << " capture=" << (wasCaptureMove ? 1 : 0);
                logTrackedEvent("prune_move_count", extra.str());
            }
            board.unmakeMove(move, undo);
            continue;
        }

        // Move is legal and not pruned by move-count pruning
        legalMoveCount++;
        moveCount++;  // Phase B1: Increment moveCount only after confirming legality
        
        // Phase 2b.1: Rank capture and telemetry (no behavior change)
#ifdef SEARCH_STATS
        if (limits.useRankAwareGates && ply > 0) {  // Skip at root, same as RankedMovePicker
            // Get current move rank (1-based index from picker, or moveCount as fallback)
            const int rank = rankedPicker ? rankedPicker->currentYieldIndex() : moveCount;
            const int bucket = SearchData::RankGateStats::bucketForRank(rank);
            info.rankGates.tried[bucket]++;
        }
#endif
        
        // Push position to search stack after we know move is legal
        searchInfo.pushSearchPosition(board.zobristKey(), move, ply);

        searchInfo.setGaveCheck(ply, givesCheckMove);

        // Calculate extension for this move (currently just check extension)
        int extension = 0;
        // Check extension could be added here if needed
        
        // Phase 1: Effective-Depth Futility Pruning (AFTER legality, BEFORE child search)
        // Following notes: Apply after confirming legality but before recursion
        bool canPruneFutility = (depth <= 6) ? (legalMoveCount > 1) : (legalMoveCount >= 1);
        
        // Check if this is a special move that shouldn't be pruned
        bool isKillerMove = info.killers->isKiller(ply, move);
        bool isCounterMove = (prevMove != NO_MOVE &&
                              info.counterMoves->getCounterMove(prevMove) == move);
        // Determine whether the CHILD would be a PV node: at a PV parent, only the first legal move is PV
        bool childIsPV = (isPvNode && legalMoveCount == 1);
        
        if (config.useFutilityPruning && !childIsPV && depth > 0 && !weAreInCheck
            && canPruneFutility && !isCapture(move) && !isPromotion(move)
            && staticEvalComputed && move != ttMove 
            && !isKillerMove && !isCounterMove && !futilitySeeGuard) {
            
            // Calculate what reduction this move would get from LMR
            int reduction = 0;
            if (ply > 0 && info.lmrParams.enabled && depth >= info.lmrParams.minDepth && legalMoveCount > 1) {
                bool captureMove = false;  // Already checked it's not a capture
                bool givesCheck = false;  // Actual check handling below via clamp
                bool pvNode = isPvNode;

                if (shouldReduceMove(move, depth, moveCount, captureMove,
                                    weAreInCheck, givesCheck, pvNode,
                                    *info.killers, *info.history,
                                    *info.counterMoves, prevMove,
                                    ply, board.sideToMove(),
                                    info.lmrParams)) {
                    bool improving = false;
                    reduction = getLMRReduction(depth, moveCount, info.lmrParams, pvNode, improving);

                    if (givesCheckMove && reduction > 0) {
                        const int rankForClamp = rankedPicker ? rankedPicker->currentYieldIndex() : moveCount;
                        const int clampLimit = (depth <= 6 && rankForClamp <= 10) ? 0 : 1;
                        reduction = std::min(reduction, clampLimit);
                    }
                }
            }
            
            // Calculate effective depth: eff = depth - 1 - reduction + extension
            int effectiveDepth = depth - 1 - reduction + extension;
            
            // Apply futility at effective depth
            if (effectiveDepth <= config.futilityMaxDepth) {
                // Handle leaf-like positions (eff <= 0)
                if (effectiveDepth <= 0) {
                    // Very shallow effective depth - use minimal margin
                    if (staticEval <= alpha - eval::Score(100)) {
                        board.unmakeMove(move, undo);
                        info.futilityPruned++;
                        continue;
                    }
                } else {
                    // Normal futility with progressive margins
                    int futilityMargin;
                    if (effectiveDepth <= 4) {
                        futilityMargin = config.futilityBase * effectiveDepth;
                    } else {
                        // Cap growth after 4 plies as requested
                        futilityMargin = config.futilityBase * 4 + (effectiveDepth - 4) * (config.futilityBase / 2);
                    }
                    
                    // Phase 2b.4: Futility margin scaling by rank
                    if (limits.useRankAwareGates && !isPvNode && ply > 0 && !weAreInCheck && depth >= 3) {
                        // Get rank from picker if available, otherwise use legalMoveCount
                        // Note: Using legalMoveCount after legality check is accurate
                        const int rank = rankedPicker ? rankedPicker->currentYieldIndex() : legalMoveCount;
                        const int K = 5;  // Protected window size
                        
                        if (rank >= 1 && rank <= K) {
                            // Ranks 1-5: no change to margin (protect top moves)
                            // (no adjustment needed)
                        } else if (rank >= 6 && rank <= 10) {
                            // Ranks 6-10: modest bump to margin
                            futilityMargin += config.futilityBase / 2;
                        } else if (rank >= 11) {
                            // Ranks 11+: bigger (but still modest) bump
                            futilityMargin += config.futilityBase;
                        }
                        
                        // Optional cap to prevent excessive margins
                        futilityMargin = std::min(futilityMargin, config.futilityBase * (effectiveDepth + 1));
                    }
                    
                    if (staticEval <= alpha - eval::Score(futilityMargin)) {
                        board.unmakeMove(move, undo);
                        info.futilityPruned++;
                        // Track by effective depth for telemetry
                        int b = SearchData::PruneBreakdown::bucketForDepth(effectiveDepth);
                        info.pruneBreakdown.futilityEff[b]++;
                        
                        // Phase 2b.4: Track rank-aware futility pruning telemetry
#ifdef SEARCH_STATS
                        if (limits.useRankAwareGates && ply > 0) {
                            const int rank = rankedPicker ? rankedPicker->currentYieldIndex() : legalMoveCount;
                            const int bucket = SearchData::RankGateStats::bucketForRank(rank);
                            info.rankGates.pruned[bucket]++;
                        }
#endif
                        
                        // Node explosion diagnostics: Track futility pruning
                        if (limits.nodeExplosionDiagnostics) {
                            g_nodeExplosionStats.recordFutilityPrune(effectiveDepth, futilityMargin);
                        }
                        continue;
                    }
                }
            }
        }
        
        // Phase PV3: Acquire child PV storage from arena when needed
        TriangularPV* childPVPtr = nullptr;
        if (pv != nullptr && isPvNode) {
            childPVPtr = info.acquireChildPV(ply);
        }
        
        // Phase P3: Principal Variation Search (PVS) with LMR integration
        eval::Score score;

        // Track beta cutoff position for move ordering analysis
        bool isCutoffMove = false;

        // Record extension metadata for the child node before searching it
        searchInfo.setExtensionApplied(ply + 1, extension);
        if (debugTrackedMove && extension != 0) {
            std::ostringstream extra;
            extra << "value=" << extension
                  << " total=" << searchInfo.totalExtensions(ply + 1);
            logTrackedEvent("extension_apply", extra.str());
        }

        // Phase 2b.7: Variables for PVS re-search smoothing (declared outside if/else)
        int moveRank = 0;
        bool didReSearch = false;
        int appliedReduction = 0;
        
        // Phase B1: Use legalMoveCount to determine if this is the first LEGAL move
        if (legalMoveCount == 1) {
            // First move: search with full window as PV node (apply extension if any)
            // Pass childPV only if we're in a PV node and have a parent PV
            TriangularPV* firstMoveChildPV = (pv != nullptr && isPvNode) ? childPVPtr : nullptr;
            score = -negamax(board, depth - 1 + extension, ply + 1,
                            -beta, -alpha, searchInfo, info, limits, tt,
                            firstMoveChildPV,  // Phase PV3: Pass child PV for PV nodes
                            isPvNode);  // Phase P3: First move inherits PV status
        } else {
            // Later moves: use PVS with LMR
            
            // Phase 3: Late Move Reductions (LMR)
            int reduction = 0;
            
            // Phase 2b.7: PVS re-search smoothing - compute early for both LMR and LMP
            bool applySmoothing = false;
            // moveRank already declared outside the if/else
            if (limits.useRankAwareGates && !isPvNode && depth >= 4) {
                // Get current move rank (1-based from picker, or moveCount as fallback)
                moveRank = rankedPicker ? rankedPicker->currentYieldIndex() : moveCount;
                
                // Check if smoothing should apply (only for non-exempt moves)
                bool isTTMove = (move == ttMove);
                bool isKillerMove = info.killers->isKiller(ply, move);
                bool isCounterMove = (prevMove != NO_MOVE && 
                                      info.counterMoves->getCounterMove(prevMove) == move);
                bool isRecapture = (prevMove != NO_MOVE && isCapture(prevMove) && 
                                    moveTo(prevMove) == moveTo(move));
                bool isCheckOrPromo = isPromotion(move);  // We already know !weAreInCheck
                
                if (!isTTMove && !isKillerMove && !isCounterMove && !isRecapture && !isCheckOrPromo) {
                    int depthBucket = SearchData::PVSReSearchSmoothing::depthBucket(depth);
                    int rankBucket = SearchData::PVSReSearchSmoothing::rankBucket(moveRank);
                    applySmoothing = info.pvsReSearchSmoothing.shouldApplySmoothing(depthBucket, rankBucket);
                    
#ifdef SEARCH_STATS
                    if (applySmoothing) {
                        info.pvsReSearchSmoothing.recordSmoothingApplied(depth, moveRank);
                    }
#endif
                }
            }
            
            // Calculate LMR reduction (don't reduce at root)
            if (ply > 0 && info.lmrParams.enabled && depth >= info.lmrParams.minDepth) {
                // Determine move properties for LMR
                bool captureMove = isCapture(move);
                bool givesCheck = false;  // Actual check handling below via clamp
                bool pvNode = isPvNode;   // Phase P3: Use actual PV status
                
                // Check if we should reduce this move with improved conditions
                if (shouldReduceMove(move, depth, moveCount, captureMove, 
                                    weAreInCheck, givesCheck, pvNode, 
                                    *info.killers, *info.history, 
                                    *info.counterMoves, prevMove,
                                    ply, board.sideToMove(),
                                    info.lmrParams)) {
                    // Calculate reduction amount
                    // For now, assume not improving to be conservative
                    // (future enhancement: track eval history for proper improving detection)
                    bool improving = false; // Conservative: assume not improving
                    
                    reduction = getLMRReduction(depth, moveCount, info.lmrParams, pvNode, improving);

                    if (givesCheckMove && reduction > 0) {
                        const int rankForClamp = rankedPicker ? rankedPicker->currentYieldIndex() : moveCount;
                        const int clampLimit = (depth <= 6 && rankForClamp <= 10) ? 0 : 1;
                        const int originalReduction = reduction;
                        reduction = std::min(reduction, clampLimit);
                        if (debugTrackedMove && reduction != originalReduction) {
                            std::ostringstream extra;
                            extra << "rank=" << rankForClamp
                                  << " depth=" << depth
                                  << " clamp=" << clampLimit
                                  << " original=" << originalReduction;
                            logTrackedEvent("lmr_check_clamp", extra.str());
                        }
                    }

                    appliedReduction = reduction;
                    
                    // Phase 2b.2: LMR scaling by rank (conservative, non-PV, depth≥4)
                    // SPRT fix: Add !weAreInCheck guard to avoid reducing evasions
                    if (limits.useRankAwareGates && !isPvNode && !weAreInCheck && depth >= 4 
                        && !isCapture(move) && !isPromotion(move)
                        && move != ttMove
                        && !info.killers->isKiller(ply, move)
                        && !(prevMove != NO_MOVE && isCapture(prevMove) && moveTo(prevMove) == moveTo(move))  // Not a recapture
                        && !(prevMove != NO_MOVE && info.counterMoves && info.counterMoves->getCounterMove(prevMove) == move))  // Not a countermove
                    {
                        // Get current move rank (1-based index from picker, or moveCount as fallback)
                        // Phase 2b.2-fix: currentYieldIndex() now always available for accurate rank
                        const int rank = rankedPicker ? rankedPicker->currentYieldIndex() : moveCount;
                        const int K = 5;  // Protected rank threshold
                        
                        // Apply rank-based scaling (CONSERVATIVE after SPRT fail)
                        int originalReduction = reduction;
                        if (rank == 1) {
                            // Rank 1: clamp reduction to 0 (no reduction for best move)
                            reduction = 0;
                        } else if (rank <= K) {
                            // Ranks 2-K: clamp reduction to at most 1
                            reduction = std::min(reduction, 1);
                        }
                        // SPRT fix: Remove the +1 tier entirely for now (too aggressive at shallow depths)
                        // Later phases can re-enable with stricter depth/history guards
                        // else if (rank <= 10) {
                        //     // Ranks 6-10: leave base reduction unchanged
                        // } else {
                        //     // Ranks 11+: DISABLED - was causing over-reduction
                        //     // Only re-enable with depth >= 8 and low history checks
                        // }
                        
#ifdef SEARCH_STATS
                        // Track telemetry if we modified the reduction
                        if (reduction != originalReduction) {
                            const int bucket = SearchData::RankGateStats::bucketForRank(rank);
                            info.rankGates.reduced[bucket]++;
                        }
#endif
                        if (debugTrackedMove && reduction != originalReduction) {
                            std::ostringstream extra;
                            extra << "rank=" << rank
                                  << " original=" << originalReduction
                                  << " adjusted=" << reduction;
                            logTrackedEvent("lmr_scaled", extra.str());
                        }
                        appliedReduction = reduction;
                    }

                    // Phase 2b.7: Apply PVS re-search smoothing to LMR
                    if (applySmoothing && reduction > 0) {
                        // Subtract 1 from any extra reduction added by rank bucket
                        // Do not go below baseline reduction (i.e., the non-rank-aware reduction)
                        int baseReduction = getLMRReduction(depth, moveCount, info.lmrParams, isPvNode, false);
                        reduction = std::max(baseReduction - 1, reduction - 1);
                        appliedReduction = reduction;
                        if (debugTrackedMove) {
                            std::ostringstream extra;
                            extra << "smoothed=" << reduction;
                            logTrackedEvent("lmr_smoothed", extra.str());
                        }
                    }
                    
                    // Track LMR statistics
                    info.lmrStats.totalReductions++;
                    // Node explosion diagnostics: Track LMR application
                    if (limits.nodeExplosionDiagnostics) {
                        g_nodeExplosionStats.recordLMRReduction(depth, reduction);
                    }
                }
            }
            
            // Scout search (possibly reduced, with extension)
            info.pvsStats.scoutSearches++;
            score = -negamax(board, depth - 1 - reduction + extension, ply + 1,
                            -(alpha + eval::Score(1)), -alpha, searchInfo, info, limits, tt,
                            nullptr,  // Phase PV3: Scout searches don't need PV
                            false);  // Scout searches are not PV
            
            // Phase 2b.7: Track re-search for smoothing (only for non-PV nodes)
            // didReSearch already declared outside the if/else
            
        // If reduced scout fails high, re-search without reduction
        if (score > alpha && reduction > 0) {
            info.lmrStats.reSearches++;
            // Node explosion diagnostics: Track LMR re-search
            if (limits.nodeExplosionDiagnostics) {
                g_nodeExplosionStats.recordLMRReSearch(depth);
            }
            score = -negamax(board, depth - 1 + extension, ply + 1,
                            -(alpha + eval::Score(1)), -alpha, searchInfo, info, limits, tt,
                            nullptr,  // Phase PV3: Still a scout search
                            false);  // Still a scout search
        }
            
            // If scout search fails high, do full window re-search
            if (score > alpha) {
                info.pvsStats.reSearches++;
                auto histCtx = info.historyContextAt(ply);
                if (histCtx == SearchData::HistoryContext::Basic) {
                    info.historyStats.basicReSearches++;
                } else if (histCtx == SearchData::HistoryContext::Counter) {
                    info.historyStats.counterReSearches++;
                }
                didReSearch = true;  // Phase 2b.7: Mark that we did a re-search
                // B1 Fix: Only the first legal move should be a PV node
                // Re-searches after scout failures are NOT PV nodes
                TriangularPV* reSearchChildPV = (pv != nullptr && isPvNode) ? childPVPtr : nullptr;
                score = -negamax(board, depth - 1 + extension, ply + 1,
                                -beta, -alpha, searchInfo, info, limits, tt,
                                reSearchChildPV,  // Phase PV3: Re-search needs PV for PV nodes
                                false);  // B1 Fix: Re-search is NOT a PV node!
            } else if (reduction > 0 && score <= alpha) {
                // Reduction was successful (move was bad as expected)
                info.lmrStats.successfulReductions++;
            }
            appliedReduction = reduction;
            if (debugTrackedMove && reduction > 0) {
                std::ostringstream extra;
                extra << "value=" << reduction
                      << " reSearch=" << (didReSearch ? 1 : 0);
                logTrackedEvent("lmr_applied", extra.str());
            }
        }

        int childStaticEval = 0;
        if (ply + 1 < seajay::MAX_PLY) {
            childStaticEval = searchInfo.getStackEntry(ply + 1).staticEval;
        }

        if (debugTrackedMove) {
            eval::EvalTrace trace;
            eval::Score evalScore = eval::evaluateWithTrace(board, trace);
            eval::Score pawnTotal = trace.passedPawns + trace.isolatedPawns + trace.doubledPawns +
                                   trace.backwardPawns + trace.pawnIslands;
            std::ostringstream extra;
            extra << "eval=" << evalScore.to_cp()
                  << " static=" << childStaticEval
                  << " mat=" << trace.material.to_cp()
                  << " pst=" << trace.pst.to_cp()
                  << " pawns=" << pawnTotal.to_cp()
                  << " king=" << trace.kingSafety.to_cp()
                  << " mob=" << trace.mobility.to_cp();
            logTrackedEvent("eval_trace", extra.str());
        }

        // Unmake the move
        board.unmakeMove(move, undo);

        // Phase 2b.7: Record PVS re-search statistics for smoothing
        // Only record for non-PV nodes that were searched (not pruned)
        if (limits.useRankAwareGates && !isPvNode && depth >= 4 && legalMoveCount > 1) {
            // Only record if we have moveRank computed (from earlier smoothing check)
            if (moveRank > 0) {
                info.pvsReSearchSmoothing.recordMove(depth, moveRank, didReSearch);
            }
        }
        
        // Debug: Validate board state is properly restored
#ifdef DEBUG
        assert(board.zobristKey() == hashBefore);
        assert(__builtin_popcountll(board.occupied()) == pieceCountBefore);
#endif
        
        // Check if search was interrupted
        if (info.stopped) {
            return bestScore;
        }

        if (debugTrackedMove) {
            std::ostringstream extra;
            extra << "score=" << score.to_cp()
                  << " alpha=" << alpha.to_cp()
                  << " beta=" << beta.to_cp()
                  << " reduction=" << appliedReduction
                  << " static=" << childStaticEval
                  << " legalIndex=" << legalMoveCount;
            logTrackedEvent("score", extra.str());
        }

        // Update best score and move
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            
            // Phase PV3: Update PV at all depths
            if (pv != nullptr && isPvNode) {
                // Update PV with best move and child's PV
                // childPV should have been populated by the successful search
                if (childPVPtr != nullptr) {
                    pv->updatePV(ply, move, childPVPtr);
                }
            }
            
            // At root, store the best move in SearchInfo
            if (ply == 0) {
                info.bestMove = move;
                info.bestScore = score;
                
                // Debug output for root moves (removed in release builds)
                #ifndef NDEBUG
                if (depth >= 3) {
                    std::cerr << "Root: Move " << SafeMoveExecutor::moveToString(move) 
                              << " score=" << score.to_cp() << " cp" << std::endl;
                }
                #endif
            }
            
            // Update alpha (best score we can guarantee)
            if (score > alpha) {
                alpha = score;
                
                // Beta cutoff - prune remaining moves
                // This is a fail-high, meaning the opponent won't choose this position
                // Return bestScore for fail-soft alpha-beta
                if (score >= beta) [[unlikely]] {
                    info.betaCutoffs++;  // Track beta cutoffs
                    auto histCtx = info.historyContextAt(ply);
                    if (moveCount == 1) {
                        info.betaCutoffsFirst++;  // Track first-move cutoffs
                        if (histCtx == SearchData::HistoryContext::Basic) {
                            info.historyStats.basicFirstMoveHits++;
                        } else if (histCtx == SearchData::HistoryContext::Counter) {
                            info.historyStats.counterFirstMoveHits++;
                        }
                    }

                    const bool isCaptureMv = isCapture(move) || isPromotion(move) || isEnPassant(move);
                    const bool isTTMove = (move == ttMove && ttMove != NO_MOVE);
                    bool isKillerMove = false;
                    if (!isCaptureMv && info.killers) {
                        for (int slot = 0; slot < 2; ++slot) {
                            if (move == info.killers->getKiller(ply, slot)) {
                                isKillerMove = true;
                                break;
                            }
                        }
                    }
                    bool isCounterMove = false;
                    if (info.counterMoves && prevMove != NO_MOVE) {
                        if (move == info.counterMoves->getCounterMove(prevMove)) {
                            isCounterMove = true;
                        }
                    }
                    if (isCounterMove && info.countermoveBonus > 0) {
                        info.counterMoveStats.hits++;
                        info.counterMoveStats.cutoffs++;
                    }
                    if (histCtx == SearchData::HistoryContext::Counter && isCounterMove) {
                        info.historyStats.counterCutoffs++;
                    } else if (histCtx == SearchData::HistoryContext::Basic &&
                               !isCaptureMv && !isTTMove && !isKillerMove && !isCounterMove) {
                        info.historyStats.basicCutoffs++;
                    }
                    
#ifdef SEARCH_STATS
                    // Phase 2a.6c: Track cutoff move rank and shortlist coverage
                    // Gate by UCI toggle to avoid any overhead when disabled
                    if (rankedPicker && limits.useRankedMovePicker && limits.showMovePickerStats) {
                        int yieldIndex = rankedPicker->currentYieldIndex();
                        
                        // Bucket the yield rank: [1], [2-5], [6-10], [11+]
                        if (yieldIndex == 1) {
                            info.movePickerStats.bestMoveRank[0]++;
                        } else if (yieldIndex >= 2 && yieldIndex <= 5) {
                            info.movePickerStats.bestMoveRank[1]++;
                        } else if (yieldIndex >= 6 && yieldIndex <= 10) {
                            info.movePickerStats.bestMoveRank[2]++;
                        } else {
                            info.movePickerStats.bestMoveRank[3]++;
                        }
                        
                        // Check if the cutoff move was in the shortlist
                        if (rankedPicker->wasInShortlist(move)) {
                            info.movePickerStats.shortlistHits++;
                        }
                    }
#endif
                    if (debugTrackedMove) {
                        std::ostringstream extra;
                        extra << "score=" << score.to_cp() << " beta=" << beta.to_cp();
                        logTrackedEvent("cutoff", extra.str());
                    }
                    
                    // Node explosion diagnostics: Track beta cutoff position and move type
                    if (limits.nodeExplosionDiagnostics) {
                        g_nodeExplosionStats.recordBetaCutoff(ply, legalMoveCount - 1, isTTMove, isKillerMove, isCaptureMv);
                        if (legalMoveCount > 10) {
                            g_nodeExplosionStats.recordLateCutoff(ply, legalMoveCount - 1);
                        }
                    }
                    
                    // Diagnostic: Track cutoff position distribution
                    if (legalMoveCount <= 3) {
                        info.cutoffsByPosition[legalMoveCount - 1]++;
                    } else if (legalMoveCount <= 10) {
                        info.cutoffsByPosition[3]++;  // Moves 4-10
                    } else {
                        info.cutoffsByPosition[4]++;  // Moves 11+
                    }
                    
                    // Track detailed move ordering statistics
                    auto& stats = info.moveOrderingStats;
                    
                    // Track which type of move caused cutoff
                    if (isTTMove) {
                        stats.ttMoveCutoffs++;
                    } else if (isCaptureMv) {
                        if (moveCount == 1) {
                            stats.firstCaptureCutoffs++;
                        }
                        // Check if it's a bad capture (QxP or RxP)
                        Square fromSq = moveFrom(move);
                        Square toSq = moveTo(move);
                        Piece attacker = board.pieceAt(fromSq);
                        Piece victim = board.pieceAt(toSq);
                        PieceType attackerType = typeOf(attacker);
                        PieceType victimType = typeOf(victim);
                        
                        if (victimType == PAWN && (attackerType == QUEEN || attackerType == ROOK)) {
                            stats.badCaptureCutoffs++;
                            if (attackerType == QUEEN) {
                                stats.qxpCutoffs++;
                            } else {
                                stats.rxpCutoffs++;
                            }
                        }
                    } else if (isKillerMove) {
                        stats.killerCutoffs++;
                    } else if (isCounterMove && info.countermoveBonus > 0) {
                        stats.counterMoveCutoffs++;
                    } else {
                        stats.quietCutoffs++;
                    }
                    
                    // Track cutoff by move index
                    int moveIndex = moveCount - 1;
                    if (moveIndex < 10) {
                        stats.cutoffsAtMove[moveIndex]++;
                    } else {
                        stats.cutoffsAfter10++;
                    }
                    
                    // Stage 19, Phase A3: Update killer moves for quiet moves that cause cutoffs
                    // Stage 20, Phase B3: Update history for quiet moves that cause cutoffs
                    // Stage 23, Phase CM2: Update countermoves (shadow mode - no ordering yet)
                    if (!isCapture(move) && !isPromotion(move)) {
                        info.killers->update(ply, move);
                        
                        // Update history with depth-based bonus for cutoff move
                        Color side = board.sideToMove();
                        info.history->update(side, moveFrom(move), moveTo(move), depth);
                        
                        // Butterfly update: penalize quiet moves tried before the cutoff
                        for (const Move& quietMove : quietMoves) {
                            if (quietMove != move) {  // Don't penalize the cutoff move itself
                                info.history->updateFailed(side, moveFrom(quietMove), moveTo(quietMove), depth);
                            }
                        }
                        
                        // Stage 23, Phase CM2: Update countermove table
                        // Shadow mode: update the table but don't use for ordering yet
                        if (ply > 0) {
                            Move prevMove = searchInfo.getStackEntry(ply - 1).move;
                            if (prevMove != NO_MOVE) {
                                info.counterMoves->update(prevMove, move);
                                info.counterMoveStats.updates++;  // Track shadow mode updates
                                
                                // Phase 4.3.a: Update counter-move history (simplified interface)
                                if (info.counterMoveHistory) {
                                    info.counterMoveHistory->update(prevMove, move, depth);
                                    
                                    // Penalize quiet moves that were tried but didn't cause cutoff
                                    for (const Move& quietMove : quietMoves) {
                                        if (quietMove != move) {  // Don't penalize the cutoff move itself
                                            info.counterMoveHistory->updateFailed(prevMove, quietMove, depth);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    break;  // Beta cutoff - no need to search more moves
                }
            }

            if (debugTrackedMove) {
                std::ostringstream extra;
                extra << "bestScore=" << bestScore.to_cp();
                if (ply == 0) {
                    extra << " root=1";
                }
                logTrackedEvent("best_update", extra.str());
            }
        }
    }  // End of move iteration loop
    
    // Phase 3.2: Check for checkmate/stalemate after trying all moves
    if (legalMoveCount == 0) {
        if (weAreInCheck) {
            // Checkmate - return mate score
            return eval::Score(-32000 + ply);
        } else {
            // Stalemate - return draw score
            return eval::Score::draw();
        }
    }
    
    // Sub-phase 5A: Basic Store Implementation
    // Store position in TT after search completes
    // Only store if we have a valid TT and didn't terminate early
    if (tt && tt->isEnabled() && !info.stopped && bestMove != NO_MOVE) {
            Hash zobristKey = board.zobristKey();
            
            // Sub-phase 5B: Determine bound type based on score relative to original window
            Bound bound;
            if (bestScore <= alphaOrig) {
                // Fail-low: All moves were worse than alpha
                bound = Bound::UPPER;
            } else if (bestScore >= beta) {
                // Fail-high: We found a move better than beta (beta cutoff)
                bound = Bound::LOWER;
            } else {
                // Exact: Score is within the original window
                bound = Bound::EXACT;
            }
            
            // Sub-phase 5C: Mate Score Adjustment for storing
            // Adjust mate scores to be relative to root (inverse of adjustMateScoreFromTT)
            eval::Score scoreToStore = bestScore;
            if (bestScore.value() >= MATE_BOUND) {
                // Positive mate score - we're winning
                // Store distance from root, not from current position
                scoreToStore = eval::Score(bestScore.value() + ply);
            } else if (bestScore.value() <= -MATE_BOUND) {
                // Negative mate score - we're losing
                // Store distance from root, not from current position
                scoreToStore = eval::Score(bestScore.value() - ply);
            }
            
            // Store the entry
            // Phase 4.2.c: Store the TRUE static eval, not the search score
            // This allows better eval reuse and improving position detection
            int16_t evalToStore = staticEvalComputed ? staticEval.value() : TT_EVAL_NONE;
            tt->store(zobristKey, bestMove, scoreToStore.value(), evalToStore, 
                     static_cast<uint8_t>(depth), bound);
            info.ttStores++;
    }

    // Root safety net: ensure we always have a move to play.
    // In rare edge cases (e.g., tightened TT window + pruning), bestMove may remain NO_MOVE.
    if (ply == 0 && bestMove == NO_MOVE) {
        MoveList legalRoot;
        MoveGenerator::generateLegalMoves(board, legalRoot);
        if (!legalRoot.empty()) {
            bestMove = legalRoot[0];
            info.bestMove = bestMove;
            // bestScore may be uninformative; keep as-is (fail-soft). UCI will still have a move.
        }
    }

    return bestScore;
}
