#pragma once

#include <vector>

#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/box.hpp"
#include "model/state.hpp"

namespace ui::panes {

// Screen boxes captured during render so mouse clicks can be mapped back to rows.
struct TopicsLayout {
    ftxui::Box pane;
    std::vector<ftxui::Box> rows;
};

ftxui::Element topics(const model::AppState& state, TopicsLayout& layout);

}  // namespace ui::panes
