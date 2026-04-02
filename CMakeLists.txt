cmake_minimum_required(VERSION 3.28)

project(claw-plusplus
    VERSION 0.1.0
    DESCRIPTION "Claw++ — High-performance C++23 AI coding agent CLI"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Options ──────────────────────────────────────────────────────────────────
option(CLAW_BUILD_TESTS    "Build unit tests"          ON)
option(CLAW_BUILD_PLUGINS  "Build example plugins"     ON)
option(CLAW_USE_VCPKG      "Expect vcpkg-managed deps" OFF)

# ── Compiler warnings ───────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── Find dependencies ───────────────────────────────────────────────────────
find_package(CLI11       CONFIG QUIET)
find_package(nlohmann_json CONFIG QUIET)
find_package(spdlog      CONFIG QUIET)
find_package(fmt         CONFIG QUIET)
find_package(httplib     CONFIG QUIET)

# Replxx — try config mode first, fall back to pkg-config
find_package(replxx CONFIG QUIET)
if(NOT replxx_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(REPLXX QUIET replxx)
    endif()
endif()

# tl-expected (C++23 std::expected backport)
find_package(tl-expected CONFIG QUIET)

# Boost (headers + Asio)
find_package(Boost 1.83 QUIET COMPONENTS system)

# ── Collect sources ──────────────────────────────────────────────────────────
set(CLAW_SOURCES
    src/main.cpp
    src/core/config.cpp
    src/core/error.cpp
    src/utils/logging.cpp
    src/utils/terminal.cpp
    src/utils/string_utils.cpp
    src/session/session.cpp
    src/session/storage.cpp
    src/api/provider.cpp
    src/api/anthropic.cpp
    src/api/openai.cpp
    src/api/ollama.cpp
    src/api/streaming.cpp
    src/tools/registry.cpp
    src/tools/shell.cpp
    src/tools/filesystem.cpp
    src/tools/git.cpp
    src/tools/web_fetch.cpp
    src/tools/todo.cpp
    src/tools/notebook.cpp
    src/tools/search.cpp
    src/commands/registry.cpp
    src/commands/builtins.cpp
    src/plugins/loader.cpp
    src/runtime/agent.cpp
    src/runtime/context.cpp
)

# ── Main executable ──────────────────────────────────────────────────────────
add_executable(claw++ ${CLAW_SOURCES})

target_include_directories(claw++ PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries (only those found)
if(nlohmann_json_FOUND)
    target_link_libraries(claw++ PRIVATE nlohmann_json::nlohmann_json)
else()
    message(STATUS "nlohmann_json not found via find_package, using header-only fallback")
endif()

if(spdlog_FOUND)
    target_link_libraries(claw++ PRIVATE spdlog::spdlog)
endif()

if(fmt_FOUND)
    target_link_libraries(claw++ PRIVATE fmt::fmt)
endif()

if(CLI11_FOUND)
    target_link_libraries(claw++ PRIVATE CLI11::CLI11)
endif()

if(httplib_FOUND)
    target_link_libraries(claw++ PRIVATE httplib::httplib)
endif()

if(replxx_FOUND)
    target_link_libraries(claw++ PRIVATE replxx::replxx)
elseif(REPLXX_FOUND)
    target_include_directories(claw++ PRIVATE ${REPLXX_INCLUDE_DIRS})
    target_link_libraries(claw++ PRIVATE ${REPLXX_LIBRARIES})
endif()

if(Boost_FOUND)
    target_link_libraries(claw++ PRIVATE Boost::boost)
    if(TARGET Boost::system)
        target_link_libraries(claw++ PRIVATE Boost::system)
    endif()
endif()

if(tl-expected_FOUND)
    target_link_libraries(claw++ PRIVATE tl::expected)
endif()

# Platform-specific link libraries
if(UNIX)
    target_link_libraries(claw++ PRIVATE dl pthread)
endif()
if(WIN32)
    target_link_libraries(claw++ PRIVATE ws2_32)
endif()

# ── Tests ────────────────────────────────────────────────────────────────────
if(CLAW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Install ──────────────────────────────────────────────────────────────────
install(TARGETS claw++ RUNTIME DESTINATION bin)
