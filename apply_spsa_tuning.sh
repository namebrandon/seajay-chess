#!/bin/bash
# Apply SPSA-tuned parameters to SeaJay source code
# This updates the default values in SearchLimits

echo "Applying SPSA-tuned parameters to SeaJay..."
echo ""

# Create a patch file with the changes
cat << 'EOF' > spsa_params.patch
--- a/src/search/types.h
+++ b/src/search/types.h
@@ -93,8 +93,8 @@ struct SearchLimits {
     
     // Futility Pruning parameters
     bool useFutilityPruning = true;     // Enable/disable futility pruning
-    int futilityMargin1 = 100;          // Futility margin for depth 1 (in centipawns)
-    int futilityMargin2 = 175;          // Futility margin for depth 2 (in centipawns)
+    int futilityMargin1 = 240;          // SPSA-tuned: Was 100, now using FutilityBase
+    int futilityMargin2 = 313;          // SPSA-tuned: Was 175, now FutilityBase + FutilityScale
     int futilityMargin3 = 250;          // Futility margin for depth 3 (in centipawns)
     int futilityMargin4 = 325;          // Futility margin for depth 4 (in centipawns)
     
@@ -114,16 +114,16 @@ struct SearchLimits {
     
     // Phase 3: Move Count Pruning parameters (conservative implementation)
     bool useMoveCountPruning = true;    // Enable/disable move count pruning
-    int moveCountLimit3 = 12;           // Move limit for depth 3
-    int moveCountLimit4 = 18;           // Move limit for depth 4
-    int moveCountLimit5 = 24;           // Move limit for depth 5
-    int moveCountLimit6 = 30;           // Move limit for depth 6
+    int moveCountLimit3 = 7;            // SPSA-tuned: Was 12
+    int moveCountLimit4 = 15;           // SPSA-tuned: Was 18
+    int moveCountLimit5 = 20;           // SPSA-tuned: Was 24
+    int moveCountLimit6 = 25;           // SPSA-tuned: Was 30
     int moveCountLimit7 = 36;           // Move limit for depth 7
     int moveCountLimit8 = 42;           // Move limit for depth 8
-    int moveCountHistoryThreshold = 1500; // History score threshold for bonus moves
+    int moveCountHistoryThreshold = 0;  // SPSA-tuned: Was 1500, now 0 (no history bonus)
     int moveCountHistoryBonus = 6;      // Extra moves for good history
     int moveCountImprovingRatio = 75;   // Percentage of moves when not improving (75 = 3/4)
     
     // Phase R1: Razoring parameters (conservative implementation)
     bool useRazoring = false;            // Enable/disable razoring (default false for safety)
-    int razorMargin1 = 300;              // Razoring margin for depth 1 (in centipawns)
-    int razorMargin2 = 500;              // Razoring margin for depth 2 (in centipawns)
+    int razorMargin1 = 274;              // SPSA-tuned: Was 300
+    int razorMargin2 = 468;              // SPSA-tuned: Was 500
     
@@ -84,12 +84,12 @@ struct SearchLimits {
     // Stage 21: Null Move Pruning parameters
     bool useNullMove = true;          // Enable/disable null move pruning (enabled for Phase A2)
-    int nullMoveStaticMargin = 90;   // Margin for static null move pruning - reduced from 120
-    int nullMoveMinDepth = 3;         // Minimum depth for null move pruning
-    int nullMoveReductionBase = 2;    // Base null move reduction
-    int nullMoveReductionDepth6 = 3;  // Reduction at depth >= 6
-    int nullMoveReductionDepth12 = 4; // Reduction at depth >= 12
+    int nullMoveStaticMargin = 87;   // SPSA-tuned: Was 90
+    int nullMoveMinDepth = 2;         // SPSA-tuned: Was 3
+    int nullMoveReductionBase = 4;    // SPSA-tuned: Was 2
+    int nullMoveReductionDepth6 = 4;  // SPSA-tuned: Was 3
+    int nullMoveReductionDepth12 = 5; // SPSA-tuned: Was 4
     int nullMoveVerifyDepth = 10;     // Depth threshold for verification search
-    int nullMoveEvalMargin = 200;     // Extra reduction when eval >> beta
+    int nullMoveEvalMargin = 198;     // SPSA-tuned: Was 200
     
@@ -73,8 +73,8 @@ struct SearchLimits {
     // Stage 18: Late Move Reductions (LMR) parameters
     bool lmrEnabled = true;           // Enable/disable LMR via UCI
-    int lmrMinDepth = 3;              // Minimum depth to apply LMR (0 to disable)
-    int lmrMinMoveNumber = 4;         // Start reducing after this many moves
+    int lmrMinDepth = 2;              // SPSA-tuned: Was 3
+    int lmrMinMoveNumber = 2;         // SPSA-tuned: Was 4
     int lmrBaseReduction = 1;         // Base reduction amount
     int lmrDepthFactor = 100;         // For formula: reduction = base + (depth-minDepth)/depthFactor
EOF

# Check if we can apply the patch
echo "Checking if patch can be applied..."
if patch --dry-run -p1 < spsa_params.patch 2>/dev/null; then
    echo "Patch can be applied cleanly."
    echo ""
    read -p "Apply SPSA-tuned parameters now? (y/n) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        patch -p1 < spsa_params.patch
        echo ""
        echo "SPSA parameters applied successfully!"
        echo "Please rebuild SeaJay to use the new parameters."
    else
        echo "Patch not applied. You can apply it later with: patch -p1 < spsa_params.patch"
    fi
else
    echo "Warning: Patch cannot be applied cleanly. Manual editing may be required."
    echo "The changes are saved in spsa_params.patch for reference."
fi

rm -f spsa_params.patch