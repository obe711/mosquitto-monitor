#include "cli/options.hpp"

#include <stdexcept>
#include <string_view>

namespace cli {

namespace {

const char* kUsage =
    "Usage: mosquitto-monitor [options]\n"
    "\n"
    "  --host <name>        broker host (default localhost)\n"
    "  --port <n>           broker port (default 1886)\n"
    "  --client-id <id>     MQTT client id (default mosquitto_monitor)\n"
    "  --username <name>    broker username\n"
    "  --password <secret>  broker password\n"
    "  --keepalive <sec>    MQTT keepalive (default 60)\n"
    "  --history <n>        messages kept in memory (default 5000)\n"
    "  --no-mouse           disable mouse support\n"
    "  --help               show this text\n";

int to_int(std::string_view flag, const std::string& value, int min, int max) {
    int n = 0;
    try {
        n = std::stoi(value);
    } catch (const std::exception&) {
        throw std::runtime_error(std::string(flag) + ": expected a number, got '" + value + "'");
    }
    if (n < min || n > max) {
        throw std::runtime_error(std::string(flag) + ": " + value + " is out of range");
    }
    return n;
}

}  // namespace

Options parse(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string_view flag = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error(std::string(flag) + " requires a value");
            }
            return argv[++i];
        };

        if (flag == "--help" || flag == "-h") {
            options.help = true;
        } else if (flag == "--no-mouse") {
            options.mouse = false;
        } else if (flag == "--host") {
            options.host = value();
        } else if (flag == "--port") {
            options.port = to_int(flag, value(), 1, 65535);
        } else if (flag == "--client-id") {
            options.client_id = value();
        } else if (flag == "--username") {
            options.username = value();
        } else if (flag == "--password") {
            options.password = value();
        } else if (flag == "--keepalive") {
            options.keepalive = to_int(flag, value(), 5, 65535);
        } else if (flag == "--history") {
            options.history = static_cast<std::size_t>(to_int(flag, value(), 10, 1000000));
        } else {
            throw std::runtime_error("unknown option '" + std::string(flag) + "'");
        }
    }
    return options;
}

const char* usage() { return kUsage; }

}  // namespace cli
