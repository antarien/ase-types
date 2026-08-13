#pragma once

#include <stdexcept>
#include <utility>
#include <string>

namespace ase::types {

/**
 * Result<T, E> - the ASE ok-or-error carrier (WRFL_ASE_STD_FORBIDDEN).
 *
 * Storage is an ok-slot plus err-slot plus discriminant instead of std::variant: the offer
 * itself may not be built on forbidden vocabulary (validator rules STD_VARIANT_FORBIDDEN /
 * STD_FUNCTION_FORBIDDEN hit its own implementation, fixed 2026-08-10). Both slots are
 * default-constructed and only the discriminated one is ever observable through the API -
 * the ASE ecosystem is POD-oriented and zero-init is its contract, so the idle slot holding
 * a zero-initialised value is the normal state of things, not a cost.
 */
template<typename T, typename E>
class Result {
public:
    using OkType = T;
    using ErrType = E;

    static Result ok(const T& value) { return Result(OkTag{}, value); }
    static Result ok(T&& value) { return Result(OkTag{}, std::move(value)); }
    static Result err(const E& error) { return Result(ErrTag{}, error); }
    static Result err(E&& error) { return Result(ErrTag{}, std::move(error)); }

    [[nodiscard]] bool is_ok() const { return m_is_ok; }
    [[nodiscard]] bool is_err() const { return !m_is_ok; }

    explicit operator bool() const { return is_ok(); }

    T& unwrap() & {
        if (!is_ok()) throw std::runtime_error("Called unwrap on Err");
        return m_ok_value;
    }

    const T& unwrap() const& {
        if (!is_ok()) throw std::runtime_error("Called unwrap on Err");
        return m_ok_value;
    }

    T&& unwrap() && {
        if (!is_ok()) throw std::runtime_error("Called unwrap on Err");
        return std::move(m_ok_value);
    }

    E& unwrap_err() & {
        if (!is_err()) throw std::runtime_error("Called unwrap_err on Ok");
        return m_err_value;
    }

    const E& unwrap_err() const& {
        if (!is_err()) throw std::runtime_error("Called unwrap_err on Ok");
        return m_err_value;
    }

    T unwrap_or(T default_value) const& {
        return is_ok() ? m_ok_value : std::move(default_value);
    }

    T unwrap_or(T default_value) && {
        return is_ok() ? std::move(m_ok_value) : std::move(default_value);
    }

    template<typename F>
    T unwrap_or_else(F&& f) const& {
        return is_ok() ? m_ok_value : std::forward<F>(f)(m_err_value);
    }

    template<typename U, typename F>
    Result<U, E> map(F&& f) const& {
        if (is_ok()) {
            return Result<U, E>::ok(std::forward<F>(f)(m_ok_value));
        }
        return Result<U, E>::err(m_err_value);
    }

    template<typename U, typename F>
    Result<U, E> and_then(F&& f) const& {
        if (is_ok()) {
            return std::forward<F>(f)(m_ok_value);
        }
        return Result<U, E>::err(m_err_value);
    }

    template<typename F, typename U>
    Result<T, U> map_err(F&& f) const& {
        if (is_err()) {
            return Result<T, U>::err(std::forward<F>(f)(m_err_value));
        }
        return Result<T, U>::ok(m_ok_value);
    }

    const T* operator->() const { return &unwrap(); }
    T* operator->() { return &unwrap(); }
    const T& operator*() const& { return unwrap(); }
    T& operator*() & { return unwrap(); }

private:
    struct OkTag {};
    struct ErrTag {};

    Result(OkTag, const T& value) : m_ok_value(value), m_err_value{}, m_is_ok(true) {}
    Result(OkTag, T&& value) : m_ok_value(std::move(value)), m_err_value{}, m_is_ok(true) {}
    Result(ErrTag, const E& error) : m_ok_value{}, m_err_value(error), m_is_ok(false) {}
    Result(ErrTag, E&& error) : m_ok_value{}, m_err_value(std::move(error)), m_is_ok(false) {}

    T m_ok_value;
    E m_err_value;
    bool m_is_ok;
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
