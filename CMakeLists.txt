cmake_minimum_required(VERSION 3.10)

set(PROJECT_VER_MAJOR 0)
set(PROJECT_VER_MINOR 0)
set(PROJECT_VER_PATCH 1)

project(mosquitto-monitor VERSION "${PROJECT_VER_MAJOR}.${PROJECT_VER_MINOR}.${PROJECT_VER_PATCH}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Set release flags
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find Mosquitto library
find_library(MOSQUITTO_LIB mosquitto REQUIRED)

# Add executable
add_executable(mosquitto-monitor main.cpp)

# Link against Mosquitto library
target_link_libraries(mosquitto-monitor ${MOSQUITTO_LIB})

# Installation rules
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

# Create a package
include(cmake-deb)

include(cmake-uninstall)
