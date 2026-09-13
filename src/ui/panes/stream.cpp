#include "ui/panes/stream.hpp"

#include <algorithm>

#include "format/payload.hpp"
#include "format/units.hpp"
#include "ui/theme.hpp"

namespace ui::panes {

using namespace ftxui;

namespace {

constexpr std::size_t kMaxTopicWidth = 40;
constexpr std::size_t kPreviewGlyphs = 160;

Element marker(const mqtt::Message& message) {
    if (message.origin == mqtt::Origin::Replay) {
        return text("\xE2\x86\xBB ") | color(theme::warn);  // ↻
    }
    if (message.retain) {
        return text("R ") | color(theme::warn);
    }
    return text("  ");
}

Element preview(const mqtt::Message& message) {
    if (format::is_text(message)) {
        return text(format::preview(message.payload, kPreviewGlyphs));
    }
    return hbox({
        text("\xE2\x96\xA3 " + std::to_string(message.payload.size()) + " B  ") | color(theme::binary),  // ▣
        text(format::hex_preview(message.payload, 24)) | color(theme::muted),
    });
}

Element line(const mqtt::Message& message, std::size_t topic_width, bool selected) {
    auto row = hbox({
        text(format::time_hms_ms(message.time)) | color(theme::muted),
        text(" "),
        marker(message),
        text(format::pad_right(message.topic, topic_width)) | color(theme::topic_color(message.topic)),
        text("  "),
        preview(message),
    });
    if (selected) {
        row = row | bgcolor(theme::selection) | focus;
    }
    return row;
}

}  // namespace

Element stream(const model::AppState& state, const std::vector<std::size_t>& visible,
               std::optional<std::size_t> selected, int rows_hint) {
    if (visible.empty()) {
        return text("waiting for messages") | color(theme::muted) | center;
    }

    std::size_t anchor = selected.value_or(visible.size() - 1);
    std::size_t window = static_cast<std::size_t>(std::max(rows_hint, 1));
    std::size_t begin = anchor > window ? anchor - window : 0;
    std::size_t end = std::min(visible.size(), anchor + window + 1);

    std::size_t topic_width = 0;
    for (std::size_t k = begin; k < end; ++k) {
        topic_width = std::max(topic_width, state.messages.at(visible[k]).topic.size());
    }
    topic_width = std::min(topic_width, kMaxTopicWidth);

    Elements lines;
    lines.reserve(end - begin);
    for (std::size_t k = begin; k < end; ++k) {
        lines.push_back(line(state.messages.at(visible[k]), topic_width, selected && k == *selected));
    }
    return vbox(lines) | vscroll_indicator | yframe | flex;
}

}  // namespace ui::panes
