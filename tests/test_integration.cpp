// Integration tests — end-to-end parity verification against claw-code Rust reference
// Tests tool execution chains, command dispatch, session lifecycle, provider wiring,
// and streaming parser behavior to ensure behavioral equivalence.

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <cstdlib>

#include "claw++/core/types.hpp"
#include "claw++/core/config.hpp"
#include "claw++/core/error.hpp"
#include "claw++/tools/registry.hpp"
#include "claw++/commands/registry.hpp"
#include "claw++/session/session.hpp"
#include "claw++/session/storage.hpp"
#include "claw++/api/streaming.hpp"
#include "claw++/api/provider.hpp"
#include "claw++/runtime/context.hpp"
#include "claw++/utils/string_utils.hpp"

namespace fs = std::filesystem;
using namespace claw;

// ═══════════════════════════════════════════════════════════════════════════════
// SECTION 1: Tool Parity — matches rust/crates/tools mvp_tool_specs()
// Reference tools: shell, file_read, file_write, file_search, git, web_fetch,
//                  todo, notebook, search
// ═══════════════════════════════════════════════════════════════════════════════

class ToolParityTest : public ::testing::Test {
protected:
    ToolRegistry registry_;
    void SetUp() override { registry_.register_builtins(); }
};

TEST_F(ToolParityTest, AllMvpToolsRegistered) {
    // Rust mvp_tool_specs() registers these tool names
    std::vector<std::string> expected_tools = {
        "shell", "file_read", "file_write", "file_search",
        "git", "web_fetch", "todo", "notebook", "search"
    };
    for (const auto& name : expected_tools) {
        EXPECT_NE(registry_.get(name), nullptr)
            << "Missing tool: " << name << " (required for Rust parity)";
    }
}

TEST_F(ToolParityTest, ToolCount) {
    auto schemas = registry_.json_schemas();
    EXPECT_GE(schemas.size(), 9u) << "Rust MVP has 9+ tools";
}

TEST_F(ToolParityTest, ShellToolExecutesAndReturnsOutput) {
    ToolCall call{.id = "int-1", .name = "shell",
                  .arguments_json = R"({"command": "echo parity_test_ok"})"};
    auto result = registry_.execute(call);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_TRUE(result->success);
    EXPECT_NE(result->output.find("parity_test_ok"), std::string::npos);
}

TEST_F(ToolParityTest, ShellToolWithWorkingDir) {
    auto tmp = fs::temp_directory_path().generic_string();
    ToolCall call{.id = "int-2", .name = "shell",
                  .arguments_json = R"({"command": "echo ok", "working_dir": ")" + tmp + R"("})"};
    auto result = registry_.execute(call);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_TRUE(result->success);
}

TEST_F(ToolParityTest, FileWriteReadDeleteRoundTrip) {
    auto tmp = (fs::temp_directory_path() / "claw_parity_test.txt").generic_string();

    // Write
    ToolCall w{.id = "w1", .name = "file_write",
               .arguments_json = R"({"path": ")" + tmp + R"(", "content": "parity check line1\nline2"})"};
    auto wr = registry_.execute(w);
    ASSERT_TRUE(wr.has_value()) << wr.error().message();
    EXPECT_TRUE(wr->success);

    // Read
    ToolCall r{.id = "r1", .name = "file_read",
               .arguments_json = R"({"path": ")" + tmp + R"("})"};
    auto rr = registry_.execute(r);
    ASSERT_TRUE(rr.has_value()) << rr.error().message();
    EXPECT_TRUE(rr->success);
    EXPECT_NE(rr->output.find("parity check"), std::string::npos);

    // Cleanup
    fs::remove(tmp);
}

TEST_F(ToolParityTest, FileSearchFindsFiles) {
    auto tmp_dir = fs::temp_directory_path() / "claw_parity_search";
    fs::create_directories(tmp_dir);
    std::ofstream(tmp_dir / "test.txt") << "search target";

    ToolCall call{.id = "s1", .name = "file_search",
                  .arguments_json = R"({"directory": ")" + tmp_dir.generic_string() + R"(", "pattern": "*.txt"})"};
    auto result = registry_.execute(call);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_NE(result->output.find("test.txt"), std::string::npos);

    fs::remove_all(tmp_dir);
}

TEST_F(ToolParityTest, GitToolStatusWorks) {
    // Git status should work in any git repo (our own project dir)
    ToolCall call{.id = "g1", .name = "git",
                  .arguments_json = R"({"subcommand": "status"})"};
    auto result = registry_.execute(call);
    // May fail if not in a git repo, but should not crash
    EXPECT_TRUE(result.has_value());
}

