#include "ui/panes/detail.hpp"

#include <sstream>

#include "format/payload.hpp"
#include "format/units.hpp"
#include "ui/theme.hpp"

namespace ui::panes {

using namespace ftxui;

namespace {

Element header(const mqtt::Message& message, bool hex) {
    Elements parts = {
        text(message.topic) | bold | color(theme::topic_color(message.topic)),
        text("  qos " + std::to_string(message.qos)) | color(theme::muted),
    };
    if (message.retain) {
        parts.push_back(text("  retained") | color(theme::warn));
    }
    if (message.origin == mqtt::Origin::Replay) {
        parts.push_back(text("  replayed") | color(theme::warn));
    }
    parts.push_back(text("  " + format::human_bytes(message.payload.size())) | color(theme::muted));
    if (message.props.content_type) {
        parts.push_back(text("  " + *message.props.content_type) | color(theme::muted));
    }
    parts.push_back(filler());
    parts.push_back(text(hex ? "[hex]" : "[text]") | color(theme::title));
    return hbox(parts);
}

Elements property_lines(const mqtt::Properties& props) {
    Elements lines;
    auto add = [&](const std::string& name, const std::string& value) {
        lines.push_back(hbox({text(name + ": ") | color(theme::muted), text(value)}));
    };
    if (props.response_topic) {
        add("response topic", *props.response_topic);
    }
    if (props.correlation_data) {
        add("correlation", format::hex_preview(*props.correlation_data, 16));
    }
    if (props.message_expiry) {
        add("expiry", std::to_string(*props.message_expiry) + " s");
    }
    for (const auto& [name, value] : props.user) {
        add(name, value);
    }
    return lines;
}

Elements text_lines(const std::string& payload) {
    Elements lines;
    std::istringstream in(format::sanitize(payload));
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(text(line));
    }
    if (lines.empty()) {
        lines.push_back(text("(empty)") | color(theme::muted));
    }
    return lines;
}

Elements hex_lines(const std::string& payload, int width) {
    Elements lines;
    std::size_t per_row = width < 76 ? 8 : 16;
    for (auto& row : format::hex_dump(payload, per_row)) {
        lines.push_back(text(row) | color(theme::value));
    }
    if (lines.empty()) {
        lines.push_back(text("(empty)") | color(theme::muted));
    }
    return lines;
}

}  // namespace

Element detail(const mqtt::Message& message, bool hex, int width) {
    Elements rows = {header(message, hex)};
    for (auto& line : property_lines(message.props)) {
        rows.push_back(line);
    }
    rows.push_back(separatorLight() | color(theme::muted));
    Elements body = hex ? hex_lines(message.payload, width) : text_lines(message.payload);
    rows.push_back(vbox(body) | vscroll_indicator | yframe | flex);
    return vbox(rows);
}

}  // namespace ui::panes
