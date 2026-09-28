#include "model/message_store.hpp"

namespace model {

void MessageStore::push(mqtt::Message message) {
    message.seq = next_seq_++;
    ++total_;
    bytes_ += message.payload.size();
    messages_.push_back(std::move(message));
    while (messages_.size() > capacity_) {
        messages_.pop_front();
    }
}

}  // namespace model
