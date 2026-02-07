#pragma once

#include <functional>
#include <source_location>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace feer {

/**
 * @brief Error payload used by feer::Result.
 *
 * Keeps a human-readable message and source location of construction.
 */
struct Err {
    /** Human-readable error message. */
    std::string message;

    /** Source location captured at error construction time. */
    std::source_location where = std::source_location::current();

    /**
     * @brief Constructs an Err.
     * @param in_message Error message.
     * @param in_where Source location for diagnostics.
     */
    explicit Err(
        std::string in_message,
        std::source_location in_where = std::source_location::current())
        : message(std::move(in_message)), where(in_where) {}
};

template <typename T>
class Result;

template <>
class Result<void>;

/**
 * @brief Constructs a successful Result<void>.
 */
[[nodiscard]] Result<void> Ok();

/**
 * @brief Result container for success value `T` or `Err`.
 *
 * @tparam T Success type.
 *
 * Constraints:
 * - `T` must not be `feer::Err`.
 * - `T` must not be an rvalue-reference (`U&&`).
 *
 * Usage pattern:
 * @code
 * if (auto r = do_work()) {
 *     use(r.value());
 * } else {
 *     log(r.error().message);
 * }
 * @endcode
 */
template <typename T>
class Result {

    static_assert(
        !std::is_same_v<std::remove_cvref_t<T>, Err>,
        "Result<T>: T must not be feer::Err");

    static_assert(
        !std::is_rvalue_reference_v<T>,
        "Result<T>: rvalue reference types (T&&) are not supported");

public:
    using value_type = std::remove_reference_t<T>;
    using stored_type = std::conditional_t<std::is_reference_v<T>, std::reference_wrapper<value_type>, value_type>;

    /** Construct success result from lvalue value (non-reference T). */
    Result(const value_type& value) requires(!std::is_reference_v<T>) : m_state(value) {}

    /** Construct success result from rvalue value (non-reference T). */
    Result(value_type&& value) requires(!std::is_reference_v<T>) : m_state(std::move(value)) {}

    /** Construct success result from lvalue reference (reference T). */
    Result(value_type& value) requires(std::is_reference_v<T>) : m_state(std::ref(value)) {}

    /** Construct error result from lvalue Err. */
    Result(const Err& err) : m_state(err) {}

    /** Construct error result from rvalue Err. */
    Result(Err&& err) : m_state(std::move(err)) {}

    /** @brief True when this object currently holds a success value. */
    [[nodiscard]] bool is_ok() const noexcept { return std::holds_alternative<stored_type>(m_state); }

    /** @brief True when this object currently holds an error. */
    [[nodiscard]] bool is_err() const noexcept { return std::holds_alternative<Err>(m_state); }

    /** @brief Convenience bool conversion. Equivalent to is_ok(). */
    [[nodiscard]] explicit operator bool() const noexcept { return is_ok(); }

    /**
     * @brief Returns mutable success value.
     * @throws std::bad_variant_access if current state is error.
     */
    [[nodiscard]] decltype(auto) value() & {
        if constexpr (std::is_reference_v<T>) {
            return std::get<stored_type>(m_state).get();
        } else {
            return (std::get<stored_type>(m_state));
        }
    }

    /**
     * @brief Returns const success value.
     * @throws std::bad_variant_access if current state is error.
     */
    [[nodiscard]] decltype(auto) value() const & {
        if constexpr (std::is_reference_v<T>) {
            return std::get<stored_type>(m_state).get();
        } else {
            return (std::get<stored_type>(m_state));
        }
    }

    /**
     * @brief Moves success value out of an rvalue Result.
     * @throws std::bad_variant_access if current state is error.
     */
    [[nodiscard]] value_type&& value() && requires(!std::is_reference_v<T>) {
        return std::get<stored_type>(std::move(m_state));
    }

    /**
     * @brief Returns contained value, or fallback if in error state.
     * @param default_value Fallback value.
     */
    template <typename U>
    [[nodiscard]] value_type value_or(U&& default_value) const& requires(!std::is_reference_v<T>) {
        if (is_ok()) {
            return std::get<stored_type>(m_state);
        }
        return static_cast<value_type>(std::forward<U>(default_value));
    }

    /**
     * @brief Returns moved contained value, or fallback if in error state.
     * @param default_value Fallback value.
     */
    template <typename U>
    [[nodiscard]] value_type value_or(U&& default_value) && requires(!std::is_reference_v<T>) {
        if (is_ok()) {
            return std::get<stored_type>(std::move(m_state));
        }
        return static_cast<value_type>(std::forward<U>(default_value));
    }

