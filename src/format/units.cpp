#include "format/units.hpp"

#include <cstdio>
#include <ctime>

namespace format {

std::string human_bytes(uint64_t bytes) {
    static const char* const units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    char buffer[32];
    if (unit == 0) {
        std::snprintf(buffer, sizeof buffer, "%llu B", static_cast<unsigned long long>(bytes));
    } else {
        std::snprintf(buffer, sizeof buffer, "%.1f %s", value, units[unit]);
    }
    return buffer;
}

std::string duration_short(std::chrono::seconds seconds) {
    auto s = seconds.count();
    char buffer[32];
    if (s < 60) {
        std::snprintf(buffer, sizeof buffer, "%llds", static_cast<long long>(s));
    } else if (s < 3600) {
        std::snprintf(buffer, sizeof buffer, "%lldm", static_cast<long long>(s / 60));
    } else if (s < 86400) {
        std::snprintf(buffer, sizeof buffer, "%lldh%02lldm", static_cast<long long>(s / 3600),
                      static_cast<long long>(s % 3600 / 60));
    } else {
        std::snprintf(buffer, sizeof buffer, "%lldd%02lldh", static_cast<long long>(s / 86400),
                      static_cast<long long>(s % 86400 / 3600));
    }
    return buffer;
}

std::string time_hms_ms(std::chrono::system_clock::time_point time) {
    auto seconds = std::chrono::system_clock::to_time_t(time);
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count() % 1000;
    std::tm local{};
    localtime_r(&seconds, &local);
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%02d:%02d:%02d.%03d", local.tm_hour, local.tm_min, local.tm_sec,
                  static_cast<int>(millis));
    return buffer;
}

std::string pad_right(std::string text, std::size_t width) {
    if (text.size() < width) {
        text.append(width - text.size(), ' ');
    }
    return text;
}

}  // namespace format
