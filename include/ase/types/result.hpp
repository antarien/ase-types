#pragma once

#include <variant>
#include <functional>
#include <stdexcept>
#include <utility>
#include <string>

namespace ase::types {

template<typename T, typename E>
class Result {
public:
    using OkType = T;
    using ErrType = E;

    static Result ok(const T& value) { return Result(OkTag{}, value); }
    static Result ok(T&& value) { return Result(OkTag{}, std::move(value)); }
    static Result err(const E& error) { return Result(ErrTag{}, error); }
    static Result err(E&& error) { return Result(ErrTag{}, std::move(error)); }

    [[nodiscard]] bool is_ok() const { return std::holds_alternative<OkValue>(m_value); }
    [[nodiscard]] bool is_err() const { return std::holds_alternative<ErrValue>(m_value); }

    explicit operator bool() const { return is_ok(); }

    T& unwrap() & {
        if (!is_ok()) throw std::runtime_error("Called unwrap on Err");
        return std::get<OkValue>(m_value).value;
    }

    const T& unwrap() const& {
        if (!is_ok()) throw std::runtime_error("Called unwrap on Err");
        return std::get<OkValue>(m_value).value;
    }

    T&& unwrap() && {
        if (!is_ok()) throw std::runtime_error("Called unwrap on Err");
        return std::move(std::get<OkValue>(m_value).value);
    }

    E& unwrap_err() & {
        if (!is_err()) throw std::runtime_error("Called unwrap_err on Ok");
        return std::get<ErrValue>(m_value).error;
    }

    const E& unwrap_err() const& {
        if (!is_err()) throw std::runtime_error("Called unwrap_err on Ok");
        return std::get<ErrValue>(m_value).error;
    }

    T unwrap_or(T default_value) const& {
        return is_ok() ? std::get<OkValue>(m_value).value : std::move(default_value);
    }

    T unwrap_or(T default_value) && {
        return is_ok() ? std::move(std::get<OkValue>(m_value).value) : std::move(default_value);
    }

    template<typename F>
    T unwrap_or_else(F&& f) const& {
        return is_ok() ? std::get<OkValue>(m_value).value : std::forward<F>(f)(std::get<ErrValue>(m_value).error);
    }

    template<typename U, typename F>
    Result<U, E> map(F&& f) const& {
        if (is_ok()) {
            return Result<U, E>::ok(std::forward<F>(f)(std::get<OkValue>(m_value).value));
        }
        return Result<U, E>::err(std::get<ErrValue>(m_value).error);
    }

    template<typename U, typename F>
    Result<U, E> and_then(F&& f) const& {
        if (is_ok()) {
            return std::forward<F>(f)(std::get<OkValue>(m_value).value);
        }
        return Result<U, E>::err(std::get<ErrValue>(m_value).error);
    }

    template<typename F, typename U>
    Result<T, U> map_err(F&& f) const& {
        if (is_err()) {
            return Result<T, U>::err(std::forward<F>(f)(std::get<ErrValue>(m_value).error));
        }
        return Result<T, U>::ok(std::get<OkValue>(m_value).value);
    }

    const T* operator->() const { return &unwrap(); }
    T* operator->() { return &unwrap(); }
    const T& operator*() const& { return unwrap(); }
    T& operator*() & { return unwrap(); }

private:
    struct OkTag {};
    struct ErrTag {};
    struct OkValue { T value; };
    struct ErrValue { E error; };

    Result(OkTag, const T& value) : m_value(OkValue{value}) {}
    Result(OkTag, T&& value) : m_value(OkValue{std::move(value)}) {}
    Result(ErrTag, const E& error) : m_value(ErrValue{error}) {}
    Result(ErrTag, E&& error) : m_value(ErrValue{std::move(error)}) {}

    std::variant<OkValue, ErrValue> m_value;
};

template<typename T, typename E>
Result<T, E> ok(T&& value) {
    return Result<T, E>::ok(std::forward<T>(value));
}

template<typename T, typename E>
Result<T, E> err(E&& error) {
    return Result<T, E>::err(std::forward<E>(error));
}

using Error = std::string;

template<typename T>
using SimpleResult = Result<T, Error>;

}  // namespace ase::types
