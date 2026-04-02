#include "claw++/utils/string_utils.hpp"
#include <algorithm>
#include <sstream>
#include <cctype>

namespace claw::str {

std::string trim(std::string_view s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string_view::npos) return {};
    auto end = s.find_last_not_of(" \t\n\r");
    return std::string(s.substr(start, end - start + 1));
}

std::vector<std::string> split(std::string_view s, char delim) {
    std::vector<std::string> parts;
    std::string token;
    std::istringstream stream{std::string(s)};
    while (std::getline(stream, token, delim)) {
        parts.push_back(token);
    }
    return parts;
}

bool starts_with(std::string_view s, std::string_view prefix) {
    return s.starts_with(prefix);
}

bool ends_with(std::string_view s, std::string_view suffix) {
    return s.ends_with(suffix);
}

std::string to_lower(std::string_view s) {
    std::string result(s);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

std::string join(const std::vector<std::string>& parts, std::string_view sep) {
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result.append(sep);
        result.append(parts[i]);
    }
    return result;
}

std::string truncate(std::string_view s, size_t max_len, std::string_view ellipsis) {
    if (s.size() <= max_len) return std::string(s);
    if (max_len <= ellipsis.size()) return std::string(ellipsis.substr(0, max_len));
    return std::string(s.substr(0, max_len - ellipsis.size())) + std::string(ellipsis);
}

} // namespace claw::str
