#include "ui/dialogs/replay.hpp"

#include "format/units.hpp"
#include "ui/theme.hpp"

namespace ui::dialogs {

using namespace ftxui;

Component replay(std::function<const mqtt::Message*()> selected) {
    return Renderer([selected = std::move(selected)] {
        const auto* message = selected();
        if (!message) {
            return text("nothing selected") | borderRounded;
        }
        auto field = [](const std::string& name, const std::string& value) {
            return hbox({text(name) | color(theme::muted) | size(WIDTH, EQUAL, 10), text(value)});
        };
        Elements rows = {
            text("Resend message") | bold,
            separatorLight(),
            field("topic", message->topic),
            field("qos", std::to_string(message->qos)),
            field("retain", message->retain ? "yes" : "no"),
            field("size", format::human_bytes(message->payload.size())),
        };
        if (message->props.content_type) {
            rows.push_back(field("type", *message->props.content_type));
        }
        for (const auto& [name, value] : message->props.user) {
            rows.push_back(field(name, value));
        }
        if (message->retain) {
            rows.push_back(text("This replaces the broker's retained value for the topic.") | color(theme::warn));
        }
        rows.push_back(text(""));
        rows.push_back(hbox({text("Enter") | color(theme::title), text(" send    "), text("Esc") | color(theme::title),
                             text(" cancel")}));
        return vbox(rows) | borderRounded | bgcolor(Color::Black) | size(WIDTH, LESS_THAN, 80);
    });
}

}  // namespace ui::dialogs
