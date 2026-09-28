#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

#include "mqtt/message.hpp"

namespace model {

struct TopicInfo {
    uint64_t count = 0;
    uint64_t bytes = 0;
    std::chrono::system_clock::time_point last_seen;
    bool enabled = true;
};

class TopicTable {
public:
    void record(const mqtt::Message& message);

    // Unknown topics count as enabled so nothing is hidden by surprise.
    bool enabled(const std::string& topic) const;

    const std::map<std::string, TopicInfo>& entries() const { return entries_; }
    std::size_t size() const { return entries_.size(); }
    std::size_t enabled_count() const;

    void toggle(std::size_t index);
    void set_all(bool enabled);
    void invert();

private:
    std::map<std::string, TopicInfo> entries_;
};

}  // namespace model
