#include "claw++/utils/logging.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace claw::log {

static std::shared_ptr<spdlog::logger> g_logger;

void init(std::string_view level) {
    if (!g_logger) {
        g_logger = spdlog::stderr_color_mt("claw++");
    }

    auto lvl = spdlog::level::info;
    if (level == "trace")      lvl = spdlog::level::trace;
    else if (level == "debug") lvl = spdlog::level::debug;
    else if (level == "info")  lvl = spdlog::level::info;
    else if (level == "warn")  lvl = spdlog::level::warn;
    else if (level == "error") lvl = spdlog::level::err;

    g_logger->set_level(lvl);
    g_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_default_logger(g_logger);
}

std::shared_ptr<spdlog::logger>& get() {
    if (!g_logger) init();
    return g_logger;
}

} // namespace claw::log
