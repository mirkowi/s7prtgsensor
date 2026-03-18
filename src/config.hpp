#pragma once
#include <string>
#include <vector>
#include <optional>

enum class S7DataType {
    BOOL,
    BYTE,
    WORD,
    DWORD,
    INT,
    DINT,
    REAL,
    CHAR
};

S7DataType parse_datatype(const std::string& s);

struct ChannelConfig {
    std::string name;
    std::string address;
    S7DataType  datatype;

    double scale_factor  = 1.0;
    double scale_offset  = 0.0;
    int    float_decimals = 0;   // 0 = integer output

    std::string unit        = "Custom";
    std::string customunit;

    bool   limits_enabled      = false;
    std::optional<double> limit_min_error;
    std::optional<double> limit_min_warning;
    std::optional<double> limit_max_warning;
    std::optional<double> limit_max_error;
};

struct ConnectionConfig {
    std::string ip;
    int  rack       = 0;
    int  slot       = 1;
    int  timeout_ms = 3000;
};

struct SensorConfig {
    ConnectionConfig connection;
    std::vector<ChannelConfig> channels;
};

SensorConfig load_config(const std::string& path);
