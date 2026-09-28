#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "parse/broker_log.hpp"

namespace model {

struct Subscription {
    std::string filter;
    int qos = 0;
};

struct ClientInfo {
    std::string id;
    std::string address;
    int protocol = 0;
    bool clean_start = false;
    int keepalive = 0;
    std::optional<std::string> username;

    bool connected = false;
    std::chrono::system_clock::time_point first_seen;
    std::chrono::system_clock::time_point last_seen;
    std::string last_reason;
    std::vector<Subscription> subscriptions;
    std::deque<std::string> history;
};

class ClientTable {
public:
    void apply(const parse::ClientEvent& event, std::chrono::system_clock::time_point now);

    const std::map<std::string, ClientInfo>& entries() const { return entries_; }
    const ClientInfo* at(std::size_t index) const;
    std::size_t size() const { return entries_.size(); }
    std::size_t connected_count() const;

private:
    ClientInfo& touch(const std::string& id, std::chrono::system_clock::time_point now);

    std::map<std::string, ClientInfo> entries_;
};

}  // namespace model
