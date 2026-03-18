#include "s7client.hpp"
#include "logger.hpp"
#include <snap7.h>
#include <sstream>
#include <iomanip>
#include <cstring>

S7Client::S7Client() {
    client_ = Cli_Create();
    if (!client_)
        throw S7Exception("Failed to create snap7 client object");
}

S7Client::~S7Client() {
    if (client_) {
        disconnect();
        Cli_Destroy(&client_);
    }
}

int S7Client::area_code(S7Area area) {
    switch (area) {
        case S7Area::DB: return S7AreaDB;
        case S7Area::MK: return S7AreaMK;
        case S7Area::PE: return S7AreaPE;
        case S7Area::PA: return S7AreaPA;
    }
    return S7AreaDB;
}

std::string S7Client::error_text(int code) {
    char buf[256] = {};
    // snap7 provides Cli_ErrorText but it returns a static description
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << code;
    return oss.str();
}

void S7Client::connect(const std::string& ip, int rack, int slot, int timeout_ms) {
    LOG("Connecting to " << ip << " rack=" << rack << " slot=" << slot << " timeout=" << timeout_ms << "ms");

    // Set timeouts (p_i32_SendTimeout=4, p_i32_RecvTimeout=5 in snap7 API)
    int timeout = timeout_ms;
    Cli_SetParam(client_, p_i32_SendTimeout, &timeout);
    Cli_SetParam(client_, p_i32_RecvTimeout, &timeout);

    int result = Cli_ConnectTo(client_, ip.c_str(), rack, slot);
    if (result != 0) {
        throw S7Exception("S7 connect failed: " + error_text(result));
    }
    LOG("Connected successfully");
}

void S7Client::disconnect() noexcept {
    if (client_) {
        Cli_Disconnect(client_);
        LOG("Disconnected");
    }
}

AreaReadResult S7Client::read_area(S7Area area, int db_number, int start_byte, int byte_count) {
    if (byte_count <= 0)
        throw S7Exception("read_area: byte_count must be > 0");

    AreaReadResult result;
    result.start_byte = start_byte;
    result.data.resize(static_cast<size_t>(byte_count), 0);

    LOG("ReadArea area=" << area_code(area) << " db=" << db_number
        << " start=" << start_byte << " count=" << byte_count);

    int ret = Cli_ReadArea(client_,
                           area_code(area),
                           db_number,
                           start_byte,
                           byte_count,
                           S7WLByte,
                           result.data.data());
    if (ret != 0) {
        throw S7Exception("ReadArea failed: " + error_text(ret));
    }

    return result;
}

std::string S7Client::get_cpu_state() {
    int status = 0;
    int ret = Cli_GetPlcStatus(client_, &status);
    if (ret != 0) {
        LOG("GetPlcStatus failed: " << error_text(ret));
        return "UNKNOWN";
    }
    // snap7 status codes: S7CpuStatusRun=0x08, S7CpuStatusStop=0x04
    switch (status) {
        case S7CpuStatusRun:  return "RUN";
        case S7CpuStatusStop: return "STOP";
        default:              return "UNKNOWN";
    }
}
