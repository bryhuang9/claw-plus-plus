# Contributing to Claw++

## Clean-Room Policy

All contributions must be original work or derived solely from publicly
available documentation and APIs. Do not reference, copy, or port any
proprietary source code.

## Code Style

- C++23 standard
- `snake_case` for functions and variables
- `PascalCase` for types and classes
- `UPPER_SNAKE_CASE` for constants and macros
- Use `std::expected` for error handling where possible
- Prefer value semantics; use `std::unique_ptr` for polymorphism
- Header files: `.hpp`, source files: `.cpp`
- All headers in `include/claw++/`

## Building

```bash
cmake --preset vcpkg-debug
cmake --build build/vcpkg-debug --parallel
ctest --test-dir build/vcpkg-debug --output-on-failure
```

## Pull Request Checklist

- [ ] Code compiles with no warnings (`-Wall -Wextra -Wpedantic`)
- [ ] All existing tests pass
- [ ] New features include tests
- [ ] PARITY.md updated if feature parity changed
- [ ] No proprietary code or patterns
