#include "ui/ui.hpp"

#include <algorithm>

#include "format/payload.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/mouse.hpp"
#include "ftxui/screen/terminal.hpp"
#include "model/stream_view.hpp"
#include "ui/dialogs/help.hpp"
#include "ui/dialogs/replay.hpp"
#include "ui/panes/clients.hpp"
#include "ui/panes/detail.hpp"
#include "ui/panes/sidebar.hpp"
#include "ui/panes/status_bar.hpp"
#include "ui/panes/stream.hpp"
#include "ui/panes/topics.hpp"
#include "ui/theme.hpp"

namespace ui {

using namespace ftxui;

Ui::Ui(model::AppState state, bool mouse) : app_(App::Fullscreen()), state_(std::move(state)) {
    app_.TrackMouse(mouse);
    app_.ForceHandleCtrlC(false);
}

void Ui::run() {
    auto sidebar = Renderer([this] { return panes::sidebar(state_.metrics); });
    auto main = Renderer([this] { return render_main(); });
    auto split = ResizableSplitLeft(sidebar, main, &state_.view.sidebar_width);
    auto screen = Renderer(split, [this, split] {
        return vbox({split->Render() | flex, separatorLight() | color(theme::muted), panes::status_bar(state_)});
    });
    auto with_help = Modal(screen, dialogs::help(), &state_.view.show_help);
    auto replay = dialogs::replay([this] { return model::selected_message(state_); });
    auto root = Modal(with_help, replay, &state_.view.show_replay) |
                CatchEvent([this](Event event) { return on_event(std::move(event)); });
    app_.Loop(root);
}

void Ui::on_message(mqtt::Message message) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_.messages.push_back(std::move(message));
    schedule_drain();
}

void Ui::on_metric(std::string topic, std::string value) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_.metrics.emplace_back(std::move(topic), std::move(value));
    schedule_drain();
}

void Ui::on_connection(mqtt::ConnectionState state, std::string detail) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_.connection = {state, std::move(detail)};
    schedule_drain();
}

void Ui::on_broker_log(std::string topic, std::string line) {
    auto event = parse::broker_log(topic, line);
    if (!event) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    pending_.client_events.push_back(std::move(*event));
    schedule_drain();
}

// Caller holds mutex_. One event per batch keeps bursts to one redraw; a plain
// closure would run but not invalidate the frame, so the drain rides on an event.
void Ui::schedule_drain() {
    if (drain_scheduled_) {
        return;
    }
    drain_scheduled_ = true;
    app_.PostEvent(Event::Custom);
}

void Ui::drain() {
    Pending batch;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        batch = std::move(pending_);
        pending_ = {};
        drain_scheduled_ = false;
    }
    for (auto& message : batch.messages) {
        state_.topics.record(message);
        state_.messages.push(std::move(message));
    }
    for (auto& [topic, value] : batch.metrics) {
        state_.metrics.update(topic, std::move(value));
    }
    if (batch.connection) {
        state_.connection = batch.connection->first;
        state_.connection_detail = std::move(batch.connection->second);
    }
    auto now = std::chrono::system_clock::now();
    for (const auto& event : batch.client_events) {
        auto own = std::visit([&](const auto& e) { return e.id == state_.client_id; }, event);
        if (!own) {
            state_.clients.apply(event, now);
        }
    }
}

Element Ui::render_main() {
    const auto& view = state_.view;
    if (!view.show_topics && !view.show_clients) {
        return render_stream_column();
    }
    Element side = view.show_clients ? panes::clients(state_, clients_layout_) | size(WIDTH, EQUAL, 44)
                                     : panes::topics(state_, topics_layout_) | size(WIDTH, EQUAL, 34);
    return hbox({
        render_stream_column() | flex,
        separatorLight() | color(theme::muted),
        side,
    });
}

