#pragma once
// src/utils/text.hpp
// Thin re-export + дополнения поверх core/text.hpp
#include "core/text.hpp"

namespace focusforge::utils {

using core::Trim;
using core::ToLower;
using core::Split;
using core::EscapeHtml;
using core::Truncate;
using core::NormalizeTagName;
using core::ExtractHashtags;
using core::ExtractPriorityHint;
using core::IsValidRrule;
using core::FormatDuration;
using core::ProgressBar;

/// Множественное число: "1 задача" / "2 задачи" / "5 задач"
std::string PluralRu(int n, const std::string& one,
                      const std::string& few, const std::string& many);

}  // namespace focusforge::utils
