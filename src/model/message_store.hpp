#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>

#include "mqtt/message.hpp"

namespace model {

class MessageStore {
public:
    explicit MessageStore(std::size_t capacity) : capacity_(capacity) {}

    // Assigns the sequence number; callers leave `seq` alone.
    void push(mqtt::Message message);

    std::size_t size() const { return messages_.size(); }
    const mqtt::Message& at(std::size_t index) const { return messages_[index]; }
    const std::deque<mqtt::Message>& all() const { return messages_; }

    uint64_t total() const { return total_; }
    uint64_t bytes() const { return bytes_; }

private:
    std::size_t capacity_;
    std::deque<mqtt::Message> messages_;
    uint64_t total_ = 0;
    uint64_t bytes_ = 0;
    uint64_t next_seq_ = 1;
};

}  // namespace model
