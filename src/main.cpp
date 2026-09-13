#include <iostream>

#include "cli/options.hpp"
#include "model/state.hpp"
#include "mqtt/client.hpp"
#include "ui/ui.hpp"

namespace {

std::vector<mqtt::Subscription> subscriptions(const model::Metrics& metrics) {
    std::vector<mqtt::Subscription> out;
    for (const auto& topic : metrics.topics()) {
        out.push_back({topic, false});
    }
    for (const char* log : {"N", "E", "M/subscribe", "M/unsubscribe"}) {
        out.push_back({std::string("$SYS/broker/log/") + log, false});
    }
    out.push_back({"#", true});
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    cli::Options options;
    try {
        options = cli::parse(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n\n" << cli::usage();
        return 2;
    }
    if (options.help) {
        std::cout << cli::usage();
        return 0;
    }

    model::AppState state(options.history);
    state.broker = options.host + ":" + std::to_string(options.port);
    state.client_id = options.client_id;
    auto subs = subscriptions(state.metrics);

    ui::Ui ui(std::move(state), options.mouse);

    mqtt::Endpoint endpoint{options.host,     options.port,     options.client_id,
                            options.username, options.password, options.keepalive};
    mqtt::Client client(std::move(endpoint), std::move(subs), ui);
    if (auto error = client.connect()) {
        std::cerr << "cannot connect to " << options.host << ":" << options.port << ": " << *error << "\n";
        return 1;
    }

    ui.set_publisher([&client](const mqtt::Message& message) { return client.publish(message); });

    ui.run();
    return 0;
}
