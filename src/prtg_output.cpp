#include "prtg_output.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>

static std::string xml_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;";  break;
            case '<': out += "&lt;";   break;
            case '>': out += "&gt;";   break;
            case '"': out += "&quot;"; break;
            default:  out += c;
        }
    }
    return out;
}

static std::string format_double(double v, int decimals) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << v;
    return oss.str();
}

void output_prtg_success(const std::vector<ChannelResult>& results,
                         const std::string& status_text)
{
    std::cout << "<prtg>\n";

    for (auto& r : results) {
        const ChannelConfig& cfg = *r.config;
        std::cout << "  <result>\n";
        std::cout << "    <channel>" << xml_escape(cfg.name) << "</channel>\n";

        if (std::holds_alternative<std::string>(r.value)) {
            // String-Kanäle (CPU.Info.*): Wert=1, Text als customunit
            const std::string& sv = std::get<std::string>(r.value);
            std::cout << "    <value>1</value>\n";
            std::cout << "    <unit>Custom</unit>\n";
            std::cout << "    <customunit>" << xml_escape(sv) << "</customunit>\n";
        } else {
            bool is_float = (cfg.float_decimals > 0)
                         || std::holds_alternative<double>(r.value);

            std::cout << "    <value>" << format_double(r.scaled_value, is_float ? cfg.float_decimals : 0) << "</value>\n";

            if (is_float)
                std::cout << "    <float>1</float>\n";

            // Unit
            if (!cfg.unit.empty() && cfg.unit != "Custom") {
                std::cout << "    <unit>" << xml_escape(cfg.unit) << "</unit>\n";
            } else {
                std::cout << "    <unit>Custom</unit>\n";
                if (!cfg.customunit.empty())
                    std::cout << "    <customunit>" << xml_escape(cfg.customunit) << "</customunit>\n";
            }

            if (cfg.float_decimals > 0)
                std::cout << "    <decimalmode>All</decimalmode>\n";

            // Limits
            if (cfg.limits_enabled) {
                std::cout << "    <limitsEnabled>1</limitsEnabled>\n";
                if (cfg.limit_min_error)
                    std::cout << "    <limitMinError>"   << format_double(*cfg.limit_min_error,   2) << "</limitMinError>\n";
                if (cfg.limit_min_warning)
                    std::cout << "    <limitMinWarning>" << format_double(*cfg.limit_min_warning, 2) << "</limitMinWarning>\n";
                if (cfg.limit_max_warning)
                    std::cout << "    <limitMaxWarning>" << format_double(*cfg.limit_max_warning, 2) << "</limitMaxWarning>\n";
                if (cfg.limit_max_error)
                    std::cout << "    <limitMaxError>"   << format_double(*cfg.limit_max_error,   2) << "</limitMaxError>\n";
            }
        }

        std::cout << "  </result>\n";
    }

    std::cout << "  <text>" << xml_escape(status_text) << "</text>\n";
    std::cout << "</prtg>\n";
}

void output_prtg_error(const std::string& message) {
    std::cout << "<prtg>\n";
    std::cout << "  <error>1</error>\n";
    std::cout << "  <text>" << xml_escape(message) << "</text>\n";
    std::cout << "</prtg>\n";
}
