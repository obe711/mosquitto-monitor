#include "ui/theme.hpp"

#include <functional>

namespace ui::theme {

ftxui::Color topic_color(const std::string& topic) {
    static const ftxui::Color palette[] = {
        ftxui::Color::Cyan,        ftxui::Color::Green,     ftxui::Color::Yellow,  ftxui::Color::Blue,
        ftxui::Color::Magenta,     ftxui::Color::CyanLight, ftxui::Color::GreenLight,
        ftxui::Color::YellowLight, ftxui::Color::BlueLight, ftxui::Color::MagentaLight,
    };
    constexpr std::size_t count = sizeof palette / sizeof palette[0];
    return palette[std::hash<std::string>{}(topic) % count];
}

}  // namespace ui::theme
