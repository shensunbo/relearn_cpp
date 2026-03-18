#pragma once
// Message queue implementation
#include <queue>
#include <mutex>
#include <condition_variable>
#include "Message.h"

class MessageQueue {
private:
    mutable std::mutex mtx_;
    std::queue<Message> queue_;
    std::condition_variable cv_;

public:
    void send(Message msg) {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(std::move(msg));
        cv_.notify_one();
    }

    Message receive() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return !queue_.empty(); });
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
};