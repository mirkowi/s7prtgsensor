#pragma once
#include "config.hpp"
#include <vector>
#include <variant>
#include <cstdint>

using ChannelValue = std::variant<bool, int64_t, double>;

// Extract a value from a byte buffer.
// local_offset: byte index within the buffer (already relative to buffer start)
// bit_number:   bit index 0-7 (only used for BOOL)
// dtype:        data type determines how many bytes are read and interpreted
ChannelValue extract_value(const std::vector<uint8_t>& buffer,
                           int local_offset,
                           int bit_number,
                           S7DataType dtype);

// Apply scale: result = raw_double * scale_factor + scale_offset
double apply_scale(double raw, double scale_factor, double scale_offset);
