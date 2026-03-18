#pragma once
#include <string>
#include <map>
#include <vector>
#include <cstdint>

// Point structure for position messages
struct Point {
    int x;
    int y;
};

// Config structure for app configuration messages
struct Config {
    int setting1;
    bool setting2;
};

// User structure for user info messages
struct User {
    int id;
    std::string name;
    std::vector<int> friends;
};

// DeviceStatus structure for device status messages
struct DeviceStatus {
    std::string deviceName;
    bool online;
    double battery;
};

// DataPacket structure for binary data messages
struct DataPacket {
    std::vector<uint8_t> data;
    int seq;
};

// Nested structure for complex nested messages
struct Nested {
    User user;
    DeviceStatus status;
    std::vector<DataPacket> packets;
};

// Settings structure for map-like config
struct Settings {
    std::map<std::string, std::string> kv;
};