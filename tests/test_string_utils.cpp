#include <gtest/gtest.h>
#include "claw++/utils/string_utils.hpp"

using namespace claw::str;

TEST(StringUtilsTest, Trim) {
    EXPECT_EQ(trim("  hello  "), "hello");
    EXPECT_EQ(trim("\t\nhello\n\t"), "hello");
    EXPECT_EQ(trim(""), "");
    EXPECT_EQ(trim("   "), "");
    EXPECT_EQ(trim("no_trim"), "no_trim");
}

TEST(StringUtilsTest, Split) {
    auto parts = split("a,b,c", ',');
    ASSERT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");

    auto single = split("hello", ',');
    ASSERT_EQ(single.size(), 1);
    EXPECT_EQ(single[0], "hello");
}

TEST(StringUtilsTest, StartsWith) {
    EXPECT_TRUE(starts_with("/help", "/"));
    EXPECT_TRUE(starts_with("hello world", "hello"));
    EXPECT_FALSE(starts_with("hello", "world"));
    EXPECT_TRUE(starts_with("", ""));
}

TEST(StringUtilsTest, EndsWith) {
    EXPECT_TRUE(ends_with("hello.cpp", ".cpp"));
    EXPECT_FALSE(ends_with("hello.cpp", ".hpp"));
    EXPECT_TRUE(ends_with("", ""));
}

TEST(StringUtilsTest, ToLower) {
    EXPECT_EQ(to_lower("HELLO"), "hello");
    EXPECT_EQ(to_lower("Hello World"), "hello world");
    EXPECT_EQ(to_lower("already"), "already");
}

TEST(StringUtilsTest, Join) {
    EXPECT_EQ(join({"a", "b", "c"}, ", "), "a, b, c");
    EXPECT_EQ(join({"single"}, ", "), "single");
    EXPECT_EQ(join({}, ", "), "");
}

TEST(StringUtilsTest, Truncate) {
    EXPECT_EQ(truncate("Hello world", 5), "He...");
    EXPECT_EQ(truncate("Hi", 10), "Hi");
    EXPECT_EQ(truncate("Hello world", 11), "Hello world");
}
