#include "claw++/tools/tool.hpp"
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <regex>

namespace claw {

class WebFetchTool : public ITool {
public:
    WebFetchTool() {
        manifest_ = ToolManifest{
            .name = "web_fetch",
            .description = "Fetch the contents of a URL. Returns the response body as text.",
            .parameters = {
                {"url", "string", "The URL to fetch (must start with http:// or https://)", true},
                {"max_length", "integer", "Maximum response length in characters (default: 10000)", false}
            }
        };
    }

    const ToolManifest& manifest() const override { return manifest_; }

    Result<ToolResult> execute(const nlohmann::json& args) override {
        auto url        = args.at("url").get<std::string>();
        auto max_length = args.value("max_length", 10000);

        // Parse URL into host + path
        std::regex url_regex(R"(^(https?)://([^/]+)(/.*)?)");
        std::smatch match;
        if (!std::regex_match(url, match, url_regex)) {
            return ToolResult{.tool_call_id={}, .success=false, .output={},
                              .error="Invalid URL format: " + url};
        }

        auto scheme = match[1].str();
        auto host   = match[2].str();
        auto path   = match[3].str();
        if (path.empty()) path = "/";

        spdlog::info("WebFetch: {} {}{}", scheme, host, path);

        try {
            std::unique_ptr<httplib::Client> cli;
            if (scheme == "https") {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
                cli = std::make_unique<httplib::Client>(scheme + "://" + host);
#else
                cli = std::make_unique<httplib::Client>(scheme + "://" + host);
#endif
            } else {
                cli = std::make_unique<httplib::Client>("http://" + host);
            }

            cli->set_follow_location(true);
            cli->set_read_timeout(30);
            cli->set_default_headers({
                {"User-Agent", "claw++/0.1.0"}
            });

            auto res = cli->Get(path);
            if (!res) {
                return ToolResult{.tool_call_id={}, .success=false, .output={},
                                  .error="HTTP request failed for: " + url};
            }

            std::string body = res->body;
            if (static_cast<int>(body.size()) > max_length) {
                body = body.substr(0, static_cast<size_t>(max_length)) + "\n... (truncated)";
            }

            return ToolResult{
                .tool_call_id = {},
                .success = (res->status >= 200 && res->status < 400),
                .output = fmt::format("HTTP {} | Content-Length: {}\n\n{}", res->status, res->body.size(), body),
                .error = (res->status >= 400) ? fmt::format("HTTP {}", res->status) : ""
            };
        } catch (const std::exception& e) {
            return ToolResult{.tool_call_id={}, .success=false, .output={},
                              .error=std::string("Fetch error: ") + e.what()};
        }
    }

private:
    ToolManifest manifest_;
};

std::unique_ptr<ITool> create_web_fetch_tool() {
    return std::make_unique<WebFetchTool>();
}

} // namespace claw
