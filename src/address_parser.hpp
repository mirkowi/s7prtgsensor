#pragma once
#include "config.hpp"
#include <string>

enum class S7Area {
    DB,         // Datenbaustein
    MK,         // Merker (Flags)
    PE,         // Eingänge (Inputs)
    PA,         // Ausgänge (Outputs)
    CPU_STATUS, // CPU-Betriebszustand (RUN=1 / STOP=0)
    CPU_INFO,   // CPU-Identifikation (string)
    SZL,        // Systemzustandsliste
};

enum class CpuInfoField {
    NONE,
    MODULE_TYPE_NAME,  // CPU.Info.ModuleTypeName
    SERIAL_NUMBER,     // CPU.Info.SerialNumber
    AS_NAME,           // CPU.Info.ASName
    MODULE_NAME,       // CPU.Info.ModuleName
    COPYRIGHT,         // CPU.Info.Copyright
};

struct ParsedAddress {
    S7Area area        = S7Area::DB;
    int    db_number   = 0;
    int    byte_offset = 0;
    int    bit_number  = 0;   // 0–7, relevant für BOOL
    int    byte_size   = 1;   // Anzahl Bytes für diesen Typ

    // CPU_INFO
    CpuInfoField cpu_info_field = CpuInfoField::NONE;

    // SZL
    int szl_id    = 0;
    int szl_index = 0;
};

// Virtuelle Sonderadressen:
//   CPU.Status                   → CPU-Betriebszustand (1=RUN, 0=STOP)
//   CPU.Info.ModuleTypeName      → Modultyp-Bezeichnung (string)
//   CPU.Info.SerialNumber        → Seriennummer (string)
//   CPU.Info.ASName              → AS-Name / Programmname (string)
//   CPU.Info.ModuleName          → Modulname (string)
//   CPU.Info.Copyright           → Copyright-String (string)
//   SZL.<hex_id>.<index>.<byte>  → SZL-Rohwert, z.B. SZL.0x0132.4.0
ParsedAddress parse_address(const std::string& addr, S7DataType dtype);
