#include <cassert>

#include "model/state.hpp"
#include "model/stream_view.hpp"

namespace {

mqtt::Message make(const char* topic, const char* payload) {
    mqtt::Message m;
    m.topic = topic;
    m.payload = payload;
    return m;
}

}  // namespace

int main() {
    model::AppState state(3);
    for (uint64_t seq = 1; seq <= 5; ++seq) {
        auto m = make(seq % 2 ? "sensors/a" : "sensors/b", seq == 4 ? "needle" : "hay");
        state.topics.record(m);
        state.messages.push(std::move(m));
    }
    assert(state.messages.size() == 3);
    assert(state.messages.at(0).seq == 3);
    assert(state.topics.size() == 2);

    auto visible = model::visible_indices(state);
    assert(visible.size() == 3);
    assert(*model::selected_position(state, visible) == 2);

    model::move_selection(state, -1);
    assert(!state.view.follow);
    assert(*state.view.selected == 4);
    model::move_selection(state, 5);
    assert(state.view.follow);

    state.topics.toggle(1);  // sensors/b
    visible = model::visible_indices(state);
    assert(visible.size() == 2);
    assert(state.messages.at(visible[0]).seq == 3);
    assert(state.messages.at(visible[1]).seq == 5);
    assert(state.topics.enabled_count() == 1);

    state.topics.set_all(true);
    state.view.filter = "needle";
    visible = model::visible_indices(state);
    assert(visible.size() == 1);
    assert(state.messages.at(visible[0]).seq == 4);

    state.view.filter = "sensors/a";
    visible = model::visible_indices(state);
    assert(visible.size() == 2);

    state.view.filter.clear();
    model::toggle_pause(state);
    state.messages.push(make("sensors/a", "late"));
    assert(model::visible_indices(state).size() == 2);  // seq 6 hidden while paused
    model::toggle_pause(state);
    assert(model::visible_indices(state).size() == 3);  // capacity 3: seq 4,5,6
    assert(state.messages.at(0).seq == 4);
    return 0;
}
