find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND)
    message(STATUS "GTest not found, skipping tests")
    return()
endif()

# ── Test sources ─────────────────────────────────────────────────────────────
set(TEST_SOURCES
    test_main.cpp
    test_config.cpp
    test_session.cpp
    test_tools.cpp
    test_commands.cpp
    test_context.cpp
    test_string_utils.cpp
    test_sse_parser.cpp
    test_integration.cpp
)

# We need the main project sources (except main.cpp) for the test binary
set(LIB_SOURCES
    ${CMAKE_SOURCE_DIR}/src/core/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/error.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/logging.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/terminal.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/string_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/session/session.cpp
    ${CMAKE_SOURCE_DIR}/src/session/storage.cpp
    ${CMAKE_SOURCE_DIR}/src/api/provider.cpp
    ${CMAKE_SOURCE_DIR}/src/api/anthropic.cpp
    ${CMAKE_SOURCE_DIR}/src/api/openai.cpp
    ${CMAKE_SOURCE_DIR}/src/api/ollama.cpp
    ${CMAKE_SOURCE_DIR}/src/api/streaming.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/registry.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/shell.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/filesystem.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/git.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/web_fetch.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/todo.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/notebook.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/search.cpp
    ${CMAKE_SOURCE_DIR}/src/commands/registry.cpp
    ${CMAKE_SOURCE_DIR}/src/commands/builtins.cpp
    ${CMAKE_SOURCE_DIR}/src/plugins/loader.cpp
    ${CMAKE_SOURCE_DIR}/src/runtime/agent.cpp
    ${CMAKE_SOURCE_DIR}/src/runtime/context.cpp
)

add_executable(claw_tests ${TEST_SOURCES} ${LIB_SOURCES})

target_include_directories(claw_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(claw_tests PRIVATE GTest::gtest GTest::gtest_main)

# Link same deps as main target
if(nlohmann_json_FOUND)
    target_link_libraries(claw_tests PRIVATE nlohmann_json::nlohmann_json)
endif()
if(spdlog_FOUND)
    target_link_libraries(claw_tests PRIVATE spdlog::spdlog)
endif()
if(fmt_FOUND)
    target_link_libraries(claw_tests PRIVATE fmt::fmt)
endif()
if(CLI11_FOUND)
    target_link_libraries(claw_tests PRIVATE CLI11::CLI11)
endif()
if(httplib_FOUND)
    target_link_libraries(claw_tests PRIVATE httplib::httplib)
endif()
if(replxx_FOUND)
    target_link_libraries(claw_tests PRIVATE replxx::replxx)
elseif(REPLXX_FOUND)
    target_include_directories(claw_tests PRIVATE ${REPLXX_INCLUDE_DIRS})
    target_link_libraries(claw_tests PRIVATE ${REPLXX_LIBRARIES})
endif()
if(Boost_FOUND)
    target_link_libraries(claw_tests PRIVATE Boost::boost)
    if(TARGET Boost::system)
        target_link_libraries(claw_tests PRIVATE Boost::system)
    endif()
endif()

if(tl-expected_FOUND)
    target_link_libraries(claw_tests PRIVATE tl::expected)
endif()

if(UNIX)
    target_link_libraries(claw_tests PRIVATE dl pthread)
endif()
if(WIN32)
    target_link_libraries(claw_tests PRIVATE ws2_32)
endif()

include(GoogleTest)
gtest_discover_tests(claw_tests)
