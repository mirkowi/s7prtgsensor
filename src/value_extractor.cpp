#include "value_extractor.hpp"
#include <stdexcept>
#include <cstring>

// Read big-endian uint16 from buffer at offset
static uint16_t read_be16(const uint8_t* buf) {
    return (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
}

// Read big-endian uint32 from buffer at offset
static uint32_t read_be32(const uint8_t* buf) {
    return (static_cast<uint32_t>(buf[0]) << 24)
         | (static_cast<uint32_t>(buf[1]) << 16)
         | (static_cast<uint32_t>(buf[2]) << 8)
         |  static_cast<uint32_t>(buf[3]);
}

ChannelValue extract_value(const std::vector<uint8_t>& buffer,
                           int local_offset,
                           int bit_number,
                           S7DataType dtype)
{
    // Bounds check
    auto need = [&](int extra) {
        if (local_offset < 0 || (local_offset + extra) > static_cast<int>(buffer.size()))
            throw std::runtime_error("Buffer overrun at offset " + std::to_string(local_offset));
    };

    switch (dtype) {
        case S7DataType::BOOL: {
            need(1);
            uint8_t byte_val = buffer[static_cast<size_t>(local_offset)];
            bool b = (byte_val >> bit_number) & 0x01;
            return b;
        }
        case S7DataType::BYTE: {
            need(1);
            return static_cast<int64_t>(buffer[static_cast<size_t>(local_offset)]);
        }
        case S7DataType::CHAR: {
            need(1);
            return static_cast<int64_t>(static_cast<int8_t>(buffer[static_cast<size_t>(local_offset)]));
        }
        case S7DataType::WORD: {
            need(2);
            uint16_t v = read_be16(&buffer[static_cast<size_t>(local_offset)]);
            return static_cast<int64_t>(v);
        }
        case S7DataType::INT: {
            need(2);
            uint16_t raw = read_be16(&buffer[static_cast<size_t>(local_offset)]);
            int16_t signed_val;
            std::memcpy(&signed_val, &raw, 2);
            return static_cast<int64_t>(signed_val);
        }
        case S7DataType::DWORD: {
            need(4);
            uint32_t v = read_be32(&buffer[static_cast<size_t>(local_offset)]);
            return static_cast<int64_t>(v);
        }
        case S7DataType::DINT: {
            need(4);
            uint32_t raw = read_be32(&buffer[static_cast<size_t>(local_offset)]);
            int32_t signed_val;
            std::memcpy(&signed_val, &raw, 4);
            return static_cast<int64_t>(signed_val);
        }
        case S7DataType::REAL: {
            need(4);
            uint32_t raw = read_be32(&buffer[static_cast<size_t>(local_offset)]);
            float f;
            std::memcpy(&f, &raw, 4);
            return static_cast<double>(f);
        }
    }
    throw std::runtime_error("Unknown datatype in extract_value");
}

double apply_scale(double raw, double scale_factor, double scale_offset) {
    return raw * scale_factor + scale_offset;
}
