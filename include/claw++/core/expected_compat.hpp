#pragma once

// Compatibility shim: use std::expected if available (C++23), else tl::expected

#if __has_include(<expected>) && defined(__cpp_lib_expected)
#include <expected>
namespace claw::compat {
    using std::expected;
    using std::unexpected;
}
#else
#include <tl/expected.hpp>
namespace claw::compat {
    using tl::expected;
    using tl::unexpected;
}
#endif
