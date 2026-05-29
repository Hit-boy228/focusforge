#pragma once
// src/dto/common.hpp
#include <string>
#include <vector>

namespace focusforge::dto {

struct ErrorResponse {
    std::string code;
    std::string message;
    std::string request_id;
};

template <typename T>
struct PagedResponse {
    std::vector<T> items;
    int total{};
    int limit{};
    int offset{};
    bool has_more{};
};

struct OkResponse {
    bool ok = true;
    std::string message;
};

}  // namespace focusforge::dto
