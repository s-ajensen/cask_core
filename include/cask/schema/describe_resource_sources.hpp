#pragma once

#include <cask/resource/resource_sources.hpp>
#include <cask/resource/resource_store.hpp>
#include <cask/schema/serialization_registry.hpp>
#include <functional>
#include <string>

namespace cask {

template<typename T>
SerializeFn build_resource_sources_serialize() {
    return [](const void* instance) -> nlohmann::json {
        const auto* sources = static_cast<const ResourceSources<T>*>(instance);
        nlohmann::json result = nlohmann::json::object();

        for (const auto& [key, path] : sources->entries) {
            result[key] = path;
        }

        return result;
    };
}

template<typename T>
DeserializeFn build_resource_sources_deserialize(const std::string& registration_name, ResourceStore<T>& store, std::function<T(const std::string&)> loader) {
    return [registration_name, &store, loader](const nlohmann::json& data, void* instance, const nlohmann::json&) -> nlohmann::json {
        auto* sources = static_cast<ResourceSources<T>*>(instance);
        nlohmann::json remap = nlohmann::json::object();

        for (const auto& [key, path_json] : data.items()) {
            std::string path = path_json.get<std::string>();
            T loaded = loader(path);
            auto handle = store.store(key, std::move(loaded));
            sources->entries[key] = path;
            remap[key] = handle.value;
        }

        return {{resource_remap_key(registration_name), remap}};
    };
}

template<typename T>
RegistryEntry describe_resource_sources(const char* name, ResourceStore<T>& store, std::function<T(const std::string&)> loader) {
    nlohmann::json schema = {
        {"name", name},
        {"type", "resource_sources"}
    };

    return RegistryEntry{
        std::move(schema),
        build_resource_sources_serialize<T>(),
        build_resource_sources_deserialize<T>(std::string(name), store, std::move(loader)),
        {}
    };
}

}
