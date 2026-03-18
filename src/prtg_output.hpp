#pragma once
#include "config.hpp"
#include "value_extractor.hpp"
#include <string>
#include <vector>

struct ChannelResult {
    const ChannelConfig* config = nullptr;
    ChannelValue         value;
    double               scaled_value = 0.0;
};

void output_prtg_success(const std::vector<ChannelResult>& results,
                         const std::string& status_text);

void output_prtg_error(const std::string& message);
