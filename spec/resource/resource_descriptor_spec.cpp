#include <catch2/catch_all.hpp>
#include <cask/resource/resource_descriptor.hpp>
#include <cask/resource/mesh_data.hpp>
#include <cask/resource/texture_data.hpp>
#include <cstring>

SCENARIO("MeshData descriptor provides correct component names", "[resource_descriptor]") {
    GIVEN("the ResourceDescriptor for MeshData") {
        THEN("store equals MeshStore") {
            REQUIRE(std::strcmp(ResourceDescriptor<MeshData>::store, "MeshStore") == 0);
        }

        THEN("components equals MeshComponents") {
            REQUIRE(std::strcmp(ResourceDescriptor<MeshData>::components, "MeshComponents") == 0);
        }

        THEN("sources equals MeshSources") {
            REQUIRE(std::strcmp(ResourceDescriptor<MeshData>::sources, "MeshSources") == 0);
        }

        THEN("loader_registry equals MeshLoaderRegistry") {
            REQUIRE(std::strcmp(ResourceDescriptor<MeshData>::loader_registry, "MeshLoaderRegistry") == 0);
        }
    }
}

SCENARIO("TextureData descriptor provides correct component names", "[resource_descriptor]") {
    GIVEN("the ResourceDescriptor for TextureData") {
        THEN("store equals TextureStore") {
            REQUIRE(std::strcmp(ResourceDescriptor<TextureData>::store, "TextureStore") == 0);
        }

        THEN("components equals TextureComponents") {
            REQUIRE(std::strcmp(ResourceDescriptor<TextureData>::components, "TextureComponents") == 0);
        }

        THEN("sources equals TextureSources") {
            REQUIRE(std::strcmp(ResourceDescriptor<TextureData>::sources, "TextureSources") == 0);
        }

        THEN("loader_registry equals TextureLoaderRegistry") {
            REQUIRE(std::strcmp(ResourceDescriptor<TextureData>::loader_registry, "TextureLoaderRegistry") == 0);
        }
    }
}
