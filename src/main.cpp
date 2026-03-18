#include "config.hpp"
#include "address_parser.hpp"
#include "s7client.hpp"
#include "value_extractor.hpp"
#include "prtg_output.hpp"
#include "logger.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

// ── CLI parsing ──────────────────────────────────────────────────────────────

struct CliArgs {
    std::string config_path;
    std::string ip_override;
    int         rack_override  = -1;
    int         slot_override  = -1;
    bool        debug          = false;
};

static void usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [--config <path>] [--ip <addr>] [--rack <n>] [--slot <n>] [--debug]\n";
}

static CliArgs parse_args(int argc, char* argv[]) {
    CliArgs args;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--debug") {
            args.debug = true;
        } else if (a == "--config" && i + 1 < argc) {
            args.config_path = argv[++i];
        } else if (a == "--ip" && i + 1 < argc) {
            args.ip_override = argv[++i];
        } else if (a == "--rack" && i + 1 < argc) {
            args.rack_override = std::stoi(argv[++i]);
        } else if (a == "--slot" && i + 1 < argc) {
            args.slot_override = std::stoi(argv[++i]);
        } else if (a == "--help" || a == "-h") {
            usage(argv[0]);
        }
    }
    return args;
}

// ── Group key for batch reads ─────────────────────────────────────────────────

struct GroupKey {
    S7Area area;
    int    db_number;

    bool operator<(const GroupKey& o) const {
        if (area != o.area) return static_cast<int>(area) < static_cast<int>(o.area);
        return db_number < o.db_number;
    }
};

// ── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    CliArgs cli = parse_args(argc, argv);

    // Enable debug logger
    if (cli.debug) {
        // Log file next to exe
        fs::path exe_dir = fs::path(argv[0]).parent_path();
        Logger::instance().enable((exe_dir / "s7sensor_debug.log").string());
        LOG("=== s7sensor start ===");
    }

    try {
        // Determine config path
        if (cli.config_path.empty()) {
            fs::path exe_dir = fs::path(argv[0]).parent_path();
            cli.config_path = (exe_dir / "s7sensor.json").string();
        }

        LOG("Loading config: " << cli.config_path);
        SensorConfig cfg = load_config(cli.config_path);

        // Apply CLI overrides
        if (!cli.ip_override.empty())   cfg.connection.ip   = cli.ip_override;
        if (cli.rack_override >= 0)     cfg.connection.rack = cli.rack_override;
        if (cli.slot_override >= 0)     cfg.connection.slot = cli.slot_override;

        LOG("Connection: " << cfg.connection.ip << " rack=" << cfg.connection.rack
            << " slot=" << cfg.connection.slot);

        // ── 1. Parse all addresses ─────────────────────────────────────────
        struct ParsedChannel {
            size_t        cfg_index;
            ParsedAddress pa;
        };
        std::vector<ParsedChannel> parsed;
        parsed.reserve(cfg.channels.size());

        for (size_t i = 0; i < cfg.channels.size(); ++i) {
            auto& ch = cfg.channels[i];
            LOG("Parsing address: " << ch.address);
            ParsedAddress pa = parse_address(ch.address, ch.datatype);
            parsed.push_back({i, pa});
        }

        // ── 2. Group MEMORY channels by (area, db_number) ─────────────────
        // CPU_STATUS / CPU_INFO / SZL werden separat behandelt
        auto is_memory = [](S7Area a) {
            return a == S7Area::DB || a == S7Area::MK ||
                   a == S7Area::PE || a == S7Area::PA;
        };

        std::map<GroupKey, std::vector<size_t>> groups;
        for (size_t i = 0; i < parsed.size(); ++i) {
            if (is_memory(parsed[i].pa.area)) {
                GroupKey key{parsed[i].pa.area, parsed[i].pa.db_number};
                groups[key].push_back(i);
            }
        }

        // ── 3. Connect ────────────────────────────────────────────────────
        S7Client client;
        client.connect(cfg.connection.ip, cfg.connection.rack,
                       cfg.connection.slot, cfg.connection.timeout_ms);

        std::string cpu_state = client.get_cpu_state();
        LOG("CPU state: " << cpu_state);

        // ── 4. Batch read memory groups ───────────────────────────────────
        std::map<GroupKey, AreaReadResult> buffers;

        for (auto& [key, indices] : groups) {
            int min_byte = INT_MAX;
            int max_byte = 0;
            for (size_t idx : indices) {
                auto& pa = parsed[idx].pa;
                min_byte = std::min(min_byte, pa.byte_offset);
                max_byte = std::max(max_byte, pa.byte_offset + pa.byte_size);
            }
            int count = max_byte - min_byte;
            LOG("Group area=" << static_cast<int>(key.area) << " db=" << key.db_number
                << " start=" << min_byte << " count=" << count);
            buffers[key] = client.read_area(key.area, key.db_number, min_byte, count);
            buffers[key].start_byte = min_byte;
        }

        // ── 5. Spezial-Reads (während Verbindung noch aktiv) ──────────────
        std::optional<S7CpuInfoData>               cpu_info_cache;
        std::map<std::pair<int,int>, SzlReadResult> szl_cache;

        for (size_t i = 0; i < parsed.size(); ++i) {
            auto& pa = parsed[i].pa;
            if (pa.area == S7Area::CPU_INFO && !cpu_info_cache) {
                cpu_info_cache = client.get_cpu_info();
            } else if (pa.area == S7Area::SZL) {
                auto key = std::make_pair(pa.szl_id, pa.szl_index);
                if (szl_cache.find(key) == szl_cache.end())
                    szl_cache[key] = client.read_szl(pa.szl_id, pa.szl_index);
            }
        }

        client.disconnect();

        // ── 6. Extract values (in original channel order) ─────────────────
        auto to_double = [](const ChannelValue& v) -> double {
            return std::visit([](auto&& x) -> double {
                using T = std::decay_t<decltype(x)>;
                if constexpr (std::is_same_v<T, bool>)             return x ? 1.0 : 0.0;
                else if constexpr (std::is_same_v<T, std::string>) return 1.0;
                else                                                return static_cast<double>(x);
            }, v);
        };

        std::vector<ChannelResult> results;
        results.reserve(cfg.channels.size());

        for (size_t i = 0; i < parsed.size(); ++i) {
            auto& pc = parsed[i];
            auto& ch = cfg.channels[pc.cfg_index];
            auto& pa = pc.pa;

            ChannelValue val;

            if (is_memory(pa.area)) {
                GroupKey key{pa.area, pa.db_number};
                auto& buf = buffers.at(key);
                val = extract_value(buf.data, pa.byte_offset - buf.start_byte,
                                    pa.bit_number, ch.datatype);

            } else if (pa.area == S7Area::CPU_STATUS) {
                if      (cpu_state == "RUN")  val = int64_t(1);
                else if (cpu_state == "STOP") val = int64_t(0);
                else                          val = int64_t(2); // UNKNOWN / ERROR

            } else if (pa.area == S7Area::CPU_INFO) {
                switch (pa.cpu_info_field) {
                    case CpuInfoField::MODULE_TYPE_NAME: val = cpu_info_cache->module_type_name; break;
                    case CpuInfoField::SERIAL_NUMBER:    val = cpu_info_cache->serial_number;    break;
                    case CpuInfoField::AS_NAME:          val = cpu_info_cache->as_name;          break;
                    case CpuInfoField::MODULE_NAME:      val = cpu_info_cache->module_name;      break;
                    case CpuInfoField::COPYRIGHT:        val = cpu_info_cache->copyright;        break;
                    default:                             val = std::string("?");
                }

            } else if (pa.area == S7Area::SZL) {
                auto key = std::make_pair(pa.szl_id, pa.szl_index);
                val = extract_value(szl_cache.at(key).data,
                                    pa.byte_offset, 0, ch.datatype);
            } else {
                val = int64_t(0);
            }

            double raw_d  = to_double(val);
            double scaled = std::holds_alternative<std::string>(val)
                            ? 1.0
                            : apply_scale(raw_d, ch.scale_factor, ch.scale_offset);

            LOG("Channel '" << ch.name << "' scaled=" << scaled);

            ChannelResult cr;
            cr.config       = &ch;
            cr.value        = val;
            cr.scaled_value = scaled;
            results.push_back(cr);
        }

        // ── 7. Output ─────────────────────────────────────────────────────
        output_prtg_success(results, "PLC " + cpu_state);

    } catch (const std::exception& e) {
        LOG("Exception: " << e.what());
        output_prtg_error(e.what());
    }

    return 0;
}