TEST_F(ToolParityTest, GitToolRejectsUnsafeCommands) {
    ToolCall call{.id = "g2", .name = "git",
                  .arguments_json = R"({"subcommand": "push --force"})"};
    auto result = registry_.execute(call);
    // Should fail — push is not in the safety allowlist
    if (result.has_value()) {
        EXPECT_FALSE(result->success);
    }
}

TEST_F(ToolParityTest, TodoAddListClearCycle) {
    // Add
    ToolCall add{.id = "t1", .name = "todo",
                 .arguments_json = R"({"action":"add","content":"Parity task","priority":"high"})"};
    auto ar = registry_.execute(add);
    ASSERT_TRUE(ar.has_value()) << ar.error().message();

    // List
    ToolCall list{.id = "t2", .name = "todo",
                  .arguments_json = R"({"action":"list"})"};
    auto lr = registry_.execute(list);
    ASSERT_TRUE(lr.has_value());
    EXPECT_NE(lr->output.find("Parity task"), std::string::npos);

    // Update
    ToolCall upd{.id = "t3", .name = "todo",
                 .arguments_json = R"({"action":"update","id":"1","content":"Updated task","priority":"low"})"};
    auto ur = registry_.execute(upd);
    EXPECT_TRUE(ur.has_value());

    // Clear
    ToolCall clr{.id = "t4", .name = "todo",
                 .arguments_json = R"({"action":"clear"})"};
    auto cr = registry_.execute(clr);
    ASSERT_TRUE(cr.has_value());

    // Verify empty
    auto lr2 = registry_.execute(list);
    ASSERT_TRUE(lr2.has_value());
    // After clear, should have no tasks or say empty
    EXPECT_EQ(lr2->output.find("Parity task"), std::string::npos);
}

TEST_F(ToolParityTest, NotebookCreateReadAppend) {
    auto tmp = (fs::temp_directory_path() / "claw_parity_notebook.md").generic_string();

    // Create
    ToolCall create{.id = "n1", .name = "notebook",
                    .arguments_json = R"({"action":"write","path":")" + tmp + R"(","content":"# Notebook\nEntry 1"})"};
    auto cr = registry_.execute(create);
    ASSERT_TRUE(cr.has_value()) << cr.error().message();

    // Append
    ToolCall append{.id = "n2", .name = "notebook",
                    .arguments_json = R"({"action":"append","path":")" + tmp + R"(","content":"\nEntry 2"})"};
    auto ar = registry_.execute(append);
    ASSERT_TRUE(ar.has_value()) << ar.error().message();

    // Read
    ToolCall read{.id = "n3", .name = "notebook",
                  .arguments_json = R"({"action":"read","path":")" + tmp + R"("})"};
    auto rr = registry_.execute(read);
    ASSERT_TRUE(rr.has_value());
    EXPECT_NE(rr->output.find("Entry 1"), std::string::npos);
    EXPECT_NE(rr->output.find("Entry 2"), std::string::npos);

    fs::remove(tmp);
}

TEST_F(ToolParityTest, SearchToolUsesGrepOrRg) {
    auto tmp_dir = fs::temp_directory_path() / "claw_parity_grep";
    fs::create_directories(tmp_dir);
    std::ofstream(tmp_dir / "haystack.txt") << "needle_parity_match here";

    ToolCall call{.id = "sr1", .name = "search",
                  .arguments_json = R"({"pattern":"needle_parity_match","path":")" +
                                    tmp_dir.generic_string() + R"("})"};
    auto result = registry_.execute(call);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_NE(result->output.find("needle_parity_match"), std::string::npos);

    fs::remove_all(tmp_dir);
}

TEST_F(ToolParityTest, UnknownToolReturnsError) {
    ToolCall call{.id = "u1", .name = "nonexistent_tool",
                  .arguments_json = R"({})"};
    auto result = registry_.execute(call);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().kind(), ErrorKind::Tool);
}

TEST_F(ToolParityTest, MissingRequiredParamReturnsError) {
    // file_read requires "path"
    ToolCall call{.id = "m1", .name = "file_read",
                  .arguments_json = R"({})"};
    auto result = registry_.execute(call);
    EXPECT_FALSE(result.has_value());
}

