#pragma once

#include <cask/resource/mesh_data.hpp>
#include <cask/resource/texture_data.hpp>

template<typename ResourceType>
struct ResourceDescriptor;

#define CASK_RESOURCE_DESCRIPTOR(Type, Prefix) \
template<> \
struct ResourceDescriptor<Type> { \
    static constexpr const char* store = Prefix "Store"; \
    static constexpr const char* components = Prefix "Components"; \
    static constexpr const char* sources = Prefix "Sources"; \
    static constexpr const char* loader_registry = Prefix "LoaderRegistry"; \
};

CASK_RESOURCE_DESCRIPTOR(MeshData, "Mesh")
CASK_RESOURCE_DESCRIPTOR(TextureData, "Texture")
