#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "claw++/tools/registry.hpp"

using namespace claw;

class ToolsTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry_.register_builtins();
    }

    ToolRegistry registry_;
};

TEST_F(ToolsTest, RegistryHasBuiltins) {
    EXPECT_GT(registry_.size(), 0);
    EXPECT_NE(registry_.get("shell"), nullptr);
    EXPECT_NE(registry_.get("file_read"), nullptr);
    EXPECT_NE(registry_.get("file_write"), nullptr);
    EXPECT_NE(registry_.get("git"), nullptr);
    EXPECT_NE(registry_.get("search"), nullptr);
    EXPECT_NE(registry_.get("todo"), nullptr);
    EXPECT_NE(registry_.get("notebook"), nullptr);
    EXPECT_NE(registry_.get("web_fetch"), nullptr);
}

TEST_F(ToolsTest, UnknownTool) {
    ToolCall call{.id = "1", .name = "nonexistent", .arguments_json = "{}"};
    auto result = registry_.execute(call);
    EXPECT_FALSE(result.has_value());
}

TEST_F(ToolsTest, ShellEcho) {
    ToolCall call{.id = "1", .name = "shell", .arguments_json = R"({"command": "echo hello"})"};
    auto result = registry_.execute(call);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->success);
    EXPECT_NE(result->output.find("hello"), std::string::npos);
}

TEST_F(ToolsTest, FileReadWriteRoundTrip) {
    auto tmp = std::filesystem::temp_directory_path() / "claw_test_file.txt";
    // Use generic_string() to get forward slashes, safe for JSON embedding
    auto tmp_str = tmp.generic_string();

    // Write
    ToolCall write_call{
        .id = "1", .name = "file_write",
        .arguments_json = R"({"path": ")" + tmp_str + R"(", "content": "test content line 2"})"
    };
    auto write_result = registry_.execute(write_call);
    ASSERT_TRUE(write_result.has_value()) << write_result.error().message();
    EXPECT_TRUE(write_result->success);

    // Read
    ToolCall read_call{
        .id = "2", .name = "file_read",
        .arguments_json = R"({"path": ")" + tmp_str + R"("})"
    };
    auto read_result = registry_.execute(read_call);
    ASSERT_TRUE(read_result.has_value());
    EXPECT_TRUE(read_result->success);
    EXPECT_NE(read_result->output.find("test content"), std::string::npos);

    // Cleanup
    std::filesystem::remove(tmp);
}

TEST_F(ToolsTest, TodoAddAndList) {
    // Add
    ToolCall add_call{
        .id = "1", .name = "todo",
        .arguments_json = R"({"action": "add", "content": "Test task", "priority": "high"})"
    };
    auto add_result = registry_.execute(add_call);
    ASSERT_TRUE(add_result.has_value());
    EXPECT_TRUE(add_result->success);

    // List
    ToolCall list_call{
        .id = "2", .name = "todo",
        .arguments_json = R"({"action": "list"})"
    };
    auto list_result = registry_.execute(list_call);
    ASSERT_TRUE(list_result.has_value());
    EXPECT_NE(list_result->output.find("Test task"), std::string::npos);

    // Clear
    ToolCall clear_call{
        .id = "3", .name = "todo",
        .arguments_json = R"({"action": "clear"})"
    };
    registry_.execute(clear_call);
}

TEST_F(ToolsTest, ManifestsNotEmpty) {
    auto manifests = registry_.manifests();
    EXPECT_GT(manifests.size(), 0);

    for (const auto& m : manifests) {
        EXPECT_FALSE(m.name.empty());
        EXPECT_FALSE(m.description.empty());
    }
}

TEST_F(ToolsTest, JsonSchemas) {
    auto schemas = registry_.json_schemas();
    EXPECT_GT(schemas.size(), 0);

    for (const auto& s : schemas) {
        EXPECT_TRUE(s.contains("name"));
        EXPECT_TRUE(s.contains("description"));
        EXPECT_TRUE(s.contains("input_schema"));
    }
}
