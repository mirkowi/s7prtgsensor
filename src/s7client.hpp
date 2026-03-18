#pragma once
#include "address_parser.hpp"
#include <snap7.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdint>

class S7Exception : public std::runtime_error {
public:
    explicit S7Exception(const std::string& msg) : std::runtime_error(msg) {}
};

struct AreaReadResult {
    std::vector<uint8_t> data;
    int  start_byte = 0;   // first byte offset that was read
};

struct S7CpuInfoData {
    std::string module_type_name;
    std::string serial_number;
    std::string as_name;
    std::string copyright;
    std::string module_name;
};

struct SzlReadResult {
    std::vector<uint8_t> data;         // raw bytes nach dem Header
    int record_length = 0;             // LENTHDR: Länge eines Datensatzes
    int record_count  = 0;             // N_DR:    Anzahl Datensätze
};

class S7Client {
public:
    S7Client();
    ~S7Client();

    // Non-copyable
    S7Client(const S7Client&) = delete;
    S7Client& operator=(const S7Client&) = delete;

    void connect(const std::string& ip, int rack, int slot, int timeout_ms);
    void disconnect() noexcept;

    AreaReadResult read_area(S7Area area, int db_number, int start_byte, int byte_count);

    std::string    get_cpu_state();
    S7CpuInfoData  get_cpu_info();
    SzlReadResult  read_szl(int szl_id, int szl_index);

private:
    S7Object client_ = 0;

    static int area_code(S7Area area);
    static std::string error_text(int code);
};