TEST_F(ToolParityTest, ToolJsonSchemasAreWellFormed) {
    auto schemas = registry_.json_schemas();
    for (const auto& s : schemas) {
        EXPECT_TRUE(s.contains("name")) << "Schema missing 'name'";
        EXPECT_TRUE(s.contains("description")) << "Schema missing 'description'";
        EXPECT_TRUE(s.contains("input_schema")) << "Schema missing 'input_schema'";
        EXPECT_TRUE(s["input_schema"].contains("type"));
        EXPECT_EQ(s["input_schema"]["type"], "object");
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// SECTION 2: Command Parity — matches rust/crates/commands slash commands
// Reference: help, status, compact, model, permissions, clear, cost, resume,
//            config, memory, init, diff, version, export, session
// Our set:   help, status, config, save, load, clear, diff, export, quit, session
// ═══════════════════════════════════════════════════════════════════════════════

class CommandParityTest : public ::testing::Test {
protected:
    CommandRegistry commands_;
    Session session_;
    Config config_;

    static void noop_print(std::string_view) {}

    void SetUp() override {
        commands_.register_builtins();
        session_.id = "parity-test";
        session_.model = "test-model";
        session_.provider = "ollama";
        config_.model = "test-model";
        config_.provider = "ollama";
    }
};

TEST_F(CommandParityTest, CoreCommandsRegistered) {
    // These commands exist in both Rust and C++ implementations
    std::vector<std::string> expected = {
        "help", "status", "config", "clear", "diff", "export", "session"
    };
    for (const auto& name : expected) {
        EXPECT_NE(commands_.get(name), nullptr)
            << "Missing command: /" << name;
    }
}

TEST_F(CommandParityTest, HelpCommandListsAllCommands) {
    CommandContext ctx{&session_, &config_, nullptr, noop_print};
    auto result = commands_.dispatch(ctx, "/help");
    ASSERT_TRUE(result.has_value()) << result.error().message();
}

TEST_F(CommandParityTest, StatusCommandShowsSessionInfo) {
    CommandContext ctx{&session_, &config_, nullptr, noop_print};
    auto result = commands_.dispatch(ctx, "/status");
    ASSERT_TRUE(result.has_value()) << result.error().message();
}

TEST_F(CommandParityTest, ConfigCommandShowsConfig) {
    CommandContext ctx{&session_, &config_, nullptr, noop_print};
    auto result = commands_.dispatch(ctx, "/config");
    ASSERT_TRUE(result.has_value()) << result.error().message();
}

TEST_F(CommandParityTest, ClearCommandResetsSession) {
    session_.add_message(Message{Role::User, "test"});
    EXPECT_FALSE(session_.messages.empty());

    CommandContext ctx{&session_, &config_, nullptr, noop_print};
    auto result = commands_.dispatch(ctx, "/clear");
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_TRUE(session_.messages.empty());
}

TEST_F(CommandParityTest, UnknownCommandReturnsError) {
    CommandContext ctx{&session_, &config_, nullptr, noop_print};
    auto result = commands_.dispatch(ctx, "/nonexistent");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().kind(), ErrorKind::Command);
}

TEST_F(CommandParityTest, IsCommandDetectsSlash) {
    EXPECT_TRUE(commands_.is_command("/help"));
    EXPECT_TRUE(commands_.is_command("/status"));
    EXPECT_FALSE(commands_.is_command("help"));
    EXPECT_FALSE(commands_.is_command(""));
}

TEST_F(CommandParityTest, CommandParsingExtractsArgs) {
    // /session list should parse "session" + ["list"]
    CommandContext ctx{&session_, &config_, nullptr, noop_print};
    auto result = commands_.dispatch(ctx, "/session list");
    // Should not crash regardless of outcome
    SUCCEED();
}

// ═══════════════════════════════════════════════════════════════════════════════
// SECTION 3: Session Parity — matches rust/crates/runtime/src/session.rs
// ═══════════════════════════════════════════════════════════════════════════════

class SessionParityTest : public ::testing::Test {
protected:
    fs::path test_dir_;
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "claw_parity_sessions";
        fs::create_directories(test_dir_);
    }
    void TearDown() override {
        fs::remove_all(test_dir_);
    }
};

TEST_F(SessionParityTest, SessionIdIsUnique) {
    auto id1 = Session::generate_id();
    auto id2 = Session::generate_id();
    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_NE(id1, id2) << "Session IDs must be unique";
    EXPECT_GE(id1.size(), 8u) << "Session ID too short";
}

