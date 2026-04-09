#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace dcompframe {

template <typename T>
class Observable {
public:
    using Listener = std::function<void(const T&)>;

    explicit Observable(T value = {}) : value_(std::move(value)) {}

    int bind(Listener listener) {
        const int id = ++next_id_;
        listeners_[id] = std::move(listener);
        return id;
    }

    void unbind(int id) {
        listeners_.erase(id);
    }

    void set(T value) {
        value_ = std::move(value);
        for (const auto& [_, listener] : listeners_) {
            listener(value_);
        }
    }

    [[nodiscard]] const T& get() const {
        return value_;
    }

private:
    T value_;
    int next_id_ = 0;
    std::unordered_map<int, Listener> listeners_;
};

class BindingContext {
public:
    Observable<std::string> title {""};
    Observable<std::string> body {""};
    Observable<bool> enabled {true};
};

}  // namespace dcompframe
