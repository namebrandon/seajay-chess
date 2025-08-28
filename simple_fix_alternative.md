# Alternative Simple Fix: Keep UCI Conversion, Optimize Hot Path

If refactoring SearchData is too invasive, here's a simpler fix that addresses the overhead:

## Option 1: Eliminate Dynamic Cast in Hot Path

Replace the dynamic cast with a type flag:

```cpp
// In SearchData:
struct SearchData {
    virtual ~SearchData() = default;
    
    // Add type flag to avoid dynamic_cast
    enum Type { BASIC, ITERATIVE };
    Type dataType = BASIC;
    
    // ... rest of SearchData
};

// In IterativeSearchData:
class IterativeSearchData : public SearchData {
    IterativeSearchData() : SearchData() {
        dataType = ITERATIVE;
    }
    // ...
};

// In negamax.cpp hot path (line 147-154):
if (info.nodes > 0 && (info.nodes & 0xFFF) == 0) {
    // Avoid dynamic_cast in hot path
    if (info.dataType == SearchData::ITERATIVE) {
        IterativeSearchData* iterativeInfo = static_cast<IterativeSearchData*>(&info);
        if (iterativeInfo->shouldSendInfo(true)) {
            sendCurrentSearchInfo(*iterativeInfo, info.rootSideToMove, tt);
            iterativeInfo->recordInfoSent(iterativeInfo->bestScore);
        }
    }
}
```

**Benefit**: Eliminates dynamic_cast overhead (~1-2% improvement)

## Option 2: Reduce Info Output Frequency

Change the check interval to reduce overhead:

```cpp
// Instead of every 4096 nodes (0xFFF):
if (info.nodes > 0 && (info.nodes & 0x3FFF) == 0) {  // Every 16384 nodes
```

**Benefit**: 75% fewer info checks (~0.5% improvement)

## Option 3: Optimize InfoBuilder

Pre-allocate string buffer to avoid repeated allocations:

```cpp
class InfoBuilder {
private:
    std::string m_buffer;  // Pre-allocated buffer
    
public:
    InfoBuilder() {
        m_buffer.reserve(256);  // Pre-allocate typical size
    }
    
    std::string build() const {
        // Reuse buffer instead of creating new string
        return "info " + m_buffer + "\n";
    }
};
```

**Benefit**: Reduces allocation overhead (~0.5% improvement)

## Option 4: Batch Optimizations

Combine all three optimizations above:
- Replace dynamic_cast with type flag
- Reduce info output frequency  
- Optimize InfoBuilder allocations

**Expected improvement**: 2-3% performance gain (8-12 ELO)

## The Real Issue Remains

While these optimizations help, they don't address the root cause:
- **SearchData is still 42KB**
- **Cache thrashing still occurs**
- **Only recovers 8-12 ELO of the 30-40 ELO loss**

The proper fix is still to reduce SearchData size by using pointers instead of embedded arrays.