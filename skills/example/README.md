# Example Claw++ Plugin/Skill

This directory contains example plugins (skills) for Claw++.

## Creating a Plugin

A Claw++ plugin is a shared library (`.so` / `.dylib` / `.dll`) that exports
two C-linkage functions:

```cpp
#include "claw++/plugins/plugin.hpp"

class MyPlugin : public claw::IPlugin {
public:
    claw::PluginInfo info() const override {
        return {"my-plugin", "1.0.0", "My example plugin", "Author"};
    }

    claw::Result<void> on_load() override {
        // Initialize your plugin
        return {};
    }

    void on_before_turn(const std::vector<claw::Message>& messages) override {
        // Called before each agent turn
    }

    void on_after_turn(const claw::Message& response) override {
        // Called after each agent turn
    }
};

extern "C" claw::IPlugin* claw_plugin_create() {
    return new MyPlugin();
}

extern "C" void claw_plugin_destroy(claw::IPlugin* p) {
    delete p;
}
```

## Building

```bash
# Linux
g++ -shared -fPIC -std=c++23 -o my_plugin.so my_plugin.cpp -I/path/to/claw++/include

# macOS
clang++ -shared -fPIC -std=c++23 -o my_plugin.dylib my_plugin.cpp -I/path/to/claw++/include

# Windows
cl /LD /std:c++latest my_plugin.cpp /I /path/to/claw++/include
```

## Loading

Place the plugin in one of the configured plugin directories, or specify
the path in your config:

```json
{
  "plugin_dirs": ["./skills"]
}
```

Plugins are loaded automatically on startup and support hot-reload.
