#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "mqtt/message.hpp"

namespace format {

bool is_text(std::string_view bytes);
bool is_text(const mqtt::Message& message);

// Control characters and invalid UTF-8 become a visible placeholder; newlines are kept.
std::string sanitize(std::string_view bytes);

// Single line, at most max_glyphs long, with an ellipsis when cut.
std::string preview(std::string_view bytes, std::size_t max_glyphs);

std::vector<std::string> hex_dump(std::string_view bytes, std::size_t bytes_per_row);
std::string hex_preview(std::string_view bytes, std::size_t max_bytes);

}  // namespace format
