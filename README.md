# mosquitto-monitor

A full-terminal debugging tool for Mosquitto MQTT brokers. It watches every message that
passes through the broker, shows text payloads as a chat-style log and binary payloads as a
hex dump, lists the topics and clients it has seen, and can resend any captured message.

```
┌ sidebar ──┬ stream ────────────────────────────────────────────────┬ topics / clients ┐
│ Broker    │ 12:03:41.120  sensors/kitchen/temp  {"t":21.4,"h":48}  │ ▣ sensors/…   12 │
│ Clients   │ 12:03:41.311  devices/cam2/frame    ▣ 1024 B  ff d8 …  │ ☐ devices/…    3 │
│ Messages  ├ detail ────────────────────────────────────────────────┤                  │
│ Network   │ devices/cam2/frame  qos 0  1.0 KiB  image/jpeg   [hex] │                  │
│ System    │ 00000000  ff d8 ff e0 00 10 4a 46  49 46 00 01 …       │                  │
├───────────┴────────────────────────────────────────────────────────┴──────────────────┤
│ ● connected localhost:1886  1234 msgs  12.3 KiB  FOLLOW      q quit  ? help  t c / r p │
└───────────────────────────────────────────────────────────────────────────────────────┘
```

## Features

- Live stream of all broker traffic (`#` subscription) with follow and scroll modes, pause,
  and a fixed-size history (default 5000 messages).
- Text or hex detail view, chosen automatically from the MQTT v5 payload format indicator,
  content type, or the bytes themselves. `x` toggles.
- Topic list with checkboxes to hide or show topics, plus a `/` filter that accepts a
  substring of topic or payload, or an MQTT wildcard filter such as `sensors/+/temp`.
- Client list built from the broker's log topics: address, protocol, keepalive, username,
  subscriptions with QoS, and recent events. Enter filters the stream by a client's
  subscription.
- Replay: `r` resends the selected message with the same topic, payload, QoS, retain flag,
  and MQTT v5 properties. Subscribers cannot tell it from the original. `R` skips the
  confirmation.
- `$SYS` broker statistics in a resizable sidebar.

## Prerequisites

- A C++17 compiler and CMake 3.14 or newer
- libmosquitto development files (`libmosquitto-dev`)
- Internet access on first configure: the FTXUI terminal library is fetched from GitHub and
  linked statically, so the installed binary depends only on `libmosquitto1`.

```bash
sudo apt-get install build-essential cmake libmosquitto-dev
```

## Build

```bash
cmake -S . -B build
cmake --build build -j
./build/mosquitto-monitor            # connects to localhost:1886
sudo cmake --install build           # installs to /usr/local/bin
```

Debian package:

```bash
cd build && cpack
sudo apt install ./mosquitto-monitor-0.0.1-Linux.deb
```

Unit tests (parsing and formatting only, no broker needed):

```bash
cmake -S . -B build -DMOSQUITTO_MONITOR_TESTS=ON
cmake --build build -j && ctest --test-dir build
```

## Usage

```
mosquitto-monitor [--host localhost] [--port 1886] [--client-id mosquitto_monitor]
                  [--username u] [--password p] [--keepalive 60] [--history 5000] [--no-mouse]
```

| Key | Action |
| --- | --- |
| `q`, Ctrl+C | quit |
| `?` | help |
| Up/Down, PgUp/PgDn, wheel | select a message; scrolling up leaves follow mode |
| Home / End | oldest message / follow the newest |
| `p` | pause the view (the history keeps filling) |
| `d`, Enter | show or hide the detail pane |
| `x` | toggle hex and text in the detail pane |
| `t` | topic list: Up/Down, Space toggles, `a` all, `n` none, `i` invert, click toggles |
| `c` | client list: Up/Down, Enter cycles the filter through the client's subscriptions |
| `/` | edit the filter; Enter applies, Esc clears |
| Esc | clear the filter, then close the side pane |
| `r` / `R` | resend the selected message, with / without confirmation |

Only one client may use a given client ID. Run a second monitor with `--client-id`.

## Client visibility

Mosquitto exposes nothing about individual clients through `$SYS`, so the client list is built
from the broker's own log, published to `$SYS/broker/log/#`. Enable it once on the broker:

```bash
sudo cp broker/monitor-log.conf /etc/mosquitto/conf.d/
sudo systemctl restart mosquitto
```

Limitations of this approach in Mosquitto 2.0:

- Log topics are not retained. Clients connected before the monitor starts show up only when
  they next subscribe, disconnect, or reconnect.
- The broker never publishes debug-level log lines to topics, and "who published this message"
  is debug-level. Messages are therefore attributed by topic, not by client.
- Client IDs containing spaces cannot be parsed reliably from subscribe lines.
- If `log_timestamp_format` is set, use a format without spaces.

## Replay caveats

The monitor publishes over its own connection, so the broker's log names the monitor as the
sender and any ACLs apply to the monitor's credentials. Resending a retained message replaces
the broker's retained value for that topic; the confirmation dialog warns about this.
