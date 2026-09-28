#pragma once

#include <optional>
#include <string>
#include <variant>

namespace parse {

struct ClientConnected {
    std::string id;
    std::string address;
    int protocol = 0;
    bool clean_start = false;
    int keepalive = 0;
    std::optional<std::string> username;
};

struct ClientDisconnected {
    std::string id;
    std::string reason;
};

struct ClientSubscribed {
    std::string id;
    int qos = 0;
    std::string filter;
};

struct ClientUnsubscribed {
    std::string id;
    std::string filter;
};

using ClientEvent = std::variant<ClientConnected, ClientDisconnected, ClientSubscribed, ClientUnsubscribed>;

// Parses one message from a $SYS/broker/log/... topic. Handles the default
// "<timestamp>: " prefix as long as the timestamp itself contains no spaces.
std::optional<ClientEvent> broker_log(const std::string& topic, const std::string& payload);

const char* protocol_name(int protocol);

}  // namespace parse
