#include "ui/panes/status_bar.hpp"

#include "format/units.hpp"
#include "model/stream_view.hpp"
#include "ui/theme.hpp"

namespace ui::panes {

using namespace ftxui;

namespace {

Element connection(const model::AppState& state) {
    switch (state.connection) {
        case mqtt::ConnectionState::Connected:
            return text("\xE2\x97\x8F connected") | color(theme::ok);
        case mqtt::ConnectionState::Connecting:
            return text("\xE2\x97\x8B connecting") | color(theme::warn);
        case mqtt::ConnectionState::Disconnected:
            break;
    }
    std::string label = "\xE2\x97\x8F disconnected";
    if (!state.connection_detail.empty()) {
        label += " (" + state.connection_detail + ")";
    }
    return text(label) | color(theme::error);
}

Element mode(const model::AppState& state) {
    if (state.view.paused_at) {
        auto newest = state.messages.size() ? state.messages.at(state.messages.size() - 1).seq : 0;
        auto pending = newest > *state.view.paused_at ? newest - *state.view.paused_at : 0;
        return text(" PAUSED +" + std::to_string(pending) + " ") | bold | bgcolor(theme::warn) | color(Color::Black);
    }
    if (state.view.follow) {
        return text(" FOLLOW ") | bold | bgcolor(theme::ok) | color(Color::Black);
    }
    return text(" SCROLL ") | bold | bgcolor(theme::muted) | color(Color::White);
}

}  // namespace

Element status_bar(const model::AppState& state) {
    const auto& store = state.messages;
    Elements parts = {
        connection(state),
        text(" " + state.broker + "  ") | color(theme::muted),
        text(std::to_string(store.total()) + " msgs  " + format::human_bytes(store.bytes()) + "  "),
        mode(state),
    };
    bool filtered = !state.view.filter.empty() || state.topics.enabled_count() != state.topics.size();
    if (filtered) {
        auto shown = model::visible_indices(state).size();
        parts.push_back(text("  " + std::to_string(shown) + "/" + std::to_string(store.size()) + " shown") |
                        color(theme::warn));
    }
    if (!state.notice.empty()) {
        parts.push_back(text("  " + state.notice) | color(theme::warn));
    }
    parts.push_back(filler());
    parts.push_back(text("q quit  ? help  t topics  c clients  / filter  r replay  p pause") |
                    color(theme::muted));
    return hbox(parts);
}

}  // namespace ui::panes
