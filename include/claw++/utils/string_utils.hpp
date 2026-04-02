#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace claw::str {

std::string              trim(std::string_view s);
std::vector<std::string> split(std::string_view s, char delim);
bool                     starts_with(std::string_view s, std::string_view prefix);
bool                     ends_with(std::string_view s, std::string_view suffix);
std::string              to_lower(std::string_view s);
std::string              join(const std::vector<std::string>& parts, std::string_view sep);
std::string              truncate(std::string_view s, size_t max_len, std::string_view ellipsis = "...");

} // namespace claw::str