Element Ui::render_stream_column() {
    const auto& view = state_.view;
    auto visible = model::visible_indices(state_);
    auto selected = model::selected_position(state_, visible);
    auto dims = Terminal::Size();
    int detail_rows = view.show_detail ? std::max(dims.dimy / 3, 6) : 0;
    int stream_rows = std::max(dims.dimy - detail_rows - 2, 4);

    Elements rows;
    if (view.editing_filter || !view.filter.empty()) {
        rows.push_back(hbox({
            text("/ ") | color(theme::title),
            text(view.filter),
            text(view.editing_filter ? "â" : "") | color(theme::title),  // ▏
            text(view.editing_filter ? "  Enter apply  Esc clear" : "  / edit  Esc clear") | color(theme::muted),
        }));
    }
    rows.push_back(panes::stream(state_, visible, selected, stream_rows));
    if (view.show_detail && selected) {
        const auto& message = state_.messages.at(visible[*selected]);
        bool hex = view.detail_hex.value_or(!format::is_text(message));
        int side = view.show_clients ? 46 : view.show_topics ? 36 : 1;
        int width = dims.dimx - view.sidebar_width - side;
        rows.push_back(separatorLight() | color(theme::muted));
        rows.push_back(panes::detail(message, hex, width) | size(HEIGHT, EQUAL, detail_rows));
    }
    return vbox(rows);
}

bool Ui::on_event(Event event) {
    if (event == Event::Custom) {
        drain();
        return true;
    }

    auto& view = state_.view;
    state_.notice.clear();

    if (event == Event::CtrlC || event == Event::Character('q')) {
        app_.Exit();
        return true;
    }
    if (view.show_help) {
        view.show_help = event.is_mouse();
        return true;
    }
    if (view.show_replay) {
        if (event.is_mouse()) {
            return true;
        }
        view.show_replay = false;
        if (event == Event::Return) {
            replay_selected();
        }
        return true;
    }
    if (event.is_mouse()) {
        return on_mouse(event.mouse());
    }
    if (view.editing_filter) {
        return on_filter_key(event);
    }
    if (event == Event::Escape) {
        if (!view.filter.empty()) {
            view.filter.clear();
        } else {
            view.show_topics = view.show_clients = false;
        }
        return true;
    }
    if (view.show_topics && on_topics_key(event)) {
        return true;
    }
    if (view.show_clients && on_clients_key(event)) {
        return true;
    }

    int page = std::max(Terminal::Size().dimy / 2, 1);
    if (event == Event::ArrowUp) {
        model::move_selection(state_, -1);
    } else if (event == Event::ArrowDown) {
        model::move_selection(state_, 1);
    } else if (event == Event::PageUp) {
        model::move_selection(state_, -page);
    } else if (event == Event::PageDown) {
        model::move_selection(state_, page);
    } else if (event == Event::Home) {
        model::select_first(state_);
    } else if (event == Event::End) {
        model::select_newest(state_);
    } else if (event == Event::Character('?')) {
        view.show_help = true;
    } else if (event == Event::Character('t')) {
        view.show_topics = !view.show_topics;
        view.show_clients = false;
    } else if (event == Event::Character('c')) {
        view.show_clients = !view.show_clients;
        view.show_topics = false;
    } else if (event == Event::Character('/')) {
        view.editing_filter = true;
    } else if (event == Event::Character('p')) {
        model::toggle_pause(state_);
    } else if (event == Event::Character('d') || event == Event::Return) {
        view.show_detail = !view.show_detail;
    } else if (event == Event::Character('r')) {
        view.show_replay = model::selected_message(state_) != nullptr;
    } else if (event == Event::Character('R')) {
        replay_selected();
    } else if (event == Event::Character('x')) {
        if (const auto* message = model::selected_message(state_)) {
            view.detail_hex = !view.detail_hex.value_or(!format::is_text(*message));
        }
    } else {
        return false;
    }
    return true;
}

