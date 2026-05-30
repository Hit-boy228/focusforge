#pragma once
// src/utils/text.hpp
// Thin re-export + дополнения поверх core/text.hpp
#include "core/text.hpp"

namespace focusforge::utils {

using core::EscapeHtml;
using core::ExtractHashtags;
using core::ExtractPriorityHint;
using core::FormatDuration;
using core::IsValidRrule;
using core::NormalizeTagName;
using core::ProgressBar;
using core::Split;
using core::ToLower;
using core::Trim;
using core::Truncate;

/// Множественное число: "1 задача" / "2 задачи" / "5 задач"
std::string PluralRu(int n, const std::string& one, const std::string& few,
                     const std::string& many);

}  // namespace focusforge::utils
