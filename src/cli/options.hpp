#pragma once

#include <cstddef>
#include <string>

namespace cli {

struct Options {
    std::string host = "localhost";
    int port = 1886;
    std::string client_id = "mosquitto_monitor";
    std::string username;
    std::string password;
    int keepalive = 60;
    std::size_t history = 5000;
    bool mouse = true;
    bool help = false;
};

// Throws std::runtime_error with a one-line message on bad input.
Options parse(int argc, char** argv);

const char* usage();

}  // namespace cli
