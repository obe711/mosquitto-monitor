#pragma once

#include <string_view>

namespace model {

// MQTT topic filter matching with '+' and '#' wildcards.
bool topic_matches(std::string_view filter, std::string_view topic);

bool has_wildcard(std::string_view filter);

}  // namespace model
