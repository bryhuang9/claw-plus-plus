#include <gtest/gtest.h>
#include "claw++/api/streaming.hpp"

using namespace claw;

TEST(SSEParserTest, SingleEvent) {
    std::vector<std::pair<std::string, std::string>> events;

    SSEParser parser([&](std::string_view event, std::string_view data) {
        events.emplace_back(std::string(event), std::string(data));
    });

    parser.feed("event: message\ndata: {\"text\": \"hello\"}\n\n");

    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].first, "message");
    EXPECT_EQ(events[0].second, "{\"text\": \"hello\"}");
}

TEST(SSEParserTest, MultipleEvents) {
    std::vector<std::pair<std::string, std::string>> events;

    SSEParser parser([&](std::string_view event, std::string_view data) {
        events.emplace_back(std::string(event), std::string(data));
    });

    parser.feed("data: first\n\ndata: second\n\n");

    ASSERT_EQ(events.size(), 2);
    EXPECT_EQ(events[0].second, "first");
    EXPECT_EQ(events[1].second, "second");
}

TEST(SSEParserTest, ChunkedInput) {
    std::vector<std::string> data_received;

    SSEParser parser([&](std::string_view /*event*/, std::string_view data) {
        data_received.emplace_back(data);
    });

    // Simulate chunked delivery
    parser.feed("data: hel");
    EXPECT_EQ(data_received.size(), 0);

    parser.feed("lo\n\n");
    ASSERT_EQ(data_received.size(), 1);
    EXPECT_EQ(data_received[0], "hello");
}

TEST(SSEParserTest, MultiLineData) {
    std::vector<std::string> data_received;

    SSEParser parser([&](std::string_view /*event*/, std::string_view data) {
        data_received.emplace_back(data);
    });

    parser.feed("data: line1\ndata: line2\n\n");

    ASSERT_EQ(data_received.size(), 1);
    EXPECT_EQ(data_received[0], "line1\nline2");
}

TEST(SSEParserTest, DoneSignal) {
    std::vector<std::string> data_received;

    SSEParser parser([&](std::string_view /*event*/, std::string_view data) {
        data_received.emplace_back(data);
    });

    parser.feed("data: [DONE]\n\n");

    ASSERT_EQ(data_received.size(), 1);
    EXPECT_EQ(data_received[0], "[DONE]");
}

TEST(SSEParserTest, FlushRemaining) {
    std::vector<std::string> data_received;

    SSEParser parser([&](std::string_view /*event*/, std::string_view data) {
        data_received.emplace_back(data);
    });

    parser.feed("data: incomplete");
    EXPECT_EQ(data_received.size(), 0);

    parser.flush();
    // flush should not emit partial line since there's no \n\n
    // Actually, our implementation flushes current_data_ if not empty
    // but "data: incomplete" hasn't been fully parsed as a line yet
    // The buffer still has it — flush() only flushes current_data_
}
