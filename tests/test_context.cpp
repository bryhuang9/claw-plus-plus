#include <gtest/gtest.h>
#include "claw++/runtime/context.hpp"

using namespace claw;

TEST(ContextTest, EstimateTokensString) {
    EXPECT_EQ(estimate_tokens(""), 0);
    EXPECT_EQ(estimate_tokens("abcd"), 1);
    EXPECT_EQ(estimate_tokens("abcdefgh"), 2);
    EXPECT_GT(estimate_tokens(std::string(1000, 'x')), 200);
}

TEST(ContextTest, EstimateTokensMessages) {
    std::vector<Message> msgs;
    EXPECT_EQ(estimate_tokens(msgs), 0);

    msgs.push_back(Message{.role = Role::User, .content = "Hello world"});
    EXPECT_GT(estimate_tokens(msgs), 0);
}

TEST(ContextTest, CompactContextSmall) {
    std::vector<Message> msgs = {
        {.role = Role::User, .content = "Hello"},
        {.role = Role::Assistant, .content = "Hi"}
    };

    // Small enough — should pass through unchanged
    auto result = compact_context(msgs, 100000, 4);
    EXPECT_EQ(result.size(), 2);
}

TEST(ContextTest, CompactContextLarge) {
    std::vector<Message> msgs;
    msgs.push_back(Message{.role = Role::System, .content = "System prompt"});
    for (int i = 0; i < 100; ++i) {
        msgs.push_back(Message{
            .role = (i % 2 == 0) ? Role::User : Role::Assistant,
            .content = std::string(500, 'x')  // ~125 tokens each
        });
    }

    auto result = compact_context(msgs, 200, 4);
    // Should have system + summary + 4 recent
    EXPECT_LE(result.size(), 6u);
    EXPECT_EQ(result[0].role, Role::System);
}

TEST(ContextTest, TruncateToTokens) {
    // 3 tokens * 4 chars = 12 chars max + "... [truncated]" suffix
    auto result = truncate_to_tokens("Hello world this is a test", 3);
    EXPECT_LT(result.size(), 40u);
    EXPECT_NE(result.find("truncated"), std::string::npos);

    auto no_truncate = truncate_to_tokens("Short", 1000);
    EXPECT_EQ(no_truncate, "Short");
}
