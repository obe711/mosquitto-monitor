#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "mqtt/message.hpp"
#include "mqtt/sink.hpp"

namespace mqtt {

struct Endpoint {
    std::string host;
    int port = 1883;
    std::string client_id;
    std::string username;
    std::string password;
    int keepalive = 60;
};

struct Subscription {
    std::string filter;
    bool no_local = false;
};

class Client {
public:
    Client(Endpoint endpoint, std::vector<Subscription> subscriptions, Sink& sink);
    ~Client();
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    // Starts the network thread; returns an error message when the connection cannot even begin.
    std::optional<std::string> connect();

    // Returns an error message on failure. Safe to call from any thread.
    std::optional<std::string> publish(const Message& message);

    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
};

}  // namespace mqtt
