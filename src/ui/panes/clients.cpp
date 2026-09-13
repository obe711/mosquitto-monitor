#include "ui/panes/clients.hpp"

#include "format/units.hpp"
#include "parse/broker_log.hpp"
#include "ui/theme.hpp"

namespace ui::panes {

using namespace ftxui;

namespace {

Element client_row(const model::ClientInfo& client, bool selected) {
    auto row = hbox({
        text(client.connected ? "\xE2\x97\x8F " : "\xE2\x97\x8B ") | color(client.connected ? theme::ok : theme::muted),
        text(client.id) | color(client.connected ? theme::value : theme::muted) | flex,
        text(" " + std::to_string(client.subscriptions.size())) | color(theme::muted),
    });
    return selected ? row | bgcolor(theme::selection) | focus : row;
}

Element client_detail(const model::ClientInfo& client, std::size_t sub_cursor) {
    auto now = std::chrono::system_clock::now();
    auto field = [](const std::string& name, const std::string& value) {
        return hbox({text(name) | color(theme::muted) | size(WIDTH, EQUAL, 10), text(value)});
    };
    Elements rows = {
        text(client.id) | bold | color(theme::title),
        field("state", client.connected ? "connected" : "offline, " + client.last_reason),
        field("address", client.address.empty() ? "-" : client.address),
        field("protocol", parse::protocol_name(client.protocol) + std::string(client.clean_start ? ", clean start" : "")),
        field("keepalive", client.keepalive ? std::to_string(client.keepalive) + " s" : "-"),
        field("user", client.username.value_or("-")),
        field("seen", format::duration_short(std::chrono::duration_cast<std::chrono::seconds>(now - client.last_seen)) + " ago"),
        text(""),
        text("Subscriptions") | bold,
    };
    if (client.subscriptions.empty()) {
        rows.push_back(text(" none seen") | color(theme::muted));
    }
    for (std::size_t i = 0; i < client.subscriptions.size(); ++i) {
        const auto& sub = client.subscriptions[i];
        auto row = hbox({
            text(" " + sub.filter) | color(theme::topic_color(sub.filter)) | flex,
            text("qos " + std::to_string(sub.qos)) | color(theme::muted),
        });
        rows.push_back(i == sub_cursor ? row | bold : row);
    }
    rows.push_back(text(""));
    rows.push_back(text("Recent") | bold);
    for (const auto& entry : client.history) {
        rows.push_back(text(" " + entry) | color(theme::muted));
    }
    return vbox(rows) | yframe;
}

}  // namespace

Element clients(const model::AppState& state, ClientsLayout& layout) {
    const auto& table = state.clients;
    layout.rows.assign(table.size(), Box{});

    Elements rows;
    std::size_t index = 0;
    for (const auto& [id, client] : table.entries()) {
        rows.push_back(client_row(client, index == state.view.client_cursor) | reflect(layout.rows[index]));
        ++index;
    }
    if (rows.empty()) {
        rows = {
            text("no client events yet") | color(theme::muted),
            text(""),
            paragraph("The broker must publish its log to $SYS/broker/log/#. "
                      "Install broker/monitor-log.conf into /etc/mosquitto/conf.d and restart it.") |
                color(theme::muted),
        };
    }

    auto header = hbox({
        text("Clients ") | bold | color(theme::title),
        text(std::to_string(table.connected_count()) + " online / " + std::to_string(table.size()) + " seen") |
            color(theme::muted),
    });
    Elements column = {header, vbox(rows) | vscroll_indicator | yframe | size(HEIGHT, LESS_THAN, 12)};
    if (const auto* client = table.at(state.view.client_cursor)) {
        column.push_back(separatorLight() | color(theme::muted));
        column.push_back(client_detail(*client, state.view.client_sub_cursor) | flex);
    }
    column.push_back(text("Enter filter by subscription") | color(theme::muted));
    return vbox(column) | reflect(layout.pane);
}

}  // namespace ui::panes