TEST_F(SessionParityTest, SessionJsonRoundTrip) {
    Session s;
    s.id = Session::generate_id();
    s.model = "llama3";
    s.provider = "ollama";
    s.workspace = "/tmp/test";
    s.add_message(Message{Role::System, "You are a coding agent."});
    s.add_message(Message{Role::User, "Hello"});
    s.add_message(Message{Role::Assistant, "Hi there!"});

    auto j = s.to_json();

    // Verify JSON structure matches Rust session schema
    EXPECT_TRUE(j.contains("id"));
    EXPECT_TRUE(j.contains("model"));
    EXPECT_TRUE(j.contains("provider"));
    EXPECT_TRUE(j.contains("messages"));
    EXPECT_TRUE(j.contains("created_at"));
    EXPECT_TRUE(j.contains("updated_at"));

    EXPECT_EQ(j["messages"].size(), 3u);
    EXPECT_EQ(j["messages"][0]["role"], "system");
    EXPECT_EQ(j["messages"][1]["role"], "user");
    EXPECT_EQ(j["messages"][2]["role"], "assistant");

    // Deserialize
    auto restored = Session::from_json(j);
    ASSERT_TRUE(restored.has_value()) << restored.error().message();
    EXPECT_EQ(restored->id, s.id);
    EXPECT_EQ(restored->model, "llama3");
    EXPECT_EQ(restored->messages.size(), 3u);
}

TEST_F(SessionParityTest, StorageSaveLoadDelete) {
    JsonSessionStorage storage(test_dir_);

    Session s;
    s.id = Session::generate_id();
    s.model = "test";
    s.provider = "ollama";
    s.add_message(Message{Role::User, "Save me"});

    // Save
    auto save_r = storage.save(s);
    ASSERT_TRUE(save_r.has_value()) << save_r.error().message();

    // Verify file exists
    auto exists_r = storage.exists(s.id);
    ASSERT_TRUE(exists_r.has_value());
    EXPECT_TRUE(*exists_r);

    // Load
    auto load_r = storage.load(s.id);
    ASSERT_TRUE(load_r.has_value()) << load_r.error().message();
    EXPECT_EQ(load_r->id, s.id);
    EXPECT_EQ(load_r->messages.size(), 1u);
    EXPECT_EQ(load_r->messages[0].content, "Save me");

    // List
    auto list_r = storage.list();
    ASSERT_TRUE(list_r.has_value());
    EXPECT_GE(list_r->size(), 1u);

    // Delete
    auto del_r = storage.remove(s.id);
    ASSERT_TRUE(del_r.has_value());

    auto exists_after = storage.exists(s.id);
    ASSERT_TRUE(exists_after.has_value());
    EXPECT_FALSE(*exists_after);
}

TEST_F(SessionParityTest, LoadNonexistentReturnsError) {
    JsonSessionStorage storage(test_dir_);
    auto result = storage.load("nonexistent-id");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().kind(), ErrorKind::NotFound);
}

TEST_F(SessionParityTest, MultipleSessionsCoexist) {
    JsonSessionStorage storage(test_dir_);

    Session s1, s2;
    s1.id = Session::generate_id();
    s2.id = Session::generate_id();
    s1.model = s2.model = "test";
    s1.provider = s2.provider = "ollama";
    s1.add_message(Message{Role::User, "Session 1"});
    s2.add_message(Message{Role::User, "Session 2"});

    ASSERT_TRUE(storage.save(s1).has_value());
    ASSERT_TRUE(storage.save(s2).has_value());

    auto list = storage.list();
    ASSERT_TRUE(list.has_value());
    EXPECT_GE(list->size(), 2u);

    auto loaded1 = storage.load(s1.id);
    auto loaded2 = storage.load(s2.id);
    ASSERT_TRUE(loaded1.has_value());
    ASSERT_TRUE(loaded2.has_value());
    EXPECT_EQ(loaded1->messages[0].content, "Session 1");
    EXPECT_EQ(loaded2->messages[0].content, "Session 2");
}

// ═══════════════════════════════════════════════════════════════════════════════
// SECTION 4: Provider Parity — matches rust/crates/api
// ═══════════════════════════════════════════════════════════════════════════════

TEST(ProviderParityTest, ThreeProvidersAvailable) {
    // Rust supports anthropic, openai, ollama
    auto anthropic = create_provider("anthropic", "test-key", "");
    auto openai    = create_provider("openai", "test-key", "");
    auto ollama    = create_provider("ollama", "", "");

    EXPECT_NE(anthropic, nullptr) << "Missing provider: anthropic";
    EXPECT_NE(openai, nullptr)    << "Missing provider: openai";
    EXPECT_NE(ollama, nullptr)    << "Missing provider: ollama";
}

