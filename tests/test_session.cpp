#include <gtest/gtest.h>
#include "claw++/session/session.hpp"

using namespace claw;

TEST(SessionTest, GenerateId) {
    auto id1 = Session::generate_id();
    auto id2 = Session::generate_id();
    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_NE(id1, id2);
}

TEST(SessionTest, AddMessage) {
    Session s;
    s.id = Session::generate_id();
    s.created_at = std::chrono::system_clock::now();

    s.add_message(Message{.role = Role::User, .content = "Hello"});
    EXPECT_EQ(s.messages.size(), 1);
    EXPECT_EQ(s.turn_count, 1);

    s.add_message(Message{.role = Role::Assistant, .content = "Hi there"});
    EXPECT_EQ(s.messages.size(), 2);
    EXPECT_EQ(s.turn_count, 2);
}

TEST(SessionTest, GetContext) {
    Session s;
    for (int i = 0; i < 10; ++i) {
        s.add_message(Message{
            .role = (i % 2 == 0) ? Role::User : Role::Assistant,
            .content = "Message " + std::to_string(i)
        });
    }

    auto all = s.get_context(0);
    EXPECT_EQ(all.size(), 10);

    auto last3 = s.get_context(3);
    EXPECT_EQ(last3.size(), 3);
    EXPECT_EQ(last3[0].content, "Message 7");
}

TEST(SessionTest, JsonRoundTrip) {
    Session s;
    s.id = "test-session-123";
    s.title = "Test Session";
    s.model = "test-model";
    s.provider = "test-provider";
    s.created_at = std::chrono::system_clock::now();
    s.updated_at = s.created_at;
    s.total_tokens_in = 100;
    s.total_tokens_out = 200;
    s.turn_count = 5;

    s.add_message(Message{.role = Role::User, .content = "Hello"});
    s.add_message(Message{.role = Role::Assistant, .content = "Hi"});

    auto j = s.to_json();
    auto restored = Session::from_json(j);

    ASSERT_TRUE(restored.has_value());
    EXPECT_EQ(restored->id, "test-session-123");
    EXPECT_EQ(restored->title, "Test Session");
    EXPECT_EQ(restored->model, "test-model");
    EXPECT_EQ(restored->messages.size(), 2);
    EXPECT_EQ(restored->messages[0].content, "Hello");
    EXPECT_EQ(restored->messages[1].content, "Hi");
}
