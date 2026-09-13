#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mqtt {

struct Properties {
    std::optional<uint8_t> payload_format;
    std::optional<uint32_t> message_expiry;
    std::optional<std::string> content_type;
    std::optional<std::string> response_topic;
    std::optional<std::string> correlation_data;
    std::vector<std::pair<std::string, std::string>> user;
};

enum class Origin { Broker, Replay };

struct Message {
    uint64_t seq = 0;  // assigned by the store
    std::chrono::system_clock::time_point time;
    std::string topic;
    std::string payload;
    int qos = 0;
    bool retain = false;
    Properties props;
    Origin origin = Origin::Broker;
};

enum class ConnectionState { Connecting, Connected, Disconnected };

}  // namespace mqtt
