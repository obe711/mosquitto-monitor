#include <iostream>
#include <mosquitto.h>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <thread>
#include <map>
#include <mutex>

const char *CLEAR_SCREEN = "\033[2J";
const char *CURSOR_HOME = "\033[H";
const char *BOLD = "\033[1m";
const char *DIM = "\033[2m";
const char *RESET = "\033[0m";
const char *GREEN = "\033[32m";
const char *YELLOW = "\033[33m";
const char *BLUE = "\033[34m";
const char *RED = "\033[31m";

std::map<std::string, std::string> metrics;
std::mutex metrics_mutex;

void update_display()
{
    std::cout << CLEAR_SCREEN << CURSOR_HOME;

    std::cout << BOLD << "MQTT Broker Monitor" << RESET << "\n\n";

    // Client Statistics
    std::cout << BOLD << "Client Statistics:" << RESET << "\n";
    std::cout << "Connected Clients: " << GREEN << metrics["$SYS/broker/clients/connected"] << RESET << "\n";
    std::cout << "Maximum Clients: " << metrics["$SYS/broker/clients/maximum"] << "\n";
    std::cout << "Total Clients: " << metrics["$SYS/broker/clients/total"] << "\n";
    std::cout << "Disconnected Clients: " << metrics["$SYS/broker/clients/disconnected"] << "\n\n";

    // Message Statistics
    std::cout << BOLD << "Message Statistics:" << RESET << "\n";
    std::cout << "Messages Sent: " << BLUE << metrics["$SYS/broker/messages/sent"] << RESET << "\n";
    std::cout << "Messages Received: " << metrics["$SYS/broker/messages/received"] << "\n";
    std::cout << "Messages Stored: " << metrics["$SYS/broker/messages/stored"] << "\n";
    std::cout << "Messages Inflight: " << YELLOW << metrics["$SYS/broker/messages/inflight"] << RESET << "\n";
    std::cout << "Retained Messages: " << metrics["$SYS/broker/retained messages/count"] << "\n";
    std::cout << "Messages Dropped: " << RED << metrics["$SYS/broker/publish/messages/dropped"] << RESET << "\n\n";

    // Network Statistics
    std::cout << BOLD << "Network Statistics:" << RESET << "\n";
    std::cout << "Bytes Sent: " << YELLOW << metrics["$SYS/broker/bytes/sent"] << RESET << "\n";
    std::cout << "Bytes Received: " << metrics["$SYS/broker/bytes/received"] << "\n";
    std::cout << "1min Bytes Sent: " << metrics["$SYS/broker/load/bytes/sent/1min"] << "\n";
    std::cout << "5min Bytes Sent: " << metrics["$SYS/broker/load/bytes/sent/5min"] << "\n";
    std::cout << "15min Bytes Sent: " << metrics["$SYS/broker/load/bytes/sent/15min"] << "\n\n";

    // System Statistics
    std::cout << BOLD << "System Statistics:" << RESET << "\n";
    std::cout << "Heap Current: " << metrics["$SYS/broker/heap/current"] << "\n";
    std::cout << "Heap Maximum: " << metrics["$SYS/broker/heap/maximum"] << "\n";
    std::cout << "Store Messages Count: " << metrics["$SYS/broker/store/messages/count"] << "\n";
    std::cout << "Store Messages Bytes: " << metrics["$SYS/broker/store/messages/bytes"] << "\n";
    std::cout << "Subscriptions Count: " << metrics["$SYS/broker/subscriptions/count"] << "\n";

    std::cout << "\n"
              << DIM << "Last Updated: " << metrics["timestamp"] << RESET << "\n";
    std::cout << "\nPress Ctrl+C to exit\n";
}

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    if (rc)
    {
        std::cerr << "Failed to connect to MQTT broker, error: " << mosquitto_connack_string(rc) << std::endl;
    }
    else
    {
        std::cout << "Connected to MQTT broker successfully" << std::endl;
    }
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    std::string topic = msg->topic;
    std::string payload = static_cast<char *>(msg->payload);

    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");

    // Update metrics with thread safety
    {
        std::lock_guard<std::mutex> lock(metrics_mutex);
        metrics[topic] = payload;
        metrics["timestamp"] = ss.str();
    }

    update_display();
}

int main()
{
    struct mosquitto *mosq;
    int rc;
    const char *clientid = "mosquitto_monitor";

    mosquitto_lib_init();

    mosq = mosquitto_new(clientid, true, nullptr);
    if (!mosq)
    {
        std::cerr << "Failed to create mosquitto instance" << std::endl;
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    rc = mosquitto_connect(mosq, "localhost", 1886, 60);
    if (rc != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "Failed to connect to MQTT broker: " << mosquitto_strerror(rc) << std::endl;
        return 1;
    }

    mosquitto_loop_start(mosq);

    // Subscribe to system topics
    std::vector<std::string> topics = {
        "$SYS/broker/bytes/sent",
        "$SYS/broker/bytes/received",
        "$SYS/broker/messages/sent",
        "$SYS/broker/messages/received",
        "$SYS/broker/messages/inflight",
        "$SYS/broker/retained messages/count",
        "$SYS/broker/load/bytes/sent/1min",
        "$SYS/broker/load/bytes/sent/5min",
        "$SYS/broker/load/bytes/sent/15min",
        "$SYS/broker/load/messages/sent/1min",
        "$SYS/broker/load/messages/received/1min",
        "$SYS/broker/clients/connected",
        "$SYS/broker/clients/disconnected",
        "$SYS/broker/clients/maximum",
        "$SYS/broker/clients/total",
        "$SYS/broker/load/connections/1min",
        "$SYS/broker/load/connections/5min",
        "$SYS/broker/messages/stored",
        "$SYS/broker/store/messages/count",
        "$SYS/broker/store/messages/bytes",
        "$SYS/broker/heap/current",
        "$SYS/broker/heap/maximum",
        "$SYS/broker/publish/messages/dropped",
        "$SYS/broker/publish/messages/received",
        "$SYS/broker/publish/messages/sent",
        "$SYS/broker/subscriptions/count"};

    for (const auto &topic : topics)
    {
        rc = mosquitto_subscribe(mosq, nullptr, topic.c_str(), 0);
        if (rc != MOSQ_ERR_SUCCESS)
        {
            std::cerr << "Failed to subscribe to topic " << topic << ": " << mosquitto_strerror(rc) << std::endl;
        }
    }

    // Initial display
    update_display();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Cleanup
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}