bool Ui::on_mouse(const Mouse& mouse) {
    auto& view = state_.view;
    bool over_topics = view.show_topics && topics_layout_.pane.Contain(mouse.x, mouse.y);
    bool over_clients = view.show_clients && clients_layout_.pane.Contain(mouse.x, mouse.y);

    if (mouse.button == Mouse::WheelUp || mouse.button == Mouse::WheelDown) {
        int step = mouse.button == Mouse::WheelUp ? -1 : 1;
        if (over_topics) {
            move_topic_cursor(step);
        } else if (over_clients) {
            move_client_cursor(step);
        } else {
            model::move_selection(state_, 3 * step);
        }
        return true;
    }
    if (over_clients && mouse.button == Mouse::Left && mouse.motion == Mouse::Pressed) {
        const auto& rows = clients_layout_.rows;
        for (std::size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].Contain(mouse.x, mouse.y)) {
                view.client_cursor = i;
                view.client_sub_cursor = 0;
                break;
            }
        }
        return true;
    }
    if (over_topics && mouse.button == Mouse::Left && mouse.motion == Mouse::Pressed) {
        const auto& rows = topics_layout_.rows;
        for (std::size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].Contain(mouse.x, mouse.y)) {
                view.topic_cursor = i;
                state_.topics.toggle(i);
                break;
            }
        }
        return true;
    }
    return false;
}

bool Ui::on_topics_key(const Event& event) {
    auto& view = state_.view;
    if (event == Event::ArrowUp) {
        move_topic_cursor(-1);
    } else if (event == Event::ArrowDown) {
        move_topic_cursor(1);
    } else if (event == Event::Character(' ') || event == Event::Return) {
        state_.topics.toggle(view.topic_cursor);
    } else if (event == Event::Character('a')) {
        state_.topics.set_all(true);
    } else if (event == Event::Character('n')) {
        state_.topics.set_all(false);
    } else if (event == Event::Character('i')) {
        state_.topics.invert();
    } else {
        return false;
    }
    return true;
}

// Enter cycles the stream filter through the selected client's subscriptions.
bool Ui::on_clients_key(const Event& event) {
    auto& view = state_.view;
    if (event == Event::ArrowUp) {
        move_client_cursor(-1);
    } else if (event == Event::ArrowDown) {
        move_client_cursor(1);
    } else if (event == Event::Return) {
        const auto* client = state_.clients.at(view.client_cursor);
        if (!client || client->subscriptions.empty()) {
            return true;
        }
        view.client_sub_cursor %= client->subscriptions.size();
        view.filter = client->subscriptions[view.client_sub_cursor].filter;
        view.client_sub_cursor = (view.client_sub_cursor + 1) % client->subscriptions.size();
    } else {
        return false;
    }
    return true;
}

bool Ui::on_filter_key(const Event& event) {
    auto& view = state_.view;
    if (event == Event::Return) {
        view.editing_filter = false;
    } else if (event == Event::Escape) {
        view.editing_filter = false;
        view.filter.clear();
    } else if (event == Event::Backspace) {
        while (!view.filter.empty()) {
            auto byte = static_cast<unsigned char>(view.filter.back());
            view.filter.pop_back();
            if ((byte & 0xC0) != 0x80) {
                break;
            }
        }
    } else if (event.is_character()) {
        view.filter += event.character();
    }
    return true;
}

namespace {

void move_cursor(std::size_t& cursor, int delta, std::size_t count) {
    if (count == 0) {
        cursor = 0;
        return;
    }
    long target = std::clamp(static_cast<long>(cursor) + delta, 0L, static_cast<long>(count) - 1);
    cursor = static_cast<std::size_t>(target);
}

}  // namespace

void Ui::replay_selected() {
    const auto* original = model::selected_message(state_);
    if (!original || !publisher_) {
        return;
    }
    mqtt::Message copy = *original;
    copy.time = std::chrono::system_clock::now();
    copy.origin = mqtt::Origin::Replay;
    if (auto error = publisher_(copy)) {
        state_.notice = "replay failed: " + *error;
        return;
    }
    ++state_.replays;
    state_.notice = "replayed " + copy.topic + " (" + std::to_string(state_.replays) + ")";
    state_.topics.record(copy);
    state_.messages.push(std::move(copy));
}

void Ui::move_topic_cursor(int delta) { move_cursor(state_.view.topic_cursor, delta, state_.topics.size()); }

void Ui::move_client_cursor(int delta) {
    move_cursor(state_.view.client_cursor, delta, state_.clients.size());
    state_.view.client_sub_cursor = 0;
}

}  // namespace ui