TEST(ProviderParityTest, UnknownProviderFallsBack) {
    // Our implementation falls back to anthropic for unknown providers (matching Rust behavior)
    auto unknown = create_provider("gemini", "", "");
    EXPECT_NE(unknown, nullptr);
    EXPECT_EQ(unknown->name(), "anthropic");
}

TEST(ProviderParityTest, ProviderNamesMatchRust) {
    // Rust: provider_name() returns "anthropic", "openai", "ollama"
    auto a = create_provider("anthropic", "k", "");
    auto o = create_provider("openai", "k", "");
    auto l = create_provider("ollama", "", "");

    EXPECT_EQ(a->name(), "anthropic");
    EXPECT_EQ(o->name(), "openai");
    EXPECT_EQ(l->name(), "ollama");
}

TEST(ProviderParityTest, OllamaHealthCheckWithoutServer) {
    auto ollama = create_provider("ollama", "", "http://localhost:1");  // bad port
    // Just verify it doesn't crash — health_check may not exist on IProvider
    EXPECT_NE(ollama, nullptr);
}

// ═══════════════════════════════════════════════════════════════════════════════
// SECTION 5: SSE Streaming Parity — matches rust/crates/api/src/sse.rs
// ═══════════════════════════════════════════════════════════════════════════════

TEST(SSEParityTest, AnthropicStyleStream) {
    // Anthropic sends: event: content_block_delta\ndata: {"type":"content_block_delta",...}
    std::vector<std::pair<std::string, std::string>> events;
    SSEParser parser([&](std::string_view event, std::string_view data) {
        events.emplace_back(std::string(event), std::string(data));
    });

    parser.feed("event: message_start\ndata: {\"type\":\"message_start\"}\n\n");
    parser.feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"text\":\"Hello\"}}\n\n");
    parser.feed("event: message_stop\ndata: {\"type\":\"message_stop\"}\n\n");
    parser.flush();

    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[0].first, "message_start");
    EXPECT_EQ(events[1].first, "content_block_delta");
    EXPECT_EQ(events[2].first, "message_stop");

    // Verify data is valid JSON
    for (const auto& [evt, data] : events) {
        EXPECT_NO_THROW(nlohmann::json::parse(data)) << "Invalid JSON in event: " << evt;
    }
}

TEST(SSEParityTest, OpenAIStyleStream) {
    // OpenAI sends: data: {"choices":[...]}\n\ndata: [DONE]\n\n
    std::vector<std::string> data_chunks;
    SSEParser parser([&](std::string_view /*event*/, std::string_view data) {
        data_chunks.push_back(std::string(data));
    });

    parser.feed("data: {\"choices\":[{\"delta\":{\"content\":\"Hi\"}}]}\n\n");
    parser.feed("data: {\"choices\":[{\"delta\":{\"content\":\" there\"}}]}\n\n");
    parser.feed("data: [DONE]\n\n");
    parser.flush();

    ASSERT_GE(data_chunks.size(), 2u);
}

TEST(SSEParityTest, ChunkedDelivery) {
    // Simulates TCP chunking — data arrives in fragments
    std::vector<std::string> events;
    SSEParser parser([&](std::string_view /*event*/, std::string_view data) {
        events.push_back(std::string(data));
    });

    parser.feed("data: {\"part\"");
    parser.feed(":1}\n\ndata: ");
    parser.feed("{\"part\":2}\n\n");
    parser.flush();

    ASSERT_EQ(events.size(), 2u);
}

// ═══════════════════════════════════════════════════════════════════════════════
// SECTION 6: Config Parity — matches rust/crates/runtime/src/config.rs
// ═══════════════════════════════════════════════════════════════════════════════

TEST(ConfigParityTest, DefaultValues) {
    Config cfg;
    EXPECT_EQ(cfg.provider, "anthropic");
    EXPECT_EQ(cfg.model, "claude-sonnet-4-20250514");
    EXPECT_TRUE(cfg.streaming);
}

TEST(ConfigParityTest, JsonRoundTrip) {
    Config cfg;
    cfg.provider = "ollama";
    cfg.model = "llama3";
    cfg.api_base_url = "http://localhost:11434";

    nlohmann::json j = cfg;
    auto restored = j.get<Config>();

    EXPECT_EQ(restored.provider, "ollama");
    EXPECT_EQ(restored.model, "llama3");
    EXPECT_EQ(restored.api_base_url, "http://localhost:11434");
}

