#include "ui/panes/sidebar.hpp"

#include "ui/theme.hpp"

namespace ui::panes {

using namespace ftxui;

Element sidebar(const model::Metrics& metrics) {
    Elements rows;
    for (const auto& group : metrics.groups()) {
        rows.push_back(text(group.title) | bold | color(theme::title));
        for (const auto& metric : group.metrics) {
            rows.push_back(hbox({
                text(" " + metric.label) | color(theme::muted),
                filler(),
                text(metrics.display_value(metric)) | color(theme::value),
            }));
        }
        rows.push_back(text(""));
    }
    return vbox(rows) | yframe;
}

}  // namespace ui::panes
