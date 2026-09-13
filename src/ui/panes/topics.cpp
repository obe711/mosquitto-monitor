#include "ui/panes/topics.hpp"

#include "format/units.hpp"
#include "ui/theme.hpp"

namespace ui::panes {

using namespace ftxui;

Element topics(const model::AppState& state, TopicsLayout& layout) {
    const auto& table = state.topics;
    layout.rows.assign(table.size(), Box{});

    Elements rows;
    std::size_t index = 0;
    for (const auto& [topic, info] : table.entries()) {
        auto row = hbox({
            text(info.enabled ? "\xE2\x96\xA3 " : "\xE2\x98\x90 ") | color(info.enabled ? theme::ok : theme::muted),  // ▣ ☐
            text(topic) | color(info.enabled ? theme::topic_color(topic) : theme::muted) | flex,
            text(" " + std::to_string(info.count)) | color(theme::muted),
        });
        if (index == state.view.topic_cursor) {
            row = row | bgcolor(theme::selection) | focus;
        }
        rows.push_back(row | reflect(layout.rows[index]));
        ++index;
    }
    if (rows.empty()) {
        rows.push_back(text("no topics yet") | color(theme::muted));
    }

    auto header = hbox({
        text("Topics ") | bold | color(theme::title),
        text(std::to_string(table.enabled_count()) + "/" + std::to_string(table.size())) | color(theme::muted),
    });
    auto hints = text("space toggle  a all  n none  i invert") | color(theme::muted);

    return vbox({
               header,
               vbox(rows) | vscroll_indicator | yframe | flex,
               hints,
           }) |
           reflect(layout.pane);
}

}  // namespace ui::panes
