#include "claw++/plugins/loader.hpp"
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace claw {

PluginLoader::~PluginLoader() {
    unload_all();
}

Result<void> PluginLoader::load(const std::filesystem::path& path) {
    spdlog::info("Loading plugin: {}", path.string());

    void* handle = nullptr;
#ifdef _WIN32
    handle = reinterpret_cast<void*>(LoadLibraryA(path.string().c_str()));
    if (!handle) {
        return compat::unexpected(Error::plugin("Failed to load DLL: " + path.string()));
    }
    auto create_fn  = reinterpret_cast<PluginCreateFn>(GetProcAddress(
        reinterpret_cast<HMODULE>(handle), "claw_plugin_create"));
    auto destroy_fn = reinterpret_cast<PluginDestroyFn>(GetProcAddress(
        reinterpret_cast<HMODULE>(handle), "claw_plugin_destroy"));
#else
    handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        return compat::unexpected(Error::plugin(
            std::string("Failed to load plugin: ") + dlerror()
        ));
    }
    auto create_fn  = reinterpret_cast<PluginCreateFn>(dlsym(handle, "claw_plugin_create"));
    auto destroy_fn = reinterpret_cast<PluginDestroyFn>(dlsym(handle, "claw_plugin_destroy"));
#endif

    if (!create_fn || !destroy_fn) {
#ifdef _WIN32
        FreeLibrary(reinterpret_cast<HMODULE>(handle));
#else
        dlclose(handle);
#endif
        return compat::unexpected(Error::plugin(
            "Plugin missing claw_plugin_create/claw_plugin_destroy: " + path.string()
        ));
    }

    IPlugin* raw_instance = create_fn();
    if (!raw_instance) {
#ifdef _WIN32
        FreeLibrary(reinterpret_cast<HMODULE>(handle));
#else
        dlclose(handle);
#endif
        return compat::unexpected(Error::plugin("Plugin factory returned null: " + path.string()));
    }

    auto info = raw_instance->info();
    spdlog::info("Loaded plugin: {} v{}", info.name, info.version);

    auto init_result = raw_instance->on_load();
    if (!init_result) {
        destroy_fn(raw_instance);
#ifdef _WIN32
        FreeLibrary(reinterpret_cast<HMODULE>(handle));
#else
        dlclose(handle);
#endif
        return compat::unexpected(init_result.error());
    }

    LoadedPlugin lp;
    lp.path       = path.string();
    lp.handle     = handle;
    lp.create_fn  = create_fn;
    lp.destroy_fn = destroy_fn;
    lp.instance   = std::unique_ptr<IPlugin, void(*)(IPlugin*)>(raw_instance, destroy_fn);

    plugins_[info.name] = std::move(lp);
    return {};
}

Result<size_t> PluginLoader::load_directory(const std::filesystem::path& dir) {
    if (!std::filesystem::exists(dir)) {
        return 0ULL;
    }

    size_t count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        auto ext = entry.path().extension().string();
#ifdef _WIN32
        if (ext != ".dll") continue;
#elif __APPLE__
        if (ext != ".dylib") continue;
#else
        if (ext != ".so") continue;
#endif
        auto result = load(entry.path());
        if (result) {
            ++count;
        } else {
            spdlog::warn("Failed to load plugin {}: {}",
                         entry.path().string(), result.error().message());
        }
    }
    return count;
}

Result<void> PluginLoader::unload(const std::string& name) {
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        return compat::unexpected(Error::not_found("Plugin not found: " + name));
    }

    auto& lp = it->second;
    if (lp.instance) {
        lp.instance->on_unload();
    }
    lp.instance.reset();

#ifdef _WIN32
    if (lp.handle) FreeLibrary(reinterpret_cast<HMODULE>(lp.handle));
#else
    if (lp.handle) dlclose(lp.handle);
#endif

    plugins_.erase(it);
    spdlog::info("Unloaded plugin: {}", name);
    return {};
}

void PluginLoader::unload_all() {
    for (auto& [name, lp] : plugins_) {
        if (lp.instance) lp.instance->on_unload();
        lp.instance.reset();
#ifdef _WIN32
        if (lp.handle) FreeLibrary(reinterpret_cast<HMODULE>(lp.handle));
#else
        if (lp.handle) dlclose(lp.handle);
#endif
    }
    plugins_.clear();
}

IPlugin* PluginLoader::get(const std::string& name) const {
    auto it = plugins_.find(name);
    return (it != plugins_.end()) ? it->second.instance.get() : nullptr;
}

std::vector<PluginInfo> PluginLoader::list() const {
    std::vector<PluginInfo> result;
    result.reserve(plugins_.size());
    for (const auto& [name, lp] : plugins_) {
        if (lp.instance) result.push_back(lp.instance->info());
    }
    return result;
}

void PluginLoader::notify_before_turn(const std::vector<Message>& messages) {
    for (auto& [name, lp] : plugins_) {
        if (lp.instance) lp.instance->on_before_turn(messages);
    }
}

void PluginLoader::notify_after_turn(const Message& response) {
    for (auto& [name, lp] : plugins_) {
        if (lp.instance) lp.instance->on_after_turn(response);
    }
}

void PluginLoader::notify_before_tool(const ToolCall& call) {
    for (auto& [name, lp] : plugins_) {
        if (lp.instance) lp.instance->on_before_tool(call);
    }
}

void PluginLoader::notify_after_tool(const ToolResult& result) {
    for (auto& [name, lp] : plugins_) {
        if (lp.instance) lp.instance->on_after_tool(result);
    }
}

} // namespace claw