    /**
     * @brief Pattern match over success/error state.
     * @param on_ok Called with success value when state is ok.
     * @param on_err Called with const Err when state is error.
     * @return Handler return value. Both handlers must return the same type.
     */
    template <typename OkFn, typename ErrFn>
    [[nodiscard]] auto match(OkFn&& on_ok, ErrFn&& on_err) const& {
        using ok_arg_type = std::conditional_t<std::is_reference_v<T>, T, const value_type&>;

        using ok_return_type = std::invoke_result_t<OkFn, ok_arg_type>;
        using err_return_type = std::invoke_result_t<ErrFn, const Err&>;

        static_assert(
            std::is_same_v<ok_return_type, err_return_type>,
            "match requires both handlers to return the same type");

        if (is_ok()) {
            return std::invoke(std::forward<OkFn>(on_ok), value());
        }
        return std::invoke(std::forward<ErrFn>(on_err), error());
    }

    /**
     * @brief Pattern match over rvalue success/error state.
     * @param on_ok Called with moved success value when state is ok.
     * @param on_err Called with moved Err when state is error.
     * @return Handler return value. Both handlers must return the same type.
     */
    template <typename OkFn, typename ErrFn>
    [[nodiscard]] auto match(OkFn&& on_ok, ErrFn&& on_err) && requires(!std::is_reference_v<T>) {
        using ok_return_type = std::invoke_result_t<OkFn, value_type&&>;
        using err_return_type = std::invoke_result_t<ErrFn, Err&&>;

        static_assert(
            std::is_same_v<ok_return_type, err_return_type>,
            "match requires both handlers to return the same type");

        if (is_ok()) {
            return std::invoke(std::forward<OkFn>(on_ok), std::get<stored_type>(std::move(m_state)));
        }
        return std::invoke(std::forward<ErrFn>(on_err), std::get<Err>(std::move(m_state)));
    }

    /**
     * @brief Returns mutable error.
     * @throws std::bad_variant_access if current state is success.
     */
    [[nodiscard]] Err& error() & { return std::get<Err>(m_state); }

    /**
     * @brief Returns const error.
     * @throws std::bad_variant_access if current state is success.
     */
    [[nodiscard]] const Err& error() const& { return std::get<Err>(m_state); }

private:
    std::variant<stored_type, Err> m_state;
};

template <>
class Result<void> {
public:
    /** Construct success result for void. */
    Result() : m_state(std::monostate{}) {}

    /** Construct error result from lvalue Err. */
    Result(const Err& err) : m_state(err) {}

    /** Construct error result from rvalue Err. */
    Result(Err&& err) : m_state(std::move(err)) {}

    /** @brief True when this object currently holds success. */
    [[nodiscard]] bool is_ok() const noexcept { return std::holds_alternative<std::monostate>(m_state); }

    /** @brief True when this object currently holds an error. */
    [[nodiscard]] bool is_err() const noexcept { return std::holds_alternative<Err>(m_state); }

    /** @brief Convenience bool conversion. Equivalent to is_ok(). */
    [[nodiscard]] explicit operator bool() const noexcept { return is_ok(); }

    /**
     * @brief Pattern match over success/error state.
     * @param on_ok Called with no parameters when state is ok.
     * @param on_err Called with const Err when state is error.
     * @return Handler return value. Both handlers must return the same type.
     */
    template <typename OkFn, typename ErrFn>
    [[nodiscard]] auto match(OkFn&& on_ok, ErrFn&& on_err) const& {
        using ok_return_type = std::invoke_result_t<OkFn>;
        using err_return_type = std::invoke_result_t<ErrFn, const Err&>;

        static_assert(
            std::is_same_v<ok_return_type, err_return_type>,
            "match requires both handlers to return the same type");

        if (is_ok()) {
            return std::invoke(std::forward<OkFn>(on_ok));
        }
        return std::invoke(std::forward<ErrFn>(on_err), error());
    }

    /**
     * @brief Pattern match over rvalue success/error state.
     * @param on_ok Called with no parameters when state is ok.
     * @param on_err Called with moved Err when state is error.
     * @return Handler return value. Both handlers must return the same type.
     */
    template <typename OkFn, typename ErrFn>
    [[nodiscard]] auto match(OkFn&& on_ok, ErrFn&& on_err) && {
        using ok_return_type = std::invoke_result_t<OkFn>;
        using err_return_type = std::invoke_result_t<ErrFn, Err&&>;

        static_assert(
            std::is_same_v<ok_return_type, err_return_type>,
            "match requires both handlers to return the same type");

        if (is_ok()) {
            return std::invoke(std::forward<OkFn>(on_ok));
        }
        return std::invoke(std::forward<ErrFn>(on_err), std::get<Err>(std::move(m_state)));
    }

    /**
     * @brief Returns mutable error.
     * @throws std::bad_variant_access if current state is success.
     */
    [[nodiscard]] Err& error() & { return std::get<Err>(m_state); }

    /**
     * @brief Returns const error.
     * @throws std::bad_variant_access if current state is success.
     */
    [[nodiscard]] const Err& error() const& { return std::get<Err>(m_state); }

private:
    std::variant<std::monostate, Err> m_state;
};

inline Result<void> Ok() {
    return Result<void>{};
}

}  // namespace feer
