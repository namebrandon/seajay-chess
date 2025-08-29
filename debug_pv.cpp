// Temporary debug version to track PV propagation
// Add this debug code to negamax.cpp temporarily

// In the move loop, after updating PV:
if (pv != nullptr && isPvNode) {
    // Update PV with best move and child's PV
    pv->updatePV(ply, move, childPVPtr);
    
    // DEBUG: Print PV update info
    if (ply <= 2) {  // Only debug first few plies to avoid spam
        std::cerr << "DEBUG: PV Update at ply " << ply 
                  << " move=" << SafeMoveExecutor::moveToString(move)
                  << " pvLength=" << pv->getLength(ply);
        
        // Show child PV length if available
        if (childPVPtr && !childPVPtr->isEmpty(ply + 1)) {
            std::cerr << " childPVLength=" << childPVPtr->getLength(ply + 1);
        }
        
        // Print the full PV at this ply
        std::cerr << " PV=";
        for (int i = 0; i < pv->getLength(ply); i++) {
            std::cerr << " " << SafeMoveExecutor::moveToString(pv->getMove(ply, i));
        }
        std::cerr << std::endl;
    }
}