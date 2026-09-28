# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

A C++17 full-terminal monitor and debugging tool for a Mosquitto MQTT broker. It subscribes to
`#` and selected `$SYS/broker/...` topics via libmosquitto (MQTT v5) and renders an FTXUI
dashboard: statistics sidebar, live message stream with text/hex detail, topic and client side
panes, and message replay.

## Build and run

Requires `cmake` (>= 3.14), a C++17 compiler, `libmosquitto-dev`, and internet on the first
configure (FTXUI v7.0.3 is fetched with FetchContent and linked statically).

```bash
cmake -S . -B build -DMOSQUITTO_MONITOR_TESTS=ON
cmake --build build -j
ctest --test-dir build                     # plain-assert tests for parse/, format/, model/
./build/mosquitto-monitor --port 1886      # needs a broker; see --help for options
cd build && cpack                          # .deb, depends only on libmosquitto1
```

Manual TUI checks work well in tmux: `tmux new-session -d -s mm -x 150 -y 42 ./build/mosquitto-monitor`,
then `tmux send-keys -t mm <key>` and `tmux capture-pane -t mm -p`. Always pass a distinct
`--client-id` when another monitor may be running; the broker kicks duplicate IDs.

The local broker listens on 1886 (not 1883). Clients cannot publish to `$SYS/` topics, so to
test the client pane without root, start a private broker:
`mosquitto -c <conf with "listener 18860", "allow_anonymous true" and broker/monitor-log.conf>`.

## Layout and dependency rules

```
src/cli/      command line options (std only)
src/format/   payload text/hex helpers, units (std only)
src/parse/    $SYS/broker/log line parser -> ClientEvent variants (std only)
src/model/    AppState and pure state logic: metrics, message store, topic and client tables,
              stream_view (visible list + selection), topic_match (MQTT wildcards)
src/mqtt/     message.hpp (plain structs), sink.hpp (callback interface), client.cpp is the ONLY
              file that includes <mosquitto.h>
src/ui/       FTXUI only lives here. ui.cpp owns App + AppState + key map; panes/ and dialogs/
              are render functions or small components
src/main.cpp  the only file that sees both mqtt::Client and ui::Ui
tests/        one executable per module, plain assert()
broker/       monitor-log.conf drop-in for /etc/mosquitto/conf.d
```

`monitor-core` (cli, format, parse, model) is a static library shared by the binary and tests.
Add new sources to the explicit lists in `CMakeLists.txt`; there is no glob.

## Threading

libmosquitto's network thread calls the `mqtt::Sink` methods implemented by `ui::Ui`. Those
only append to a mutex-guarded pending batch and post `Event::Custom` to the FTXUI loop; `drain()`
applies the batch to `AppState` on the UI thread. Everything in `AppState` is UI-thread only.
Post an event, not a bare closure: in FTXUI 7 a posted closure runs but does not invalidate the
frame, so nothing redraws.

`Ui` (and its `App`) is constructed before `mqtt::Client` in `main` so the client's thread is
stopped before the UI loop is destroyed. Ctrl+C is caught (`ForceHandleCtrlC(false)`) so the
normal shutdown path runs.

## MQTT details that matter

- Subscriptions use QoS 2 with `RETAIN_AS_PUBLISHED` and, for `#`, `NO_LOCAL`, so captured
  messages carry the publisher's QoS and retain flag and the monitor's own replays are not
  echoed back. A replay is appended to the store locally with `Origin::Replay`.
- `#` never matches `$SYS/...`; the metric and log topics are subscribed individually.
- `MessageStore::push` assigns `seq`; selection is stored as a seq, not an index, because the
  ring evicts from the front.
- Mosquitto 2.0 publishes only connect/disconnect/subscribe/unsubscribe lines to
  `$SYS/broker/log/#`; publish attribution (debug level) is never available there.

## Style

Small files with one owner each, plain C++17, no narrative comments; comment only a non-obvious
why. Warnings are on (`-Wall -Wextra`) and the build should stay warning-free.
