#pragma once

// =============================================================================
// FocusForge — Result<T, E> type
// src/core/result.hpp
//
// Контролируемый flow ошибок без исключений для критических путей.
// Основан на std::variant + helpers.
// =============================================================================

#include "errors.hpp"

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>

namespace focusforge::core {

// =============================================================================
// Result<T, E>
//
// Использование:
//   Result<Task, ValidationError> result = service.CreateTask(req);
//   if (result.IsOk()) {
//       auto& task = result.Value();
//   } else {
//       auto& err = result.Error();
//   }
// =============================================================================

template <typename T, typename E = DomainError>
class Result {
   public:
    // Конструкторы
    static Result Ok(T value) {
        Result r;
        r.data_ = std::move(value);
        return r;
    }

    static Result Err(E error) {
        Result r;
        r.data_ = std::move(error);
        return r;
    }

    // Проверки
    bool IsOk() const {
        return std::holds_alternative<T>(data_);
    }
    bool IsErr() const {
        return std::holds_alternative<E>(data_);
    }

    // Доступ к значению (бросает если IsErr)
    T& Value() {
        if (IsErr())
            throw std::logic_error("Result::Value() called on Err");
        return std::get<T>(data_);
    }

    const T& Value() const {
        if (IsErr())
            throw std::logic_error("Result::Value() called on Err");
        return std::get<T>(data_);
    }

    T ValueOr(T default_value) const {
        if (IsOk())
            return std::get<T>(data_);
        return std::move(default_value);
    }

    // Доступ к ошибке (бросает если IsOk)
    E& Error() {
        if (IsOk())
            throw std::logic_error("Result::Error() called on Ok");
        return std::get<E>(data_);
    }

    const E& Error() const {
        if (IsOk())
            throw std::logic_error("Result::Error() called on Ok");
        return std::get<E>(data_);
    }

    // Функциональные операторы
    template <typename F>
    auto Map(F&& f) -> Result<std::invoke_result_t<F, T>, E> {
        if (IsOk()) {
            return Result<std::invoke_result_t<F, T>, E>::Ok(f(std::get<T>(data_)));
        }
        return Result<std::invoke_result_t<F, T>, E>::Err(std::get<E>(data_));
    }

    template <typename F>
    Result AndThen(F&& f) {
        if (IsOk())
            return f(std::get<T>(data_));
        return *this;
    }

    // Конвертация Ok → throws on Err
    T Unwrap() {
        if (IsErr()) {
            throw std::runtime_error("Unwrap called on Err: " + std::get<E>(data_).Message());
        }
        return std::move(std::get<T>(data_));
    }

   private:
    Result() = default;
    std::variant<T, E> data_;
};

// Специализация для void (операции без возвращаемого значения)
template <typename E>
class Result<void, E> {
   public:
    static Result Ok() {
        Result r;
        r.is_ok_ = true;
        return r;
    }

    static Result Err(E error) {
        Result r;
        r.is_ok_ = false;
        r.error_ = std::move(error);
        return r;
    }

    bool IsOk() const {
        return is_ok_;
    }
    bool IsErr() const {
        return !is_ok_;
    }

    E& Error() {
        if (is_ok_)
            throw std::logic_error("Result<void>::Error() called on Ok");
        return *error_;
    }

   private:
    Result() = default;
    bool is_ok_ = false;
    std::optional<E> error_;
};

// =============================================================================
// Хелперы для создания результатов
// =============================================================================

template <typename T>
Result<T> Ok(T value) {
    return Result<T>::Ok(std::move(value));
}

template <typename T, typename E>
Result<T, E> Err(E error) {
    return Result<T, E>::Err(std::move(error));
}

}  // namespace focusforge::core
