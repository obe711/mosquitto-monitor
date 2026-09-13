#include "parse/broker_log.hpp"

#include <cstdlib>
#include <string_view>

namespace parse {

namespace {

using std::string_view;

bool starts_with(string_view text, string_view prefix) { return text.substr(0, prefix.size()) == prefix; }

bool ends_with(string_view text, string_view suffix) {
    return text.size() >= suffix.size() && text.substr(text.size() - suffix.size()) == suffix;
}

string_view strip_timestamp(string_view line) {
    auto colon = line.find(": ");
    if (colon == string_view::npos || line.substr(0, colon).find(' ') != string_view::npos) {
        return line;
    }
    return line.substr(colon + 2);
}

string_view strip_period(string_view text) {
    return ends_with(text, ".") ? text.substr(0, text.size() - 1) : text;
}

// "New client connected from 10.0.0.5:51234 as sensor-1 (p2, c1, k60, u'bob')."
std::optional<ClientEvent> parse_connected(string_view line) {
    auto as = line.find(" as ");
    auto paren = line.rfind(" (p");
    if (as == string_view::npos || paren == string_view::npos || paren <= as) {
        return std::nullopt;
    }
    ClientConnected event;
    event.address = std::string(line.substr(0, as));
    event.id = std::string(line.substr(as + 4, paren - as - 4));

    string_view flags = line.substr(paren + 2);
    event.protocol = std::atoi(flags.data() + 1);
    if (auto c = flags.find(", c"); c != string_view::npos) {
        event.clean_start = flags[c + 3] == '1';
    }
    if (auto k = flags.find(", k"); k != string_view::npos) {
        event.keepalive = std::atoi(flags.data() + k + 3);
    }
    if (auto u = flags.find(", u'"); u != string_view::npos) {
        auto end = flags.find('\'', u + 4);
        if (end != string_view::npos) {
            event.username = std::string(flags.substr(u + 4, end - u - 4));
        }
    }
    return event;
}

// "Client sensor-1 closed its connection." and the other loop.c variants.
std::optional<ClientEvent> parse_disconnected(string_view line) {
    static const string_view markers[] = {
        " disconnected", " closed its connection", " has exceeded timeout", " been disconnected by",
    };
    auto split = string_view::npos;
    for (auto marker : markers) {
        auto at = line.find(marker);
        if (at != string_view::npos && at < split) {
            split = at;
        }
    }
    if (split == string_view::npos) {
        return std::nullopt;
    }
    return ClientDisconnected{std::string(line.substr(0, split)), std::string(strip_period(line.substr(split + 1)))};
}

std::optional<ClientEvent> parse_notice(string_view line) {
    if (starts_with(line, "New client connected from ")) {
        return parse_connected(line.substr(26));
    }
    if (starts_with(line, "New bridge connected from ")) {
        return parse_connected(line.substr(26));
    }
    if (starts_with(line, "Client ")) {
        return parse_disconnected(line.substr(7));
    }
    if (starts_with(line, "Bad socket read/write on client ")) {
        auto rest = line.substr(32);
        auto colon = rest.rfind(": ");
        if (colon == string_view::npos) {
            return std::nullopt;
        }
        return ClientDisconnected{std::string(rest.substr(0, colon)), "socket error: " + std::string(rest.substr(colon + 2))};
    }
    return std::nullopt;
}

std::optional<ClientEvent> parse_error(string_view line) {
    constexpr string_view suffix = " already connected, closing old connection.";
    if (starts_with(line, "Client ") && ends_with(line, suffix)) {
        return ClientDisconnected{std::string(line.substr(7, line.size() - 7 - suffix.size())),
                                  "replaced by a new connection"};
    }
    return std::nullopt;
}

// "<id> <qos> <filter>"
std::optional<ClientEvent> parse_subscribe(string_view line) {
    auto first = line.find(' ');
    if (first == string_view::npos) {
        return std::nullopt;
    }
    auto second = line.find(' ', first + 1);
    if (second == string_view::npos) {
        return std::nullopt;
    }
    return ClientSubscribed{std::string(line.substr(0, first)), std::atoi(line.data() + first + 1),
                            std::string(line.substr(second + 1))};
}

// "<id> <filter>"
std::optional<ClientEvent> parse_unsubscribe(string_view line) {
    auto first = line.find(' ');
    if (first == string_view::npos) {
        return std::nullopt;
    }
    return ClientUnsubscribed{std::string(line.substr(0, first)), std::string(line.substr(first + 1))};
}

}  // namespace

std::optional<ClientEvent> broker_log(const std::string& topic, const std::string& payload) {
    auto line = strip_timestamp(payload);
    if (topic == "$SYS/broker/log/N") {
        return parse_notice(line);
    }
    if (topic == "$SYS/broker/log/E") {
        return parse_error(line);
    }
    if (topic == "$SYS/broker/log/M/subscribe") {
        return parse_subscribe(line);
    }
    if (topic == "$SYS/broker/log/M/unsubscribe") {
        return parse_unsubscribe(line);
    }
    return std::nullopt;
}

const char* protocol_name(int protocol) {
    switch (protocol) {
        case 1:
            return "MQTT 3.1";
        case 2:
            return "MQTT 3.1.1";
        case 3:
            return "MQTT 3.1.1 (TLS)";
        case 5:
            return "MQTT 5";
        default:
            return "MQTT";
    }
}

}  // namespace parse
