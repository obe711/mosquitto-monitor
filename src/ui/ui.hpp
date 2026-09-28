#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "ftxui/component/app.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/dom/elements.hpp"
#include "model/state.hpp"
#include "mqtt/sink.hpp"
#include "parse/broker_log.hpp"
#include "ui/panes/clients.hpp"
#include "ui/panes/topics.hpp"

namespace ui {

class Ui : public mqtt::Sink {
public:
    using Publisher = std::function<std::optional<std::string>(const mqtt::Message&)>;

    Ui(model::AppState state, bool mouse);

    void set_publisher(Publisher publisher) { publisher_ = std::move(publisher); }
    void run();

    // mqtt::Sink, called from the network thread.
    void on_message(mqtt::Message message) override;
    void on_metric(std::string topic, std::string value) override;
    void on_connection(mqtt::ConnectionState state, std::string detail) override;
    void on_broker_log(std::string topic, std::string line) override;

private:
    struct Pending {
        std::vector<mqtt::Message> messages;
        std::vector<std::pair<std::string, std::string>> metrics;
        std::optional<std::pair<mqtt::ConnectionState, std::string>> connection;
        std::vector<parse::ClientEvent> client_events;
    };

    void schedule_drain();
    void drain();
    bool on_event(ftxui::Event event);
    bool on_mouse(const ftxui::Mouse& mouse);
    bool on_topics_key(const ftxui::Event& event);
    bool on_clients_key(const ftxui::Event& event);
    bool on_filter_key(const ftxui::Event& event);
    void move_topic_cursor(int delta);
    void move_client_cursor(int delta);
    void replay_selected();
    ftxui::Element render_main();
    ftxui::Element render_stream_column();

    ftxui::App app_;
    model::AppState state_;
    Publisher publisher_;
    panes::TopicsLayout topics_layout_;
    panes::ClientsLayout clients_layout_;

    std::mutex mutex_;
    Pending pending_;
    bool drain_scheduled_ = false;
};

}  // namespace ui
