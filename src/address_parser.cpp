#include "address_parser.hpp"
#include <regex>
#include <stdexcept>
#include <cctype>
#include <algorithm>

static int dtype_bytes(S7DataType dt) {
    switch (dt) {
        case S7DataType::BOOL:  return 1;
        case S7DataType::BYTE:
        case S7DataType::CHAR:  return 1;
        case S7DataType::WORD:
        case S7DataType::INT:   return 2;
        case S7DataType::DWORD:
        case S7DataType::DINT:
        case S7DataType::REAL:  return 4;
    }
    return 1;
}

// Converts address string to uppercase for matching, preserving digits.
static std::string to_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::toupper(c); });
    return s;
}

ParsedAddress parse_address(const std::string& addr, S7DataType dtype) {
    ParsedAddress pa;
    pa.byte_size = dtype_bytes(dtype);

    std::string a = to_upper(addr);

    // --- DB addresses: DB<n>.DB[BWDX]<byte>[.<bit>] ---
    // DB5.DBX0.3   -> DB, db=5, byte=0, bit=3, BOOL
    // DB5.DBB4     -> DB, db=5, byte=4
    // DB5.DBW6     -> DB, db=5, byte=6
    // DB5.DBD10    -> DB, db=5, byte=10
    {
        static const std::regex re_db_bit(R"(^DB(\d+)\.DBX(\d+)\.([0-7])$)");
        static const std::regex re_db_byte(R"(^DB(\d+)\.DB([BWDX])(\d+)$)");
        std::smatch m;

        if (std::regex_match(a, m, re_db_bit)) {
            pa.area       = S7Area::DB;
            pa.db_number  = std::stoi(m[1]);
            pa.byte_offset = std::stoi(m[2]);
            pa.bit_number  = std::stoi(m[3]);
            pa.byte_size   = 1;
            return pa;
        }

        if (std::regex_match(a, m, re_db_byte)) {
            pa.area       = S7Area::DB;
            pa.db_number  = std::stoi(m[1]);
            char sz       = m[2].str()[0];
            pa.byte_offset = std::stoi(m[3]);
            switch (sz) {
                case 'B': pa.byte_size = 1; break;
                case 'W': pa.byte_size = 2; break;
                case 'D': pa.byte_size = 4; break;
                case 'X': pa.byte_size = 1; break; // bit
                default:  pa.byte_size = dtype_bytes(dtype);
            }
            return pa;
        }
    }

    // --- Merker (M) ---
    // MW100 -> Merkerwort byte 100
    // MB50  -> Merkerbyte 50
    // M0.1  -> Merkerbit byte 0, bit 1
    {
        static const std::regex re_m_bit(R"(^M(\d+)\.([0-7])$)");
        static const std::regex re_m_sz(R"(^M([BWD])(\d+)$)");
        static const std::regex re_m_byte(R"(^M(\d+)$)");
        std::smatch m;

        if (std::regex_match(a, m, re_m_bit)) {
            pa.area        = S7Area::MK;
            pa.byte_offset = std::stoi(m[1]);
            pa.bit_number  = std::stoi(m[2]);
            pa.byte_size   = 1;
            return pa;
        }

        if (std::regex_match(a, m, re_m_sz)) {
            pa.area        = S7Area::MK;
            char sz        = m[1].str()[0];
            pa.byte_offset = std::stoi(m[2]);
            switch (sz) {
                case 'B': pa.byte_size = 1; break;
                case 'W': pa.byte_size = 2; break;
                case 'D': pa.byte_size = 4; break;
                default:  pa.byte_size = dtype_bytes(dtype);
            }
            return pa;
        }

        if (std::regex_match(a, m, re_m_byte)) {
            pa.area        = S7Area::MK;
            pa.byte_offset = std::stoi(m[1]);
            pa.byte_size   = dtype_bytes(dtype);
            return pa;
        }
    }

    // --- Eingänge: E0.0 / I0.0 ---
    {
        static const std::regex re_e_bit(R"(^[EI](\d+)\.([0-7])$)");
        static const std::regex re_e_sz(R"(^[EI]([BWD])(\d+)$)");
        static const std::regex re_e_byte(R"(^[EI](\d+)$)");
        std::smatch m;

        if (std::regex_match(a, m, re_e_bit)) {
            pa.area        = S7Area::PE;
            pa.byte_offset = std::stoi(m[1]);
            pa.bit_number  = std::stoi(m[2]);
            pa.byte_size   = 1;
            return pa;
        }

        if (std::regex_match(a, m, re_e_sz)) {
            pa.area        = S7Area::PE;
            char sz        = m[1].str()[0];
            pa.byte_offset = std::stoi(m[2]);
            switch (sz) {
                case 'B': pa.byte_size = 1; break;
                case 'W': pa.byte_size = 2; break;
                case 'D': pa.byte_size = 4; break;
                default:  pa.byte_size = dtype_bytes(dtype);
            }
            return pa;
        }

        if (std::regex_match(a, m, re_e_byte)) {
            pa.area        = S7Area::PE;
            pa.byte_offset = std::stoi(m[1]);
            pa.byte_size   = dtype_bytes(dtype);
            return pa;
        }
    }

    // --- Ausgänge: A0.0 / Q0.0 ---
    {
        static const std::regex re_a_bit(R"(^[AQ](\d+)\.([0-7])$)");
        static const std::regex re_a_sz(R"(^[AQ]([BWD])(\d+)$)");
        static const std::regex re_a_byte(R"(^[AQ](\d+)$)");
        std::smatch m;

        if (std::regex_match(a, m, re_a_bit)) {
            pa.area        = S7Area::PA;
            pa.byte_offset = std::stoi(m[1]);
            pa.bit_number  = std::stoi(m[2]);
            pa.byte_size   = 1;
            return pa;
        }

        if (std::regex_match(a, m, re_a_sz)) {
            pa.area        = S7Area::PA;
            char sz        = m[1].str()[0];
            pa.byte_offset = std::stoi(m[2]);
            switch (sz) {
                case 'B': pa.byte_size = 1; break;
                case 'W': pa.byte_size = 2; break;
                case 'D': pa.byte_size = 4; break;
                default:  pa.byte_size = dtype_bytes(dtype);
            }
            return pa;
        }

        if (std::regex_match(a, m, re_a_byte)) {
            pa.area        = S7Area::PA;
            pa.byte_offset = std::stoi(m[1]);
            pa.byte_size   = dtype_bytes(dtype);
            return pa;
        }
    }

    // --- CPU.Status ---
    if (a == "CPU.STATUS") {
        pa.area      = S7Area::CPU_STATUS;
        pa.byte_size = 0;
        return pa;
    }

    // --- CPU.DiagBufCount → Alias für SZL 0x0074, Byte 2-3 (N_DR) ---
    if (a == "CPU.DIAGBUFCOUNT") {
        pa.area        = S7Area::SZL;
        pa.szl_id      = 0x0074;
        pa.szl_index   = 0;
        pa.byte_offset = 2;   // N_DR liegt nach dem 2-Byte LENTHDR
        pa.byte_size   = 2;
        return pa;
    }

    // --- CPU.Info.* ---
    {
        static const std::pair<const char*, CpuInfoField> info_fields[] = {
            {"CPU.INFO.MODULETYPENAME", CpuInfoField::MODULE_TYPE_NAME},
            {"CPU.INFO.SERIALNUMBER",   CpuInfoField::SERIAL_NUMBER},
            {"CPU.INFO.ASNAME",         CpuInfoField::AS_NAME},
            {"CPU.INFO.MODULENAME",     CpuInfoField::MODULE_NAME},
            {"CPU.INFO.COPYRIGHT",      CpuInfoField::COPYRIGHT},
        };
        for (auto& [key, field] : info_fields) {
            if (a == key) {
                pa.area           = S7Area::CPU_INFO;
                pa.cpu_info_field = field;
                pa.byte_size      = 0;
                return pa;
            }
        }
    }

    // --- SZL.<hex_id>.<index>.<byte_offset> ---
    // Beispiel: SZL.0x0132.4.0  oder  SZL.306.4.0
    {
        static const std::regex re_szl(R"(^SZL\.(0X[0-9A-F]+|\d+)\.(\d+)\.(\d+)$)");
        std::smatch m;
        if (std::regex_match(a, m, re_szl)) {
            pa.area        = S7Area::SZL;
            pa.szl_id      = static_cast<int>(std::stoul(m[1].str(), nullptr, 0));
            pa.szl_index   = std::stoi(m[2]);
            pa.byte_offset = std::stoi(m[3]);
            pa.byte_size   = dtype_bytes(dtype);
            return pa;
        }
    }

    throw std::runtime_error("Cannot parse address: " + addr);
}
