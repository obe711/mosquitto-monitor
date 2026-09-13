#include "model/stream_view.hpp"

#include <algorithm>

#include "model/topic_match.hpp"

namespace model {

namespace {

std::optional<std::size_t> index_of_seq(const MessageStore& store, uint64_t seq) {
    const auto& all = store.all();
    auto it = std::lower_bound(all.begin(), all.end(), seq,
                               [](const mqtt::Message& m, uint64_t s) { return m.seq < s; });
    if (it == all.end() || it->seq != seq) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(it - all.begin());
}

bool passes_filter(const AppState& state, const mqtt::Message& message) {
    if (!state.topics.enabled(message.topic)) {
        return false;
    }
    const auto& needle = state.view.filter;
    if (needle.empty()) {
        return true;
    }
    if (has_wildcard(needle)) {
        return topic_matches(needle, message.topic);
    }
    return message.topic.find(needle) != std::string::npos || message.payload.find(needle) != std::string::npos;
}

}  // namespace

std::vector<std::size_t> visible_indices(const AppState& state) {
    std::vector<std::size_t> out;
    const auto& store = state.messages;
    out.reserve(store.size());
    for (std::size_t i = 0; i < store.size(); ++i) {
        const auto& message = store.at(i);
        if (state.view.paused_at && message.seq > *state.view.paused_at) {
            break;
        }
        if (passes_filter(state, message)) {
            out.push_back(i);
        }
    }
    return out;
}

std::optional<std::size_t> selected_position(const AppState& state, const std::vector<std::size_t>& visible) {
    if (visible.empty()) {
        return std::nullopt;
    }
    if (state.view.follow || !state.view.selected) {
        return visible.size() - 1;
    }
    auto index = index_of_seq(state.messages, *state.view.selected);
    if (!index) {
        return visible.size() - 1;
    }
    auto it = std::lower_bound(visible.begin(), visible.end(), *index);
    if (it == visible.end()) {
        return visible.size() - 1;
    }
    return static_cast<std::size_t>(it - visible.begin());
}

void move_selection(AppState& state, int delta) {
    auto visible = visible_indices(state);
    auto position = selected_position(state, visible);
    if (!position) {
        return;
    }
    long target = static_cast<long>(*position) + delta;
    target = std::clamp(target, 0L, static_cast<long>(visible.size()) - 1);
    state.view.selected = state.messages.at(visible[static_cast<std::size_t>(target)]).seq;
    state.view.follow = static_cast<std::size_t>(target) + 1 == visible.size();
}

void select_first(AppState& state) {
    auto visible = visible_indices(state);
    if (visible.empty()) {
        return;
    }
    state.view.selected = state.messages.at(visible.front()).seq;
    state.view.follow = false;
}

void select_newest(AppState& state) {
    state.view.selected.reset();
    state.view.follow = true;
}

void toggle_pause(AppState& state) {
    if (state.view.paused_at) {
        state.view.paused_at.reset();
        return;
    }
    const auto& store = state.messages;
    state.view.paused_at = store.size() ? store.at(store.size() - 1).seq : 0;
}

const mqtt::Message* selected_message(const AppState& state) {
    auto visible = visible_indices(state);
    auto position = selected_position(state, visible);
    if (!position) {
        return nullptr;
    }
    return &state.messages.at(visible[*position]);
}

}  // namespace model
