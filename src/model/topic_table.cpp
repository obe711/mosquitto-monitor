#include "model/topic_table.hpp"

#include <iterator>

namespace model {

void TopicTable::record(const mqtt::Message& message) {
    auto& info = entries_[message.topic];
    ++info.count;
    info.bytes += message.payload.size();
    info.last_seen = message.time;
}

bool TopicTable::enabled(const std::string& topic) const {
    auto it = entries_.find(topic);
    return it == entries_.end() || it->second.enabled;
}

std::size_t TopicTable::enabled_count() const {
    std::size_t count = 0;
    for (const auto& [topic, info] : entries_) {
        count += info.enabled;
    }
    return count;
}

void TopicTable::toggle(std::size_t index) {
    if (index >= entries_.size()) {
        return;
    }
    auto& info = std::next(entries_.begin(), static_cast<long>(index))->second;
    info.enabled = !info.enabled;
}

void TopicTable::set_all(bool enabled) {
    for (auto& [topic, info] : entries_) {
        info.enabled = enabled;
    }
}

void TopicTable::invert() {
    for (auto& [topic, info] : entries_) {
        info.enabled = !info.enabled;
    }
}

}  // namespace model
