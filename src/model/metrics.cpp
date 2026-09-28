#include "model/metrics.hpp"

#include <cstdlib>
#include <string_view>

#include "format/units.hpp"

namespace model {

Metrics::Metrics()
    : groups_{
          {"Broker",
           {{"$SYS/broker/version", "Version"},
            {"$SYS/broker/uptime", "Uptime", MetricKind::Seconds}}},
          {"Clients",
           {{"$SYS/broker/clients/connected", "Connected"},
            {"$SYS/broker/clients/maximum", "Maximum"},
            {"$SYS/broker/clients/total", "Total"},
            {"$SYS/broker/clients/disconnected", "Disconnected"}}},
          {"Messages",
           {{"$SYS/broker/messages/sent", "Sent"},
            {"$SYS/broker/messages/received", "Received"},
            {"$SYS/broker/messages/stored", "Stored"},
            {"$SYS/broker/messages/inflight", "Inflight"},
            {"$SYS/broker/retained messages/count", "Retained"},
            {"$SYS/broker/publish/messages/dropped", "Dropped"}}},
          {"Network",
           {{"$SYS/broker/bytes/sent", "Sent", MetricKind::Bytes},
            {"$SYS/broker/bytes/received", "Received", MetricKind::Bytes},
            {"$SYS/broker/load/bytes/sent/1min", "Sent 1m", MetricKind::Bytes},
            {"$SYS/broker/load/bytes/sent/5min", "Sent 5m", MetricKind::Bytes},
            {"$SYS/broker/load/bytes/sent/15min", "Sent 15m", MetricKind::Bytes}}},
          {"System",
           {{"$SYS/broker/heap/current", "Heap", MetricKind::Bytes},
            {"$SYS/broker/heap/maximum", "Heap max", MetricKind::Bytes},
            {"$SYS/broker/store/messages/count", "Stored msgs"},
            {"$SYS/broker/store/messages/bytes", "Stored bytes", MetricKind::Bytes},
            {"$SYS/broker/subscriptions/count", "Subscriptions"}}},
      } {}

bool Metrics::update(const std::string& topic, std::string value) {
    constexpr std::string_view version_prefix = "mosquitto version ";
    if (value.compare(0, version_prefix.size(), version_prefix) == 0) {
        value.erase(0, version_prefix.size());
    }
    for (auto& group : groups_) {
        for (auto& metric : group.metrics) {
            if (metric.topic == topic) {
                metric.value = std::move(value);
                last_update = std::chrono::system_clock::now();
                return true;
            }
        }
    }
    return false;
}

std::vector<std::string> Metrics::topics() const {
    std::vector<std::string> out;
    for (const auto& group : groups_) {
        for (const auto& metric : group.metrics) {
            out.push_back(metric.topic);
        }
    }
    return out;
}

std::string Metrics::display_value(const Metric& metric) const {
    if (metric.value.empty()) {
        return "-";
    }
    switch (metric.kind) {
        case MetricKind::Bytes:
            return format::human_bytes(std::strtod(metric.value.c_str(), nullptr));
        case MetricKind::Seconds:
            return format::duration_short(std::chrono::seconds(std::strtoll(metric.value.c_str(), nullptr, 10)));
        case MetricKind::Raw:
            break;
    }
    return metric.value;
}

}  // namespace model
