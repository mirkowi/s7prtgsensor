#pragma once
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void enable(const std::string& path) {
        file_.open(path, std::ios::app);
        enabled_ = file_.is_open();
    }

    bool enabled() const { return enabled_; }

    void log(const std::string& msg) {
        if (!enabled_) return;
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf{};
#ifdef _WIN32
        localtime_s(&tm_buf, &t);
#else
        localtime_r(&t, &tm_buf);
#endif
        file_ << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << " " << msg << "\n";
        file_.flush();
    }

private:
    Logger() = default;
    std::ofstream file_;
    bool enabled_ = false;
};

#define LOG(msg) do { \
    if (Logger::instance().enabled()) { \
        std::ostringstream _oss; \
        _oss << msg; \
        Logger::instance().log(_oss.str()); \
    } \
} while(0)
