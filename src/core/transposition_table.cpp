#include "transposition_table.h"
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <bit>
#include <cassert>

namespace seajay {

// AlignedBuffer implementation
AlignedBuffer::AlignedBuffer(size_t size) : m_size(size) {
    if (size > 0) {
        // Allocate 64-byte aligned memory for cache line optimization
        m_data = std::aligned_alloc(64, size);
        if (!m_data) {
            throw std::bad_alloc();
        }
        // Zero-initialize for consistency
        std::memset(m_data, 0, size);
    }
}

AlignedBuffer::~AlignedBuffer() {
    free();
}

AlignedBuffer::AlignedBuffer(AlignedBuffer&& other) noexcept
    : m_data(other.m_data), m_size(other.m_size) {
    other.m_data = nullptr;
    other.m_size = 0;
}

AlignedBuffer& AlignedBuffer::operator=(AlignedBuffer&& other) noexcept {
    if (this != &other) {
        free();
        m_data = other.m_data;
        m_size = other.m_size;
        other.m_data = nullptr;
        other.m_size = 0;
    }
    return *this;
}

void AlignedBuffer::resize(size_t newSize) {
    if (newSize != m_size) {
        free();
        m_size = newSize;
        if (newSize > 0) {
            m_data = std::aligned_alloc(64, newSize);
            if (!m_data) {
                m_size = 0;
                throw std::bad_alloc();
            }
            std::memset(m_data, 0, newSize);
        }
    }
}

void AlignedBuffer::clear() {
    if (m_data && m_size > 0) {
        std::memset(m_data, 0, m_size);
    }
}

void AlignedBuffer::free() {
    if (m_data) {
        std::free(m_data);
        m_data = nullptr;
        m_size = 0;
    }
}

// TranspositionTable implementation
TranspositionTable::TranspositionTable() 
    : TranspositionTable(
#ifdef NDEBUG
        DEFAULT_SIZE_MB_RELEASE
#else
        DEFAULT_SIZE_MB_DEBUG
#endif
    ) {}

TranspositionTable::TranspositionTable(size_t sizeInMB)
    : m_generation(0), m_enabled(true) {
    resize(sizeInMB);
}

void TranspositionTable::resize(size_t sizeInMB) {
    // Calculate number of entries (must be power of 2)
    m_numEntries = calculateNumEntries(sizeInMB);
    m_mask = m_numEntries - 1;
    
    // Allocate buffer
    size_t bufferSize = m_numEntries * sizeof(TTEntry);
    m_buffer.resize(bufferSize);
    m_entries = static_cast<TTEntry*>(m_buffer.data());
    
    // Reset statistics
    m_stats.reset();
    
    // Validate alignment in debug builds
#ifndef NDEBUG
    assert(reinterpret_cast<uintptr_t>(m_entries) % 64 == 0);
    assert(std::has_single_bit(m_numEntries));  // Power of 2 check
#endif
}

void TranspositionTable::store(Hash key, Move move, int16_t score, 
                               int16_t evalScore, uint8_t depth, Bound bound) {
    if (!m_enabled) return;
    
    m_stats.stores++;
    
    size_t idx = index(key);
    TTEntry* entry = &m_entries[idx];
    
    // Check for collision (different position mapping to same index)
    uint32_t key32 = static_cast<uint32_t>(key >> 32);
    if (!entry->isEmpty() && entry->key32 != key32) {
        m_stats.collisions++;
    }
    
    // Depth-preferred replacement strategy
    // Replace if:
    // 1. Entry is empty
    // 2. Entry is from a different generation (old search)
    // 3. New search is at least as deep as existing entry
    const bool canReplace = entry->isEmpty()
                         || entry->generation() != m_generation
                         || depth >= entry->depth;
    
    if (canReplace) {
        entry->save(key32, move, score, evalScore, depth, bound, m_generation);
    }
}

TTEntry* TranspositionTable::probe(Hash key) {
    if (!m_enabled) return nullptr;
    
    m_stats.probes++;
    
    size_t idx = index(key);
    TTEntry* entry = &m_entries[idx];
    
    // Check if this is the position we're looking for
    uint32_t key32 = static_cast<uint32_t>(key >> 32);
    if (entry->key32 == key32) {
        m_stats.hits++;
        return entry;
    }
    
    return nullptr;
}

void TranspositionTable::clear() {
    m_buffer.clear();
    m_stats.reset();
    m_generation = 0;
}

double TranspositionTable::fillRate() const {
    if (m_numEntries == 0) return 0.0;
    
    // Sample a subset of entries for performance
    constexpr size_t SAMPLE_SIZE = 1000;
    size_t sampleSize = std::min(SAMPLE_SIZE, m_numEntries);
    size_t used = 0;
    
    for (size_t i = 0; i < sampleSize; ++i) {
        size_t idx = (i * m_numEntries) / sampleSize;
        if (!m_entries[idx].isEmpty()) {
            used++;
        }
    }
    
    return 100.0 * used / sampleSize;
}

size_t TranspositionTable::hashfull() const {
    if (m_numEntries == 0) return 0;
    
    // Standard UCI hashfull: sample 1000 entries
    constexpr size_t SAMPLE_SIZE = 1000;
    size_t used = 0;
    
    for (size_t i = 0; i < SAMPLE_SIZE; ++i) {
        size_t idx = (i * m_numEntries) / SAMPLE_SIZE;
        if (!m_entries[idx].isEmpty() && 
            m_entries[idx].generation() == m_generation) {
            used++;
        }
    }
    
    return used;  // Returns value 0-1000
}

size_t TranspositionTable::calculateNumEntries(size_t sizeInMB) {
    size_t sizeInBytes = sizeInMB * 1024 * 1024;
    size_t numEntries = sizeInBytes / sizeof(TTEntry);
    
    // Round down to nearest power of 2 for fast indexing
    if (numEntries == 0) return 1;
    
    // Find the highest bit set
    size_t powerOf2 = 1;
    while (powerOf2 <= numEntries / 2) {
        powerOf2 <<= 1;
    }
    
    return powerOf2;
}

} // namespace seajay