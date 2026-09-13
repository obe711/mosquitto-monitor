#pragma once

#include "ftxui/dom/elements.hpp"
#include "mqtt/message.hpp"

namespace ui::panes {

ftxui::Element detail(const mqtt::Message& message, bool hex, int width);

}
