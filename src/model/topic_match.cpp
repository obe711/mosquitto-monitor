#include "model/topic_match.hpp"

namespace model {

bool topic_matches(std::string_view filter, std::string_view topic) {
    if (topic.substr(0, 1) == "$" && filter.substr(0, 1) != "$") {
        return false;
    }
    while (true) {
        auto filter_slash = filter.find('/');
        auto pattern = filter.substr(0, filter_slash);
        if (pattern == "#") {
            return true;
        }
        auto topic_slash = topic.find('/');
        auto level = topic.substr(0, topic_slash);
        if (pattern != "+" && pattern != level) {
            return false;
        }
        bool filter_done = filter_slash == std::string_view::npos;
        bool topic_done = topic_slash == std::string_view::npos;
        if (filter_done && topic_done) {
            return true;
        }
        if (topic_done) {
            return filter.substr(filter_slash + 1) == "#";  // "a/#" also matches "a"
        }
        if (filter_done) {
            return false;
        }
        filter.remove_prefix(filter_slash + 1);
        topic.remove_prefix(topic_slash + 1);
    }
}

bool has_wildcard(std::string_view filter) { return filter.find_first_of("+#") != std::string_view::npos; }

}  // namespace model
