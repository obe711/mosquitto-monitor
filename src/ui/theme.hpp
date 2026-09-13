#pragma once

#include <string>

#include "ftxui/screen/color.hpp"

namespace ui::theme {

inline const ftxui::Color title = ftxui::Color::Cyan;
inline const ftxui::Color value = ftxui::Color::White;
inline const ftxui::Color ok = ftxui::Color::Green;
inline const ftxui::Color warn = ftxui::Color::Yellow;
inline const ftxui::Color error = ftxui::Color::Red;
inline const ftxui::Color muted = ftxui::Color::GrayDark;
inline const ftxui::Color selection = ftxui::Color::GrayDark;
inline const ftxui::Color binary = ftxui::Color::Magenta;

ftxui::Color topic_color(const std::string& topic);

}  // namespace ui::theme
