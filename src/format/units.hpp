#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace format {

std::string human_bytes(uint64_t bytes);
std::string duration_short(std::chrono::seconds seconds);
std::string time_hms_ms(std::chrono::system_clock::time_point time);
std::string pad_right(std::string text, std::size_t width);

}  // namespace format
