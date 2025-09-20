#pragma once

#include "../core/types.h"
#include <cstdint>
#include <type_traits>

#ifndef ALWAYS_INLINE
#ifdef NDEBUG
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#else
#define ALWAYS_INLINE inline
#endif
#endif

namespace seajay::search {

struct alignas(8) NodeContext {
    static constexpr uint8_t FLAG_PV = 0x01;
    static constexpr uint8_t FLAG_ROOT = 0x02;

    Move excluded = NO_MOVE;
    uint8_t flags = 0;
    uint8_t padding = 0;

    ALWAYS_INLINE constexpr bool isPv() const {
        return (flags & FLAG_PV) != 0;
    }

    ALWAYS_INLINE constexpr bool isRoot() const {
        return (flags & FLAG_ROOT) != 0;
    }

    ALWAYS_INLINE constexpr bool hasExcludedMove() const {
        return excluded != NO_MOVE;
    }

    ALWAYS_INLINE constexpr Move excludedMove() const {
        return excluded;
    }

    ALWAYS_INLINE constexpr void setPv(bool pv) {
        flags = static_cast<uint8_t>((flags & ~FLAG_PV) | (pv ? FLAG_PV : 0));
    }

    ALWAYS_INLINE constexpr void setRoot(bool root) {
        flags = static_cast<uint8_t>((flags & ~FLAG_ROOT) | (root ? FLAG_ROOT : 0));
    }

    ALWAYS_INLINE constexpr void setExcluded(Move move) {
        excluded = move;
    }

    ALWAYS_INLINE constexpr void clearExcluded() {
        excluded = NO_MOVE;
    }
};

ALWAYS_INLINE constexpr NodeContext makeRootContext() {
    NodeContext ctx;
    ctx.setRoot(true);
    ctx.setPv(true);
    ctx.clearExcluded();
    return ctx;
}

ALWAYS_INLINE constexpr NodeContext makeChildContext(const NodeContext& parent, bool childIsPv) {
    NodeContext ctx = parent;
    ctx.setRoot(false);
    ctx.setPv(childIsPv);
    ctx.clearExcluded();
    return ctx;
}

ALWAYS_INLINE constexpr NodeContext makeExcludedContext(const NodeContext& parent, Move excludedMove) {
    NodeContext ctx = parent;
    ctx.setRoot(false);
    ctx.setExcluded(excludedMove);
    return ctx;
}

static_assert(sizeof(NodeContext) <= 8, "NodeContext too large");
static_assert(alignof(NodeContext) <= 8, "NodeContext over-aligned");
static_assert(std::is_trivially_copyable_v<NodeContext>, "NodeContext must be trivially copyable");

} // namespace seajay::search

