#pragma once

// =============================================================================
// FocusForge — Component Registration
// src/app/component_list.hpp
// =============================================================================

#include <userver/components/component_list.hpp>

namespace focusforge::app {

/// Регистрирует все компоненты приложения в переданный список.
/// Порядок важен: зависимые компоненты после их зависимостей.
void AppendComponents(userver::components::ComponentList& list);

}  // namespace focusforge::app
