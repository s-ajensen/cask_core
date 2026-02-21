#pragma once

#include <cask/schema/loader.hpp>
#include <cask/schema/saver.hpp>
#include <functional>
#include <string>

namespace cask {

using PluginLoader = std::function<void(const std::string&)>;

inline nlohmann::json save_bundle(
    const std::vector<std::string>& plugin_names,
    const std::vector<std::string>& component_names,
    const SerializationRegistry& registry,
    ComponentResolver component_resolver
) {
    auto bundle = save(component_names, registry, component_resolver);
    bundle["plugins"] = plugin_names;
    return bundle;
}

inline nlohmann::json load_bundle(
    const nlohmann::json& bundle_data,
    const SerializationRegistry& registry,
    PluginLoader plugin_loader,
    ComponentResolver component_resolver
) {
    for (const auto& plugin_name : bundle_data.value("plugins", nlohmann::json::array())) {
        plugin_loader(plugin_name.get<std::string>());
    }
    return load(bundle_data, registry, component_resolver);
}

}
