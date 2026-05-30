#pragma once
// src/utils/pagination.hpp
#include "core/pagination.hpp"

namespace focusforge::utils {

using core::Page;
using core::PageRequest;

/// Построить PageRequest из HTTP query params (offset/limit)
inline core::PageRequest ParsePageRequest(int limit, int offset) {
    return {std::clamp(limit, 1, 100), std::max(0, offset)};
}

}  // namespace focusforge::utils
