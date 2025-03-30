# MQTT Broker Monitor

A real-time monitoring tool for MQTT brokers that displays various system metrics in a clean, organized interface.

## Features

- Real-time monitoring of MQTT broker metrics
- Organized display of client, message, network, and system statistics
- Color-coded important metrics
- Automatic updates as new data arrives
- Thread-safe operation

## Prerequisites

- C++17 compatible compiler
- CMake 3.10 or higher
- libmosquitto development files

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libmosquitto-dev
```

### macOS
```bash
brew install cmake mosquitto
```

## Building from Source

1. Clone the repository:
```bash
git clone git@github.com:obe711/mosquitto-monitor.git
cd mosquitto-monitor
```

2. Create a build directory and build the project:
```bash
mkdir build
cd build
cmake ..
make
```

3. Install the application:
```bash
sudo make install
```

## Using Pre-built Packages

### Debian/Ubuntu
```bash
cd build
make
sudo cpack
sudo apt install ./mosquitto-monitor-0.0.1-Linux.deb
```

### Other Systems
Extract the tar.gz package and run the binary:
```bash
tar xzf mosquitto-monitor-0.0.1.tar.gz
cd mosquitto-monitor-0.0.1
./bin/mosquitto-monitor
```

## Usage

Run the monitor:
```bash
mosquitto-monitor
```

The monitor will connect to the MQTT broker on localhost:1886 and display real-time metrics. Press Ctrl+C to exit.

## Display Sections

1. **Client Statistics**
   - Connected Clients
   - Maximum Clients
   - Total Clients
   - Disconnected Clients

2. **Message Statistics**
   - Messages Sent
   - Messages Received
   - Messages Stored
   - Messages Inflight
   - Retained Messages
   - Messages Dropped

3. **Network Statistics**
   - Bytes Sent
   - Bytes Received
   - 1min/5min/15min Bytes Sent

4. **System Statistics**
   - Heap Current/Maximum
   - Store Messages Count/Bytes
   - Subscriptions Count
