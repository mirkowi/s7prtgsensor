#pragma once
#include "config.hpp"
#include <string>

enum class S7Area {
    DB,   // Datenbaustein
    MK,   // Merker (Flags)
    PE,   // Eingänge (Inputs)
    PA,   // Ausgänge (Outputs)
};

struct ParsedAddress {
    S7Area area;
    int    db_number   = 0;
    int    byte_offset = 0;
    int    bit_number  = 0;   // 0–7, relevant für BOOL
    int    byte_size   = 1;   // Anzahl Bytes für diesen Typ
};

ParsedAddress parse_address(const std::string& addr, S7DataType dtype);
