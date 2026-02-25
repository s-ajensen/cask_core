#pragma once

#include <cask/ecs/component_store.hpp>
#include <cask/resource/resource_handle.hpp>
#include <cask/resource/resource_store.hpp>
#include <cask/schema/serialization_registry.hpp>
#include <string>

namespace cask {

template<typename T>
SerializeFn build_resource_component_serialize(const ResourceStore<T>& resource_store) {
    return [&resource_store](const void* instance) -> nlohmann::json {
        const auto* store = static_cast<const ComponentStore<ResourceHandle<T>>*>(instance);
        nlohmann::json result = nlohmann::json::object();

        store->each([&result, &resource_store](uint32_t entity, const ResourceHandle<T>& handle) {
            result[std::to_string(entity)] = resource_store.key(handle);
        });

        return result;
    };
}

template<typename T>
DeserializeFn build_resource_component_deserialize(const char* sources_name) {
    std::string remap_key = resource_remap_key(sources_name);
    return [remap_key](const nlohmann::json& json, void* instance, const nlohmann::json& context) -> nlohmann::json {
        auto* store = static_cast<ComponentStore<ResourceHandle<T>>*>(instance);
        const auto& entity_remap = context.at("entity_remap");
        const auto& resource_remap = context.at(remap_key);

        for (const auto& [entity_key, resource_key] : json.items()) {
            uint32_t entity = entity_remap.at(entity_key).template get<uint32_t>();
            uint32_t handle_value = resource_remap.at(resource_key).template get<uint32_t>();
            store->insert(entity, ResourceHandle<T>{handle_value});
        }

        return nlohmann::json::object();
    };
}

template<typename T>
RegistryEntry describe_resource_components(const char* name, const char* sources_name, ResourceStore<T>& store) {
    nlohmann::json schema = {
        {"name", name},
        {"container", "component_store"},
        {"value_type", "resource_handle"}
    };

    return RegistryEntry{
        std::move(schema),
        build_resource_component_serialize<T>(store),
        build_resource_component_deserialize<T>(sources_name),
        {"EntityRegistry", std::string(sources_name)}
    };
}

}
