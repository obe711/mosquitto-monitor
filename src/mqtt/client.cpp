#include "mqtt/client.hpp"

#include <mosquitto.h>
#include <mqtt_protocol.h>

#include <cstdlib>
#include <string_view>

namespace mqtt {

struct Client::Impl {
    Endpoint endpoint;
    std::vector<Subscription> subscriptions;
    Sink& sink;
    mosquitto* mosq = nullptr;
};

namespace {

Client::Impl& impl_of(void* obj) { return *static_cast<Client::Impl*>(obj); }

bool starts_with(std::string_view text, std::string_view prefix) {
    return text.substr(0, prefix.size()) == prefix;
}

std::optional<std::string> read_string(const mosquitto_property* props, int id) {
    char* value = nullptr;
    if (!mosquitto_property_read_string(props, id, &value, false)) {
        return std::nullopt;
    }
    std::string out(value);
    std::free(value);
    return out;
}

std::optional<std::string> read_binary(const mosquitto_property* props, int id) {
    void* value = nullptr;
    uint16_t length = 0;
    if (!mosquitto_property_read_binary(props, id, &value, &length, false)) {
        return std::nullopt;
    }
    std::string out(static_cast<const char*>(value), length);
    std::free(value);
    return out;
}

Properties read_properties(const mosquitto_property* props) {
    Properties out;
    if (!props) {
        return out;
    }
    uint8_t format = 0;
    if (mosquitto_property_read_byte(props, MQTT_PROP_PAYLOAD_FORMAT_INDICATOR, &format, false)) {
        out.payload_format = format;
    }
    uint32_t expiry = 0;
    if (mosquitto_property_read_int32(props, MQTT_PROP_MESSAGE_EXPIRY_INTERVAL, &expiry, false)) {
        out.message_expiry = expiry;
    }
    out.content_type = read_string(props, MQTT_PROP_CONTENT_TYPE);
    out.response_topic = read_string(props, MQTT_PROP_RESPONSE_TOPIC);
    out.correlation_data = read_binary(props, MQTT_PROP_CORRELATION_DATA);

    char* name = nullptr;
    char* value = nullptr;
    const mosquitto_property* pair =
        mosquitto_property_read_string_pair(props, MQTT_PROP_USER_PROPERTY, &name, &value, false);
    while (pair) {
        out.user.emplace_back(name, value);
        std::free(name);
        std::free(value);
        pair = mosquitto_property_read_string_pair(pair, MQTT_PROP_USER_PROPERTY, &name, &value, true);
    }
    return out;
}

mosquitto_property* build_properties(const Properties& in) {
    mosquitto_property* props = nullptr;
    if (in.payload_format) {
        mosquitto_property_add_byte(&props, MQTT_PROP_PAYLOAD_FORMAT_INDICATOR, *in.payload_format);
    }
    if (in.message_expiry) {
        mosquitto_property_add_int32(&props, MQTT_PROP_MESSAGE_EXPIRY_INTERVAL, *in.message_expiry);
    }
    if (in.content_type) {
        mosquitto_property_add_string(&props, MQTT_PROP_CONTENT_TYPE, in.content_type->c_str());
    }
    if (in.response_topic) {
        mosquitto_property_add_string(&props, MQTT_PROP_RESPONSE_TOPIC, in.response_topic->c_str());
    }
    if (in.correlation_data) {
        mosquitto_property_add_binary(&props, MQTT_PROP_CORRELATION_DATA, in.correlation_data->data(),
                                      static_cast<uint16_t>(in.correlation_data->size()));
    }
    for (const auto& [name, value] : in.user) {
        mosquitto_property_add_string_pair(&props, MQTT_PROP_USER_PROPERTY, name.c_str(), value.c_str());
    }
    return props;
}

std::string describe(int code) {
    return code >= 128 ? mosquitto_reason_string(code) : mosquitto_strerror(code);
}

void on_connect(mosquitto* mosq, void* obj, int reason, int, const mosquitto_property*) {
    auto& impl = impl_of(obj);
    if (reason != 0) {
        impl.sink.on_connection(ConnectionState::Disconnected, mosquitto_reason_string(reason));
        return;
    }
    // QoS 2 and retain-as-published keep the publisher's QoS and retain flag, so a
    // replay can reproduce them.
    for (const auto& sub : impl.subscriptions) {
        int options = MQTT_SUB_OPT_RETAIN_AS_PUBLISHED | (sub.no_local ? MQTT_SUB_OPT_NO_LOCAL : 0);
        mosquitto_subscribe_v5(mosq, nullptr, sub.filter.c_str(), 2, options, nullptr);
    }
    impl.sink.on_connection(ConnectionState::Connected, "");
}

void on_disconnect(mosquitto*, void* obj, int reason, const mosquitto_property*) {
    impl_of(obj).sink.on_connection(ConnectionState::Disconnected, reason == 0 ? "" : describe(reason));
}

void on_message(mosquitto*, void* obj, const mosquitto_message* raw, const mosquitto_property* props) {
    auto& impl = impl_of(obj);
    std::string topic = raw->topic ? raw->topic : "";
    std::string payload(static_cast<const char*>(raw->payload), raw->payload ? raw->payloadlen : 0);

    if (starts_with(topic, "$SYS/broker/log/")) {
        impl.sink.on_broker_log(std::move(topic), std::move(payload));
        return;
    }
    if (starts_with(topic, "$SYS/")) {
        impl.sink.on_metric(std::move(topic), std::move(payload));
        return;
    }

    Message message;
    message.time = std::chrono::system_clock::now();
    message.topic = std::move(topic);
    message.payload = std::move(payload);
    message.qos = raw->qos;
    message.retain = raw->retain;
    message.props = read_properties(props);
    impl.sink.on_message(std::move(message));
}

}  // namespace

Client::Client(Endpoint endpoint, std::vector<Subscription> subscriptions, Sink& sink)
    : impl_(new Impl{std::move(endpoint), std::move(subscriptions), sink}) {
    mosquitto_lib_init();
}

Client::~Client() {
    if (impl_->mosq) {
        mosquitto_disconnect(impl_->mosq);
        mosquitto_loop_stop(impl_->mosq, false);
        mosquitto_destroy(impl_->mosq);
    }
    mosquitto_lib_cleanup();
}

std::optional<std::string> Client::connect() {
    auto& impl = *impl_;
    impl.mosq = mosquitto_new(impl.endpoint.client_id.c_str(), true, &impl);
    if (!impl.mosq) {
        return "could not create mosquitto client";
    }
    mosquitto_int_option(impl.mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
    if (!impl.endpoint.username.empty()) {
        mosquitto_username_pw_set(impl.mosq, impl.endpoint.username.c_str(),
                                  impl.endpoint.password.empty() ? nullptr : impl.endpoint.password.c_str());
    }
    mosquitto_reconnect_delay_set(impl.mosq, 1, 30, true);
    mosquitto_connect_v5_callback_set(impl.mosq, on_connect);
    mosquitto_disconnect_v5_callback_set(impl.mosq, on_disconnect);
    mosquitto_message_v5_callback_set(impl.mosq, on_message);

    int rc = mosquitto_connect_async(impl.mosq, impl.endpoint.host.c_str(), impl.endpoint.port,
                                     impl.endpoint.keepalive);
    if (rc != MOSQ_ERR_SUCCESS) {
        return mosquitto_strerror(rc);
    }
    rc = mosquitto_loop_start(impl.mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        return mosquitto_strerror(rc);
    }
    return std::nullopt;
}

std::optional<std::string> Client::publish(const Message& message) {
    mosquitto_property* props = build_properties(message.props);
    int rc = mosquitto_publish_v5(impl_->mosq, nullptr, message.topic.c_str(),
                                  static_cast<int>(message.payload.size()), message.payload.data(),
                                  message.qos, message.retain, props);
    mosquitto_property_free_all(&props);
    if (rc != MOSQ_ERR_SUCCESS) {
        return mosquitto_strerror(rc);
    }
    return std::nullopt;
}

}  // namespace mqtt
