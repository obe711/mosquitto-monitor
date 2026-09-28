#pragma once

#include <functional>

#include "ftxui/component/component.hpp"
#include "mqtt/message.hpp"

namespace ui::dialogs {

ftxui::Component replay(std::function<const mqtt::Message*()> selected);

}
