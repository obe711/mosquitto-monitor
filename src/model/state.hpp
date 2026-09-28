#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "model/client_table.hpp"
#include "model/message_store.hpp"
#include "model/metrics.hpp"
#include "model/topic_table.hpp"
#include "mqtt/message.hpp"

namespace model {

struct View {
    bool follow = true;
    std::optional<uint64_t> selected;   // message seq
    std::optional<uint64_t> paused_at;  // newest seq shown while paused
    bool show_detail = true;
    std::optional<bool> detail_hex;     // unset means decide from the payload
    bool show_help = false;
    int sidebar_width = 26;

    bool show_topics = false;
    std::size_t topic_cursor = 0;
    std::string filter;
    bool editing_filter = false;

    bool show_clients = false;
    std::size_t client_cursor = 0;
    std::size_t client_sub_cursor = 0;

    bool show_replay = false;
};

struct AppState {
    explicit AppState(std::size_t history) : messages(history) {}

    std::string broker;
    std::string client_id;
    mqtt::ConnectionState connection = mqtt::ConnectionState::Connecting;
    std::string connection_detail;
    std::string notice;
    uint64_t replays = 0;

    Metrics metrics;
    MessageStore messages;
    TopicTable topics;
    ClientTable clients;
    View view;
};

}  // namespace model
