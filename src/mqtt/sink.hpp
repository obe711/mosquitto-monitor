#pragma once

#include <string>

#include "mqtt/message.hpp"

namespace mqtt {

// Every method is invoked on libmosquitto's network thread.
struct Sink {
    virtual ~Sink() = default;
    virtual void on_message(Message message) = 0;
    virtual void on_metric(std::string topic, std::string value) = 0;
    virtual void on_connection(ConnectionState state, std::string detail) = 0;
    virtual void on_broker_log(std::string topic, std::string line) = 0;
};

}  // namespace mqtt
