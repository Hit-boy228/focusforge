// =============================================================================
// FocusForge — main.cpp
// Точка входа приложения.
// =============================================================================

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#include "app/component_list.hpp"

int main(int argc, char* argv[]) {
    // Формируем список компонентов: минимальный server stack + наши компоненты
    auto component_list = userver::components::MinimalServerComponentList();
    focusforge::app::AppendComponents(component_list);

    // userver daemon main: читает конфиг, инициализирует компоненты,
    // запускает event loop, обрабатывает сигналы SIGTERM/SIGINT
    return userver::utils::DaemonMain(argc, argv, component_list);
}
