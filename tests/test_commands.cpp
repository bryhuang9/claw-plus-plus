#include <gtest/gtest.h>
#include "claw++/commands/registry.hpp"
#include "claw++/session/session.hpp"
#include "claw++/core/config.hpp"
#include "claw++/tools/registry.hpp"

using namespace claw;

class CommandsTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry_.register_builtins();
        session_.id = "test-session";
        session_.created_at = std::chrono::system_clock::now();
        config_ = Config{};
        tools_ = ToolRegistry{};
        tools_.register_builtins();
    }

    CommandRegistry registry_;
    Session session_;
    Config config_;
    ToolRegistry tools_;

    CommandContext make_ctx() {
        return CommandContext{
            .session = &session_,
            .config = &config_,
            .tools = &tools_,
            .print = [this](std::string_view msg) { last_output_ += msg; }
        };
    }

    std::string last_output_;
};

TEST_F(CommandsTest, IsCommand) {
    EXPECT_TRUE(registry_.is_command("/help"));
    EXPECT_TRUE(registry_.is_command("/status"));
    EXPECT_FALSE(registry_.is_command("hello"));
    EXPECT_FALSE(registry_.is_command(""));
}

TEST_F(CommandsTest, HelpCommand) {
    auto ctx = make_ctx();
    auto result = registry_.dispatch(ctx, "/help");
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(last_output_.find("Available commands"), std::string::npos);
}

TEST_F(CommandsTest, StatusCommand) {
    auto ctx = make_ctx();
    auto result = registry_.dispatch(ctx, "/status");
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(last_output_.find("Session"), std::string::npos);
}

TEST_F(CommandsTest, ClearCommand) {
    session_.add_message(Message{.role = Role::User, .content = "test"});
    EXPECT_EQ(session_.messages.size(), 1);

    auto ctx = make_ctx();
    registry_.dispatch(ctx, "/clear");
    EXPECT_EQ(session_.messages.size(), 0);
    EXPECT_EQ(session_.turn_count, 0);
}

TEST_F(CommandsTest, SessionNewCommand) {
    auto old_id = session_.id;
    auto ctx = make_ctx();
    registry_.dispatch(ctx, "/session new");
    EXPECT_NE(session_.id, old_id);
}

TEST_F(CommandsTest, UnknownCommand) {
    auto ctx = make_ctx();
    auto result = registry_.dispatch(ctx, "/nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST_F(CommandsTest, ConfigCommand) {
    auto ctx = make_ctx();
    auto result = registry_.dispatch(ctx, "/config");
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(last_output_.find("configuration"), std::string::npos);
}
