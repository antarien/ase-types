#pragma once

#include <optional>
#include <functional>
#include <stdexcept>
#include <utility>

namespace ase::types {

template<typename T>
class Option {
public:
    Option() : m_value(std::nullopt) {}
    Option(std::nullopt_t) : m_value(std::nullopt) {}
    Option(const T& value) : m_value(value) {}
    Option(T&& value) : m_value(std::move(value)) {}

    static Option some(const T& value) { return Option(value); }
    static Option some(T&& value) { return Option(std::move(value)); }
    static Option none() { return Option(); }

    [[nodiscard]] bool is_some() const { return m_value.has_value(); }
    [[nodiscard]] bool is_none() const { return !m_value.has_value(); }

    explicit operator bool() const { return is_some(); }

    T& unwrap() & {
        if (!is_some()) throw std::runtime_error("Called unwrap on None");
        return *m_value;
    }

    const T& unwrap() const& {
        if (!is_some()) throw std::runtime_error("Called unwrap on None");
        return *m_value;
    }

    T&& unwrap() && {
        if (!is_some()) throw std::runtime_error("Called unwrap on None");
        return std::move(*m_value);
    }

    T unwrap_or(T default_value) const& {
        return is_some() ? *m_value : std::move(default_value);
    }

    T unwrap_or(T default_value) && {
        return is_some() ? std::move(*m_value) : std::move(default_value);
    }

    template<typename F>
    T unwrap_or_else(F&& f) const& {
        return is_some() ? *m_value : std::forward<F>(f)();
    }

    template<typename U, typename F>
    Option<U> map(F&& f) const& {
        if (is_some()) return Option<U>::some(std::forward<F>(f)(*m_value));
        return Option<U>::none();
    }

    template<typename U, typename F>
    Option<U> and_then(F&& f) const& {
        if (is_some()) return std::forward<F>(f)(*m_value);
        return Option<U>::none();
    }

    template<typename F>
    Option<T> filter(F&& predicate) const& {
        if (is_some() && std::forward<F>(predicate)(*m_value)) {
            return *this;
        }
        return Option<T>::none();
    }

    const T* operator->() const { return &unwrap(); }
    T* operator->() { return &unwrap(); }
    const T& operator*() const& { return unwrap(); }
    T& operator*() & { return unwrap(); }
    T&& operator*() && { return std::move(*this).unwrap(); }

private:
    std::optional<T> m_value;
};

template<typename T>
Option<T> some(T&& value) {
    return Option<T>::some(std::forward<T>(value));
}

template<typename T>
Option<T> none() {
    return Option<T>::none();
}

}  // namespace ase::types
