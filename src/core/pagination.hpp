#pragma once

// =============================================================================
// FocusForge — Pagination
// src/core/pagination.hpp
// =============================================================================

#include <algorithm>
#include <vector>

namespace focusforge::core {

struct PageRequest {
    int limit = 20;
    int offset = 0;

    int Limit() const {
        return std::clamp(limit, 1, 100);
    }
    int Offset() const {
        return std::max(0, offset);
    }
};

template <typename T>
struct Page {
    std::vector<T> items;
    int total{};
    int limit{};
    int offset{};

    bool HasMore() const {
        return (offset + static_cast<int>(items.size())) < total;
    }

    bool IsEmpty() const {
        return items.empty();
    }
};

}  // namespace focusforge::core
