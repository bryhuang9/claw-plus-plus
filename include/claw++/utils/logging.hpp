#pragma once

#include <string_view>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace claw::log {

// Initialize the global logger with the given level
void init(std::string_view level = "info");

// Convenience accessors
std::shared_ptr<spdlog::logger>& get();

} // namespace claw::log
