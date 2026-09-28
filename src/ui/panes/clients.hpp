#pragma once

#include <vector>

#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/box.hpp"
#include "model/state.hpp"

namespace ui::panes {

struct ClientsLayout {
    ftxui::Box pane;
    std::vector<ftxui::Box> rows;
};

ftxui::Element clients(const model::AppState& state, ClientsLayout& layout);

}  // namespace ui::panes
