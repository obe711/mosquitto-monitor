#include <cassert>
#include <string>

#include "model/client_table.hpp"
#include "model/topic_match.hpp"
#include "parse/broker_log.hpp"

int main() {
    using namespace parse;
    const std::string notice = "$SYS/broker/log/N";

    auto connected = broker_log(notice, "1694600000: New client connected from 10.0.0.5:51234 as sensor-1 (p2, c1, k60, u'bob').");
    assert(connected);
    auto& c = std::get<ClientConnected>(*connected);
    assert(c.id == "sensor-1");
    assert(c.address == "10.0.0.5:51234");
    assert(c.protocol == 2);
    assert(c.clean_start);
    assert(c.keepalive == 60);
    assert(c.username && *c.username == "bob");

    auto anon = broker_log(notice, "New client connected from ::1:40000 as a b (p5, c0, k30).");
    assert(anon);
    auto& a = std::get<ClientConnected>(*anon);
    assert(a.id == "a b");
    assert(a.address == "::1:40000");
    assert(a.protocol == 5);
    assert(!a.clean_start);
    assert(!a.username);

    auto closed = broker_log(notice, "1694600001: Client sensor-1 closed its connection.");
    assert(closed);
    assert(std::get<ClientDisconnected>(*closed).id == "sensor-1");
    assert(std::get<ClientDisconnected>(*closed).reason == "closed its connection");

    auto timeout = broker_log(notice, "Client sensor-1 has exceeded timeout, disconnecting.");
    assert(timeout && std::get<ClientDisconnected>(*timeout).reason == "has exceeded timeout, disconnecting");

    auto proto = broker_log(notice, "Client sensor-1 disconnected due to protocol error.");
    assert(proto && std::get<ClientDisconnected>(*proto).reason == "disconnected due to protocol error");

    auto bad = broker_log(notice, "Bad socket read/write on client sensor-1: Connection reset by peer");
    assert(bad && std::get<ClientDisconnected>(*bad).reason == "socket error: Connection reset by peer");

    auto replaced = broker_log("$SYS/broker/log/E", "Client sensor-1 already connected, closing old connection.");
    assert(replaced && std::get<ClientDisconnected>(*replaced).id == "sensor-1");

    auto sub = broker_log("$SYS/broker/log/M/subscribe", "1694600002: sensor-1 1 sensors/#");
    assert(sub);
    assert(std::get<ClientSubscribed>(*sub).id == "sensor-1");
    assert(std::get<ClientSubscribed>(*sub).qos == 1);
    assert(std::get<ClientSubscribed>(*sub).filter == "sensors/#");

    auto unsub = broker_log("$SYS/broker/log/M/unsubscribe", "sensor-1 sensors/#");
    assert(unsub && std::get<ClientUnsubscribed>(*unsub).filter == "sensors/#");

    assert(!broker_log(notice, "mosquitto version 2.0.11 running"));
    assert(!broker_log("$SYS/broker/log/W", "anything"));

    model::ClientTable table;
    auto now = std::chrono::system_clock::now();
    table.apply(*connected, now);
    table.apply(*sub, now);
    table.apply(*broker_log("$SYS/broker/log/M/subscribe", "sensor-1 0 alerts"), now);
    assert(table.size() == 1);
    assert(table.connected_count() == 1);
    const auto* info = table.at(0);
    assert(info->subscriptions.size() == 2);
    assert(info->subscriptions[0].filter == "alerts");
    table.apply(*unsub, now);
    assert(info->subscriptions.size() == 1);
    table.apply(*closed, now);
    assert(!info->connected);
    assert(info->last_reason == "closed its connection");
    assert(info->history.size() == 5);
    assert(table.at(1) == nullptr);

    using model::topic_matches;
    assert(topic_matches("sensors/#", "sensors/kitchen/temp"));
    assert(topic_matches("sensors/#", "sensors"));
    assert(topic_matches("#", "a/b"));
    assert(topic_matches("sensors/+/temp", "sensors/hall/temp"));
    assert(!topic_matches("sensors/+/temp", "sensors/hall/x/temp"));
    assert(!topic_matches("sensors/+", "sensors/hall/temp"));
    assert(topic_matches("a/b", "a/b"));
    assert(!topic_matches("a/b", "a/b/c"));
    assert(!topic_matches("#", "$SYS/broker/uptime"));
    assert(topic_matches("$SYS/#", "$SYS/broker/uptime"));
    assert(model::has_wildcard("a/+") && !model::has_wildcard("a/b"));
    return 0;
}
