#include "config.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

S7DataType parse_datatype(const std::string& s) {
    if (s == "BOOL")  return S7DataType::BOOL;
    if (s == "BYTE")  return S7DataType::BYTE;
    if (s == "WORD")  return S7DataType::WORD;
    if (s == "DWORD") return S7DataType::DWORD;
    if (s == "INT")   return S7DataType::INT;
    if (s == "DINT")  return S7DataType::DINT;
    if (s == "REAL")  return S7DataType::REAL;
    if (s == "CHAR")  return S7DataType::CHAR;
    throw std::runtime_error("Unknown datatype: " + s);
}

SensorConfig load_config(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("Cannot open config file: " + path);

    json j;
    try {
        f >> j;
    } catch (const json::parse_error& e) {
        throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    }

    SensorConfig cfg;

    // Connection
    auto& conn = j.at("connection");
    cfg.connection.ip         = conn.at("ip").get<std::string>();
    cfg.connection.rack       = conn.value("rack", 0);
    cfg.connection.slot       = conn.value("slot", 1);
    cfg.connection.timeout_ms = conn.value("timeout_ms", 3000);

    // Channels
    for (auto& ch : j.at("channels")) {
        ChannelConfig c;
        c.name     = ch.at("name").get<std::string>();
        c.address  = ch.at("address").get<std::string>();
        c.datatype = parse_datatype(ch.value("datatype", std::string("BYTE")));

        c.scale_factor  = ch.value("scale_factor", 1.0);
        c.scale_offset  = ch.value("scale_offset", 0.0);
        c.float_decimals = ch.value("float_decimals", 0);

        c.unit       = ch.value("unit", std::string("Custom"));
        c.customunit = ch.value("customunit", std::string(""));

        c.limits_enabled = ch.value("limits_enabled", false);
        if (ch.contains("limit_min_error"))   c.limit_min_error   = ch["limit_min_error"].get<double>();
        if (ch.contains("limit_min_warning")) c.limit_min_warning = ch["limit_min_warning"].get<double>();
        if (ch.contains("limit_max_warning")) c.limit_max_warning = ch["limit_max_warning"].get<double>();
        if (ch.contains("limit_max_error"))   c.limit_max_error   = ch["limit_max_error"].get<double>();

        cfg.channels.push_back(std::move(c));
    }

    if (cfg.channels.empty())
        throw std::runtime_error("Config has no channels defined");

    return cfg;
}
