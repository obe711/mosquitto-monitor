cmake_minimum_required(VERSION 3.14)

set(PROJECT_VER_MAJOR 0)
set(PROJECT_VER_MINOR 0)
set(PROJECT_VER_PATCH 1)

project(mosquitto-monitor VERSION "${PROJECT_VER_MAJOR}.${PROJECT_VER_MINOR}.${PROJECT_VER_PATCH}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

find_library(MOSQUITTO_LIB mosquitto REQUIRED)
find_package(Threads REQUIRED)

include(FetchContent)
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(FTXUI_QUIET ON CACHE BOOL "" FORCE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG        v7.0.3
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(ftxui)

add_library(monitor-core STATIC
  src/cli/options.cpp
  src/format/payload.cpp
  src/format/units.cpp
  src/model/metrics.cpp
  src/model/message_store.cpp
  src/model/stream_view.cpp
  src/model/topic_table.cpp
  src/model/topic_match.cpp
  src/model/client_table.cpp
  src/parse/broker_log.cpp
)
target_include_directories(monitor-core PUBLIC src)
target_compile_options(monitor-core PRIVATE -Wall -Wextra)

add_executable(mosquitto-monitor
  src/main.cpp
  src/mqtt/client.cpp
  src/ui/theme.cpp
  src/ui/ui.cpp
  src/ui/panes/sidebar.cpp
  src/ui/panes/stream.cpp
  src/ui/panes/detail.cpp
  src/ui/panes/status_bar.cpp
  src/ui/panes/topics.cpp
  src/ui/panes/clients.cpp
  src/ui/dialogs/help.cpp
  src/ui/dialogs/replay.cpp
)
target_compile_options(mosquitto-monitor PRIVATE -Wall -Wextra)
target_link_libraries(mosquitto-monitor PRIVATE
  monitor-core ftxui::component ${MOSQUITTO_LIB} Threads::Threads)

option(MOSQUITTO_MONITOR_TESTS "Build unit tests" OFF)
if(MOSQUITTO_MONITOR_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

install(TARGETS mosquitto-monitor
  RUNTIME DESTINATION bin
  COMPONENT runtime
)

set(CPACK_DEBIAN_PACKAGE_NAME "mosquitto-monitor")
set(CPACK_PACKAGE_NAME "mosquitto-monitor")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libmosquitto1")
set(CPACK_PACKAGE_VENDOR "Obe's Tools")
set(CPACK_PACKAGE_CONTACT "obe711@gmail.com")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Obediah Klopfenstein")
set(NEXUS_DEB_USER "nexus")
set(NEXUS_DEB_GROUP "nexus")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/obe711/mosquitto-monitor")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "A real-time monitoring tool for MQTT brokers that displays various system metrics in a clean, organized interface.")

include(cmake-deb)
include(cmake-uninstall)
