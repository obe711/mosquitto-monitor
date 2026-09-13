#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "model/state.hpp"

namespace model {

// Indices into the store that the stream pane should show, oldest first.
std::vector<std::size_t> visible_indices(const AppState& state);

// Position of the selected message inside `visible`; the newest entry while following.
std::optional<std::size_t> selected_position(const AppState& state, const std::vector<std::size_t>& visible);

void move_selection(AppState& state, int delta);
void select_first(AppState& state);
void select_newest(AppState& state);
void toggle_pause(AppState& state);

const mqtt::Message* selected_message(const AppState& state);

}  // namespace model
