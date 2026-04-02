#include <gtest/gtest.h>
#include "claw++/core/config.hpp"

using namespace claw;

TEST(ConfigTest, DefaultValues) {
    Config cfg;
    EXPECT_EQ(cfg.provider, "anthropic");
    EXPECT_EQ(cfg.model, "claude-sonnet-4-20250514");
    EXPECT_EQ(cfg.ollama_host, "http://localhost:11434");
    EXPECT_TRUE(cfg.auto_save);
    EXPECT_TRUE(cfg.color);
    EXPECT_TRUE(cfg.streaming);
    EXPECT_TRUE(cfg.allow_shell);
    EXPECT_TRUE(cfg.allow_write);
    EXPECT_TRUE(cfg.allow_network);
    EXPECT_EQ(cfg.max_tokens, 4096);
}

TEST(ConfigTest, JsonRoundTrip) {
    Config cfg;
    cfg.provider = "openai";
    cfg.model = "gpt-4o";
    cfg.max_tokens = 8192;

    nlohmann::json j = cfg;
    auto restored = j.get<Config>();

    EXPECT_EQ(restored.provider, "openai");
    EXPECT_EQ(restored.model, "gpt-4o");
    EXPECT_EQ(restored.max_tokens, 8192);
}

TEST(ConfigTest, GlobalConfigDir) {
    auto dir = global_config_dir();
    EXPECT_FALSE(dir.empty());
    // Should end with "claw++"
    EXPECT_EQ(dir.filename().string(), "claw++");
}

TEST(ConfigTest, ResolveConfigPaths) {
    Config cfg;
    resolve_config_paths(cfg);
    EXPECT_FALSE(cfg.session_dir.empty());
    EXPECT_FALSE(cfg.workspace_root.empty());
}
