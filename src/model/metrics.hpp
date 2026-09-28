#pragma once

#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace model {

enum class MetricKind { Raw, Bytes, Seconds };

struct Metric {
    Metric(std::string topic, std::string label, MetricKind kind = MetricKind::Raw)
        : topic(std::move(topic)), label(std::move(label)), kind(kind) {}

    std::string topic;
    std::string label;
    MetricKind kind;
    std::string value;
};

struct MetricGroup {
    std::string title;
    std::vector<Metric> metrics;
};

class Metrics {
public:
    Metrics();

    bool update(const std::string& topic, std::string value);
    const std::vector<MetricGroup>& groups() const { return groups_; }
    std::vector<std::string> topics() const;
    std::string display_value(const Metric& metric) const;

    std::chrono::system_clock::time_point last_update;

private:
    std::vector<MetricGroup> groups_;
};

}  // namespace model
