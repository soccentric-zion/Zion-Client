#pragma once

#include <optional>
#include <string>
#include <utility>

namespace zion::client {

// Lightweight value-based error propagation.
// Use for expected failure paths (I/O, validation, parsing).
// Do not use for programmer errors — let those throw or assert.
template <typename T>
class Result {
public:
    static Result ok(T value) {
        Result r;
        r.value_ = std::move(value);
        return r;
    }

    static Result err(std::string message) {
        Result r;
        r.error_ = std::move(message);
        return r;
    }

    explicit operator bool() const noexcept { return value_.has_value(); }

    T& operator*() { return *value_; }
    const T& operator*() const { return *value_; }
    T* operator->() { return &*value_; }
    const T* operator->() const { return &*value_; }

    const std::string& error() const noexcept { return error_; }

private:
    std::optional<T> value_;
    std::string error_;
};

} // namespace zion::client
