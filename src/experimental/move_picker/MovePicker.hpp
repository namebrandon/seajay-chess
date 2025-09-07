// Scaffold only — not part of build. For review before implementation.
#pragma once

// This header sketches a staged MovePicker interface that is thread-safe
// (no global state) and suitable for LazySMP where each worker owns its
// history/killers/countermove tables. It is intentionally decoupled from
// current code to allow review without integration.

namespace seajay {
    // Forward declarations (avoid including engine headers in scaffold)
    using Move = unsigned short; // placeholder, matches 16-bit Move in engine
}

namespace seajay::search {

struct MovePickerConfig {
    bool useSEEForCaptures = true;     // SEE gate for captures
    bool enableQuietHeuristics = true; // history/countermove for quiets
    int quietHeuristicMinDepth = 2;    // apply quiet heuristics from this depth
};

// Lightweight read-only views to avoid coupling
struct MovePickerInputs {
    // Pointers are opaque; in implementation, these would be references to
    // engine types: Board, HistoryHeuristic, KillerMoves, CounterMoves, etc.
    const void* board = nullptr;           // const Board*
    const void* history = nullptr;         // const HistoryHeuristic*
    const void* killers = nullptr;         // const KillerMoves*
    const void* countermoves = nullptr;    // const CounterMoves*

    Move ttMove = 0;                       // pre-probed TT move (0 = none)
    int depth = 0;                         // current depth
    int ply = 0;                           // current ply
    bool inCheck = false;                  // current node in check
    bool isPvNode = false;                 // PV node flag
};

// Staged iterator that yields moves in efficient phases, without allocating
// or sorting whole lists. Thread-safe by construction when used per node.
class MovePicker {
public:
    enum class Phase : unsigned char {
        TT,                 // TT move first
        WINNING_CAPTURES,   // SEE >= 0
        KILLERS,            // 2 killers
        QUIET_GOOD,         // history/countermove-scored quiets
        LOSING_CAPTURES,    // SEE < 0
        QUIET_REST,         // remaining quiets
        DONE
    };

    MovePicker(const MovePickerInputs& inputs, const MovePickerConfig& cfg) noexcept;

    // Return next move in the staged order; returns 0 when exhausted.
    Move next() noexcept;

    // Reset iteration (e.g., for PVS re-search) optionally switching PV flag.
    void reset(bool isPvNode) noexcept;

    // Introspection (for diagnostics)
    Phase phase() const noexcept { return m_phase; }
    int sizeHint() const noexcept { return m_totalCount; }

private:
    // Opaque internal buffers (indices into a backing move array in impl)
    static constexpr int MAX_MOVES = 256; // matches engine MoveList cap
    Move m_bufferTT[1]{};
    Move m_bufferWinCaps[MAX_MOVES]{};
    Move m_bufferKillers[2]{};
    Move m_bufferQuietGood[MAX_MOVES]{};
    Move m_bufferLoseCaps[MAX_MOVES]{};
    Move m_bufferQuietRest[MAX_MOVES]{};

    int m_countTT = 0;
    int m_countWinCaps = 0;
    int m_countKillers = 0;
    int m_countQuietGood = 0;
    int m_countLoseCaps = 0;
    int m_countQuietRest = 0;
    int m_totalCount = 0;

    int m_idxTT = 0;
    int m_idxWinCaps = 0;
    int m_idxKillers = 0;
    int m_idxQuietGood = 0;
    int m_idxLoseCaps = 0;
    int m_idxQuietRest = 0;

    Phase m_phase = Phase::TT;
    MovePickerInputs m_in{};
    MovePickerConfig m_cfg{};

    // Private helpers (would be defined in .cpp during implementation)
    void buildPhases_Quiescence() noexcept; // captures/promos only
    void buildPhases_Search() noexcept;     // full staged plan
    void advancePhase() noexcept;
};

} // namespace seajay::search

