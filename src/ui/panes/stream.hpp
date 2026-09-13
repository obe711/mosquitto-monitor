#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "ftxui/dom/elements.hpp"
#include "model/state.hpp"

namespace ui::panes {

ftxui::Element stream(const model::AppState& state, const std::vector<std::size_t>& visible,
                      std::optional<std::size_t> selected, int rows_hint);

}