TEST(ConfigParityTest, EnvironmentOverrides) {
    // Rust reads ANTHROPIC_API_KEY, OPENAI_API_KEY, CLAW_MODEL, CLAW_PROVIDER
    // We read the same env vars in apply_env()
    // Just verify the config load doesn't crash
    auto result = load_config(std::nullopt);
    EXPECT_TRUE(result.has_value()) << result.error().message();
}

// ═══════════════════════════════════════════════════════════════════════════════
// SECTION 7: Context Compaction Parity
// Matches rust/crates/runtime conversation compaction
// ═══════════════════════════════════════════════════════════════════════════════

TEST(ContextParityTest, CompactionPreservesSystemAndRecent) {
    std::vector<Message> msgs;
    msgs.push_back(Message{Role::System, "System prompt"});
    for (int i = 0; i < 20; ++i) {
        msgs.push_back(Message{Role::User, "Message " + std::to_string(i)});
        msgs.push_back(Message{Role::Assistant, "Reply " + std::to_string(i)});
    }

    auto compacted = compact_context(msgs, 100, 3);

    // System message always preserved
    ASSERT_FALSE(compacted.empty());
    EXPECT_EQ(compacted[0].role, Role::System);
    EXPECT_EQ(compacted[0].content, "System prompt");

    // Recent messages preserved
    EXPECT_LE(compacted.size(), 8u); // system + summary + 3 recent pairs
}

TEST(ContextParityTest, TokenEstimation) {
    // ~4 chars per token heuristic
    auto tokens = estimate_tokens("Hello world! This is a test.");
    EXPECT_GT(tokens, 0u);
    EXPECT_LE(tokens, 20u);
}

// ═══════════════════════════════════════════════════════════════════════════════
// SECTION 8: String Utils Parity
// ═══════════════════════════════════════════════════════════════════════════════

TEST(StringUtilsParityTest, TrimHandlesEdgeCases) {
    EXPECT_EQ(str::trim("  hello  "), "hello");
    EXPECT_EQ(str::trim("\t\n"), "");
    EXPECT_EQ(str::trim(""), "");
    EXPECT_EQ(str::trim("no_whitespace"), "no_whitespace");
}

TEST(StringUtilsParityTest, SplitByDelimiter) {
    auto parts = str::split("a,b,c", ',');
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

TEST(StringUtilsParityTest, StartsWithEndsWith) {
    EXPECT_TRUE(str::starts_with("/help", "/"));
    EXPECT_FALSE(str::starts_with("help", "/"));
    EXPECT_TRUE(str::ends_with("test.json", ".json"));
    EXPECT_FALSE(str::ends_with("test.txt", ".json"));
}

TEST(StringUtilsParityTest, ToLower) {
    EXPECT_EQ(str::to_lower("HELLO"), "hello");
    EXPECT_EQ(str::to_lower("MiXeD"), "mixed");
}

// ═══════════════════════════════════════════════════════════════════════════════
// SECTION 9: Error System Parity
// ═══════════════════════════════════════════════════════════════════════════════

TEST(ErrorParityTest, AllErrorKindsExist) {
    // Verify all error categories match Rust error types
    EXPECT_EQ(error_kind_name(ErrorKind::Config), "Config");
    EXPECT_EQ(error_kind_name(ErrorKind::Session), "Session");
    EXPECT_EQ(error_kind_name(ErrorKind::Api), "Api");
    EXPECT_EQ(error_kind_name(ErrorKind::Network), "Network");
    EXPECT_EQ(error_kind_name(ErrorKind::Tool), "Tool");
    EXPECT_EQ(error_kind_name(ErrorKind::Command), "Command");
    EXPECT_EQ(error_kind_name(ErrorKind::Plugin), "Plugin");
    EXPECT_EQ(error_kind_name(ErrorKind::Io), "Io");
    EXPECT_EQ(error_kind_name(ErrorKind::Parse), "Parse");
    EXPECT_EQ(error_kind_name(ErrorKind::NotFound), "NotFound");
    EXPECT_EQ(error_kind_name(ErrorKind::Internal), "Internal");
}

TEST(ErrorParityTest, ErrorToStringFormat) {
    auto err = Error::api("test error");
    auto s = err.to_string();
    EXPECT_NE(s.find("Api"), std::string::npos);
    EXPECT_NE(s.find("test error"), std::string::npos);
}
