#pragma once
// Message queue implementation
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <optional>
#include <atomic>
#include "Message.h"

class MessageQueue {
private:
    mutable std::mutex mtx_;
    std::queue<Message> queue_;
    std::condition_variable cv_;
    std::atomic<bool> stopped_{false};

public:
    void send(Message msg) {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(std::move(msg));
        cv_.notify_one();
    }

    // Blocking receive — unblocked by stop()
    Message receive() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return !queue_.empty() || stopped_.load(); });
        if (stopped_ && queue_.empty()) {
            throw std::runtime_error("MessageQueue stopped");
        }
        auto msg = std::move(queue_.front());
        queue_.pop();
        return msg;
    }

    // Non-blocking timed receive — returns nullopt on timeout or stop
    std::optional<Message> tryReceiveFor(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mtx_);
        bool ready = cv_.wait_for(lock, timeout,
                                  [this] { return !queue_.empty() || stopped_.load(); });
        if (!ready || (stopped_ && queue_.empty())) return std::nullopt;
        auto msg = std::move(queue_.front());
        queue_.pop();
        return msg;
    }

    bool tryReceive(Message& out) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.empty()) return false;
        out = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    // Signal all waiting threads to unblock and return nullopt/throw
    void stop() {
        stopped_ = true;
        cv_.notify_all();
    }

    bool isStopped() const { return stopped_.load(); }
};