#pragma once

/**
 * ASE Layer 0 TYPE - Presence Carrier
 *
 * @file        option.hpp
 * @brief       Option<T> - the ASE replacement for std::optional
 * @description A value that may or may not be there, carried WITHOUT the vocabulary it exists
 *              to replace. WRFL_ASE_STD_FORBIDDEN Section 1 bans std::optional across the tree,
 *              and the offer standing in for it may not be built on it - the validator rule
 *              STD_OPTIONAL_FORBIDDEN hit this very implementation on 2026-08-10, which is why
 *              the storage is a value-plus-flag pair and not a wrapped std::optional.
 *
 *              T IS DEFAULT-CONSTRUCTED IN THE None STATE, and that is a contract, not an
 *              accident: the ASE component ecosystem is POD-oriented and zero-init is what it
 *              promises. An empty Option therefore holds a zero-initialised T that is never
 *              observable through the API - every accessor gates on m_has first.
 *
 *              Import via:
 *                #include <ase/types/option.hpp>
 *                using ase::types::Option;
 *
 * @module      ase-types
 * @layer       0 (Foundation)
 * @category    error/result/optional
 * @created     2025-12-15
 * @modified    2026-08-20
 * @version     1.1.0
 */

#include <stdexcept>
#include <utility>

namespace ase::types {

/**
 * Option<T> - the ASE replacement for std::optional (WRFL_ASE_STD_FORBIDDEN, Section 1).
 *
 * Storage is a value-plus-flag pair instead of std::optional: the offer itself may not be built
 * on the vocabulary it exists to replace (validator rule STD_OPTIONAL_FORBIDDEN hit its own
 * implementation, fixed 2026-08-10). T is default-constructed in the None state - the ASE
 * component ecosystem is POD-oriented and zero-init is its contract, so an empty Option holds a
 * zero-initialised T that is never observable through the API (every accessor gates on m_has).
 */
template<typename T>
class Option {
public:
    Option() : m_value{}, m_has(false) {}
    Option(const T& value) : m_value(value), m_has(true) {}
    Option(T&& value) : m_value(std::move(value)), m_has(true) {}

    static Option some(const T& value) { return Option(value); }
    static Option some(T&& value) { return Option(std::move(value)); }
    static Option none() { return Option(); }

    [[nodiscard]] bool is_some() const { return m_has; }
    [[nodiscard]] bool is_none() const { return !m_has; }

    explicit operator bool() const { return is_some(); }

    T& unwrap() & {
        if (!is_some()) throw std::runtime_error("Called unwrap on None");
        return m_value;
    }

    const T& unwrap() const& {
        if (!is_some()) throw std::runtime_error("Called unwrap on None");
        return m_value;
    }

    T&& unwrap() && {
        if (!is_some()) throw std::runtime_error("Called unwrap on None");
        return std::move(m_value);
    }

    T unwrap_or(T default_value) const& {
        return is_some() ? m_value : std::move(default_value);
    }

    T unwrap_or(T default_value) && {
        return is_some() ? std::move(m_value) : std::move(default_value);
    }

    template<typename F>
    T unwrap_or_else(F&& f) const& {
        return is_some() ? m_value : std::forward<F>(f)();
    }

    template<typename U, typename F>
    Option<U> map(F&& f) const& {
        if (is_some()) return Option<U>::some(std::forward<F>(f)(m_value));
        return Option<U>::none();
    }

    template<typename U, typename F>
    Option<U> and_then(F&& f) const& {
        if (is_some()) return std::forward<F>(f)(m_value);
        return Option<U>::none();
    }

    template<typename F>
    Option<T> filter(F&& predicate) const& {
        if (is_some() && std::forward<F>(predicate)(m_value)) {
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
    T m_value;
    bool m_has;
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
