#include "ui/dialogs/help.hpp"

#include "ui/theme.hpp"

namespace ui::dialogs {

using namespace ftxui;

Component help() {
    return Renderer([] {
        auto key = [](const std::string& keys, const std::string& what) {
            return hbox({text(keys) | color(theme::title) | size(WIDTH, EQUAL, 14), text(what)});
        };
        return vbox({
                   text("Keys") | bold,
                   separatorLight(),
                   key("q  Ctrl+C", "quit"),
                   key("?", "show this help, any key closes it"),
                   key("Up/Down", "select older/newer message"),
                   key("PgUp/PgDn", "move by a page"),
                   key("Home/End", "oldest message / follow newest"),
                   key("p", "pause the stream (store keeps filling)"),
                   key("d  Enter", "show or hide the detail pane"),
                   key("x", "toggle hex and text in the detail pane"),
                   key("t", "topic list: Up/Down, Space toggle, a/n/i all/none/invert"),
                   key("c", "client list: Up/Down select, Enter filters by its subscriptions"),
                   key("/", "filter: substring of topic or payload, or an MQTT wildcard filter"),
                   key("Esc", "clear the filter, then close the side pane"),
                   key("r  R", "resend the selected message as-is (R skips the confirmation)"),
                   key("mouse", "wheel scrolls, drag the divider to resize"),
               }) |
               borderRounded | bgcolor(Color::Black);
    });
}

}  // namespace ui::dialogs
