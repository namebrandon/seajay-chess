// Scaffold only — not part of build. For review before implementation.
#pragma once

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <new>

// This header sketches a clustered (set-associative) Transposition Table
// that is safe for LazySMP: lock-free, benign data races, generation-based.
// It mirrors the existing public API shape for ease of integration.

namespace seajay {

using Hash = std::uint64_t;
using Move = std::uint16_t; // placeholder

enum class Bound : std::uint8_t { NONE=0, EXACT=1, LOWER=2, UPPER=3 };

struct alignas(16) TTEntryScaffold {
    std::uint32_t key32;     // upper 32 bits of key
    std::uint16_t move;      // best move (0 if none)
    std::int16_t score;      // search score (mate adjusted externally)
    std::int16_t evalScore;  // static eval or sentinel
    std::uint8_t depth;      // search depth (plies)
    std::uint8_t genBound;   // generation (6 bits) | bound (2 bits)
    std::uint8_t pad[4];

    inline std::uint8_t generation() const noexcept { return genBound >> 2; }
    inline Bound bound() const noexcept { return static_cast<Bound>(genBound & 0x03); }
    inline bool isEmpty() const noexcept { return genBound == 0; }

    inline void save(std::uint32_t k, Move m, std::int16_t s, std::int16_t e,
                     std::uint8_t d, Bound b, std::uint8_t gen) noexcept {
        key32 = k; move = m; score = s; evalScore = e; depth = d;
        genBound = static_cast<std::uint8_t>((gen << 2) | static_cast<std::uint8_t>(b));
    }
};

static_assert(sizeof(TTEntryScaffold)==16, "entry must be 16 bytes");

template<int CLUSTER=4>
struct alignas(64) TTCluster {
    TTEntryScaffold e[CLUSTER];
};

struct TTStatsScaffold {
    std::atomic<std::uint64_t> probes{0}, hits{0}, stores{0}, collisions{0};
    std::atomic<std::uint64_t> probeEmpties{0}, probeMismatches{0};
};

class ClusteredTranspositionTable {
public:
    explicit ClusteredTranspositionTable(std::size_t sizeMB = 16) { resize(sizeMB); }
    ~ClusteredTranspositionTable() { release(); }

    // non-copyable, movable omitted for brevity in scaffold

    inline void newSearch() noexcept { m_generation = (m_generation + 1) & 0x3F; }
    inline void setEnabled(bool en) noexcept { m_enabled = en; }
    inline bool isEnabled() const noexcept { return m_enabled; }

    void resize(std::size_t sizeMB) {
        release();
        // Round to clusters
        std::size_t bytes = sizeMB * 1024ULL * 1024ULL;
        if (bytes < sizeof(TTCluster<>)) bytes = sizeof(TTCluster<>);
        // number of clusters (power of two)
        std::size_t clusters = bytes / sizeof(TTCluster<>) ;
        // round down to power of two
        std::size_t p2 = 1; while ((p2<<1) <= clusters) p2 <<= 1; clusters = p2;
        m_mask = clusters - 1;
        m_clusters = static_cast<TTCluster<>*>(::operator new[](clusters * sizeof(TTCluster<>), std::align_val_t(64)));
        m_numClusters = clusters;
        // zero entries
        std::byte* p = reinterpret_cast<std::byte*>(m_clusters);
        std::size_t total = clusters * sizeof(TTCluster<>);
        for (std::size_t i=0;i<total;++i) p[i] = std::byte{0};
    }

    inline void prefetch(Hash key) const noexcept {
        if (!m_enabled || !m_clusters) return;
        auto idx = index(key);
        __builtin_prefetch(&m_clusters[idx], 0, 1);
    }

    // Probe: returns pointer to entry if key32 matches any in cluster; nullptr otherwise.
    TTEntryScaffold* probe(Hash key) noexcept {
        if (!m_enabled || !m_clusters) return nullptr;
        m_stats.probes++;
        auto idx = index(key);
        auto& cl = m_clusters[idx];
        std::uint32_t k32 = static_cast<std::uint32_t>(key >> 32);
        bool anyNonEmpty = false;
        for (auto& e : cl.e) {
            if (e.isEmpty()) continue; else anyNonEmpty = true;
            if (e.key32 == k32) { m_stats.hits++; return &e; }
        }
        if (!anyNonEmpty) m_stats.probeEmpties++; else m_stats.probeMismatches++;
        return nullptr;
    }

    // Store with clustered victim selection (best-effort, lock-free)
    void store(Hash key, Move move, std::int16_t score, std::int16_t eval,
               std::uint8_t depth, Bound bound) noexcept {
        if (!m_enabled || !m_clusters) return;
        m_stats.stores++;
        auto idx = index(key);
        auto& cl = m_clusters[idx];
        std::uint32_t k32 = static_cast<std::uint32_t>(key >> 32);

        // First pass: exact match update (prefer deeper/EXACT)
        for (auto& e : cl.e) {
            if (!e.isEmpty() && e.key32 == k32) {
                // Prefer keeping moves and deeper entries
                bool replace = (depth >= e.depth);
                if (replace) e.save(k32, move, score, eval, depth, bound, m_generation);
                return;
            }
        }

        // Choose victim with policy
        TTEntryScaffold* victim = nullptr;
        // 1) empty
        for (auto& e : cl.e) { if (e.isEmpty()) { victim = &e; break; } }
        if (!victim) {
            // 2) old generation
            for (auto& e : cl.e) { if (e.generation() != m_generation) { victim = &e; break; } }
        }
        if (!victim) {
            // 3) shallower depth
            victim = &cl.e[0];
            for (auto& e : cl.e) { if (e.depth < victim->depth) victim = &e; }
        }
        // 4/5) prefer replacing non-EXACT or NO_MOVE if depth similar — simplified in scaffold
        victim->save(k32, move, score, eval, depth, bound, m_generation);
    }

    void clear() noexcept {
        if (!m_clusters) return;
        std::byte* p = reinterpret_cast<std::byte*>(m_clusters);
        std::size_t total = m_numClusters * sizeof(TTCluster<>);
        for (std::size_t i=0;i<total;++i) p[i] = std::byte{0};
        m_generation = 0;
    }

    const TTStatsScaffold& stats() const noexcept { return m_stats; }

private:
    TTCluster<>* m_clusters = nullptr;
    std::size_t m_numClusters = 0;
    std::size_t m_mask = 0; // cluster mask
    std::uint8_t m_generation = 0;
    bool m_enabled = true;
    mutable TTStatsScaffold m_stats{};

    inline std::size_t index(Hash key) const noexcept {
        // mix + mask to cluster index
        return static_cast<std::size_t>((key * 0x9E3779B97F4A7C15ULL) & m_mask);
    }

    void release() noexcept {
        if (m_clusters) {
            ::operator delete[](m_clusters, std::align_val_t(64));
            m_clusters = nullptr; m_numClusters = 0; m_mask = 0;
        }
    }
};

} // namespace seajay

