#include "model/client_table.hpp"

#include <algorithm>
#include <iterator>

#include "format/units.hpp"

namespace model {

namespace {

constexpr std::size_t kHistoryLimit = 20;

void remember(ClientInfo& client, std::chrono::system_clock::time_point now, std::string what) {
    client.history.push_front(format::time_hms_ms(now).substr(0, 8) + " " + std::move(what));
    while (client.history.size() > kHistoryLimit) {
        client.history.pop_back();
    }
}

}  // namespace

ClientInfo& ClientTable::touch(const std::string& id, std::chrono::system_clock::time_point now) {
    auto [it, inserted] = entries_.try_emplace(id);
    auto& client = it->second;
    if (inserted) {
        client.id = id;
        client.first_seen = now;
    }
    client.last_seen = now;
    return client;
}

void ClientTable::apply(const parse::ClientEvent& event, std::chrono::system_clock::time_point now) {
    std::visit(
        [&](const auto& e) {
            using T = std::decay_t<decltype(e)>;
            auto& client = touch(e.id, now);
            if constexpr (std::is_same_v<T, parse::ClientConnected>) {
                client.address = e.address;
                client.protocol = e.protocol;
                client.clean_start = e.clean_start;
                client.keepalive = e.keepalive;
                client.username = e.username;
                client.connected = true;
                client.last_reason.clear();
                if (e.clean_start) {
                    client.subscriptions.clear();
                }
                remember(client, now, "connected from " + e.address);
            } else if constexpr (std::is_same_v<T, parse::ClientDisconnected>) {
                client.connected = false;
                client.last_reason = e.reason;
                remember(client, now, e.reason);
            } else if constexpr (std::is_same_v<T, parse::ClientSubscribed>) {
                auto& subs = client.subscriptions;
                auto it = std::find_if(subs.begin(), subs.end(), [&](const Subscription& s) { return s.filter == e.filter; });
                if (it == subs.end()) {
                    subs.push_back({e.filter, e.qos});
                    std::sort(subs.begin(), subs.end(),
                              [](const Subscription& a, const Subscription& b) { return a.filter < b.filter; });
                } else {
                    it->qos = e.qos;
                }
                remember(client, now, "subscribed " + e.filter);
            } else if constexpr (std::is_same_v<T, parse::ClientUnsubscribed>) {
                auto& subs = client.subscriptions;
                subs.erase(std::remove_if(subs.begin(), subs.end(),
                                          [&](const Subscription& s) { return s.filter == e.filter; }),
                           subs.end());
                remember(client, now, "unsubscribed " + e.filter);
            }
        },
        event);
}

const ClientInfo* ClientTable::at(std::size_t index) const {
    if (index >= entries_.size()) {
        return nullptr;
    }
    return &std::next(entries_.begin(), static_cast<long>(index))->second;
}

std::size_t ClientTable::connected_count() const {
    std::size_t count = 0;
    for (const auto& [id, client] : entries_) {
        count += client.connected;
    }
    return count;
}

}  // namespace model
