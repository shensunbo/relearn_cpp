#pragma once
#include <any>
#include <functional>
#include <mutex>
#include <vector>
#include <typeindex>
#include <stdexcept>
#include <shared_mutex>
#include "mylog.h"

class Attribute {
public:
    using ValueChangedCallback = std::function<void(const std::any& oldValue, const std::any& newValue)>;

    // Constructor: initialize with a default value of the specified type
    template<typename T>
    explicit Attribute(T initialValue)
        : value_(std::move(initialValue))
        , valueType_(typeid(T))
    {}

    // Default constructor (empty attribute, needs to be set later)
    Attribute() = default;

    // Disable copy (optional, enable move or deep copy as needed)
    Attribute(const Attribute&) = delete;
    Attribute& operator=(const Attribute&) = delete;

    // Get current value (thread-safe)
    template<typename T>
    T get() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        if (!hasValue()) {
            throw std::runtime_error("Attribute has no value");
        }
        if (value_.type() != typeid(T)) {
            throw std::bad_any_cast();
        }
        return std::any_cast<T>(value_);
    }

    // Set new value (thread-safe, triggers callbacks)
    template<typename T>
    void set(T newValue) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        // Type check: first set or type must be consistent
        if (value_.has_value() && value_.type() != typeid(T)) {
            throw std::bad_any_cast();
        }
        std::any oldValue = value_;
        value_ = std::move(newValue);
        valueType_ = typeid(T);
        auto handlers = handlers_;
        lock.unlock(); // Release lock early to avoid deadlock (if callback calls this object)
        for (const auto& handler : handlers) {
            try {
                handler(oldValue, value_);
            } catch (const std::exception& e) {
                mylog(MyLogLevel::E, "[Attribute] Exception in value change handler: %s", e.what());
                assert(false && "Exception in value change handler");
            }
        }
    }

    // Add value change handler (thread-safe)
    void addHandler(ValueChangedCallback handler) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        handlers_.push_back(std::move(handler));
    }

    // Remove specific handler (thread-safe, based on address comparison)
    void removeHandler(const ValueChangedCallback& handler) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        handlers_.erase(
            std::remove_if(handlers_.begin(), handlers_.end(),
                [&handler](const ValueChangedCallback& h) {
                    return h.target_type() == handler.target_type() &&
                           h.template target<ValueChangedCallback>() == handler.template target<ValueChangedCallback>();
                }),
            handlers_.end()
        );
    }

    // Check if value exists
    bool hasValue() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return value_.has_value();
    }

    // Get current type (for debugging or reflection)
    std::type_index getType() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return value_.type();
    }

private:
    mutable std::shared_mutex mutex_;          // C++17 shared_mutex, read-write lock
    std::any value_;                           // Store value of any type
    std::type_index valueType_{typeid(void)};  // Record current type (redundant, for checking)
    std::vector<ValueChangedCallback> handlers_; // Callback list
};