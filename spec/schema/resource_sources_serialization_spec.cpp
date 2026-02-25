#include <catch2/catch_all.hpp>
#include <cask/resource/resource_sources.hpp>
#include <cask/resource/resource_store.hpp>
#include <cask/schema/describe_resource_sources.hpp>

namespace {

struct FakeResource {
    int data;
};

}

SCENARIO("resource sources serialization produces key-to-path json", "[resource_sources_serialization]") {
    GIVEN("a resource sources with known key-to-path entries") {
        ResourceSources<FakeResource> sources;
        sources.entries["wall_mesh"] = "meshes/wall.obj";
        sources.entries["floor_mesh"] = "meshes/floor.obj";

        ResourceStore<FakeResource> store;
        auto loader = [](const std::string&) -> FakeResource { return FakeResource{0}; };
        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader);

        WHEN("the sources are serialized") {
            auto data = entry.serialize(&sources);

            THEN("the json contains keys mapped to their paths") {
                REQUIRE(data.contains("wall_mesh"));
                REQUIRE(data["wall_mesh"] == "meshes/wall.obj");
                REQUIRE(data.contains("floor_mesh"));
                REQUIRE(data["floor_mesh"] == "meshes/floor.obj");
            }
        }
    }
}

SCENARIO("resource sources deserialization calls loader and stores results", "[resource_sources_serialization]") {
    GIVEN("json data with key-to-path entries and a tracking loader") {
        nlohmann::json data = {
            {"wall_mesh", "meshes/wall.obj"},
            {"floor_mesh", "meshes/floor.obj"}
        };

        ResourceStore<FakeResource> store;
        std::vector<std::string> loaded_paths;
        auto loader = [&loaded_paths](const std::string& path) -> FakeResource {
            loaded_paths.push_back(path);
            return FakeResource{static_cast<int>(path.size())};
        };

        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader);

        WHEN("the json is deserialized") {
            ResourceSources<FakeResource> sources;
            entry.deserialize(data, &sources, nlohmann::json{});

            THEN("the loader was called for each path") {
                REQUIRE(loaded_paths.size() == 2);
                REQUIRE(std::find(loaded_paths.begin(), loaded_paths.end(), "meshes/wall.obj") != loaded_paths.end());
                REQUIRE(std::find(loaded_paths.begin(), loaded_paths.end(), "meshes/floor.obj") != loaded_paths.end());
            }

            THEN("the resource store contains the loaded resources") {
                auto wall_handle = store.key_to_handle_.at("wall_mesh");
                auto floor_handle = store.key_to_handle_.at("floor_mesh");
                REQUIRE(store.resources_[wall_handle].data == static_cast<int>(std::string("meshes/wall.obj").size()));
                REQUIRE(store.resources_[floor_handle].data == static_cast<int>(std::string("meshes/floor.obj").size()));
            }

            THEN("the sources entries are populated") {
                REQUIRE(sources.entries.size() == 2);
                REQUIRE(sources.entries["wall_mesh"] == "meshes/wall.obj");
                REQUIRE(sources.entries["floor_mesh"] == "meshes/floor.obj");
            }
        }
    }
}

SCENARIO("resource sources deserialization produces resource remap context", "[resource_sources_serialization]") {
    GIVEN("json data with a single entry") {
        nlohmann::json data = {
            {"wall_mesh", "meshes/wall.obj"}
        };

        ResourceStore<FakeResource> store;
        auto loader = [](const std::string&) -> FakeResource { return FakeResource{42}; };
        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader);

        WHEN("the json is deserialized") {
            ResourceSources<FakeResource> sources;
            auto context = entry.deserialize(data, &sources, nlohmann::json{});

            THEN("the returned context contains resource_remap_MeshSources") {
                REQUIRE(context.contains("resource_remap_MeshSources"));
            }

            THEN("the remap maps keys to handle values") {
                auto remap = context["resource_remap_MeshSources"];
                auto handle_value = store.key_to_handle_.at("wall_mesh");
                REQUIRE(remap["wall_mesh"] == handle_value);
            }
        }
    }
}

SCENARIO("resource sources entry has correct schema metadata", "[resource_sources_serialization]") {
    GIVEN("a described resource sources entry") {
        ResourceStore<FakeResource> store;
        auto loader = [](const std::string&) -> FakeResource { return FakeResource{0}; };
        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader);

        THEN("the schema type is resource_sources") {
            REQUIRE(entry.schema["type"] == "resource_sources");
        }

        THEN("the schema name matches the registration name") {
            REQUIRE(entry.schema["name"] == "MeshSources");
        }
    }
}

SCENARIO("resource sources entry has no dependencies", "[resource_sources_serialization]") {
    GIVEN("a described resource sources entry") {
        ResourceStore<FakeResource> store;
        auto loader = [](const std::string&) -> FakeResource { return FakeResource{0}; };
        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader);

        THEN("the dependencies are empty") {
            REQUIRE(entry.dependencies.empty());
        }
    }
}

SCENARIO("empty resource sources serializes to empty json", "[resource_sources_serialization]") {
    GIVEN("an empty resource sources") {
        ResourceSources<FakeResource> sources;

        ResourceStore<FakeResource> store;
        auto loader = [](const std::string&) -> FakeResource { return FakeResource{0}; };
        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader);

        WHEN("the empty sources are serialized") {
            auto data = entry.serialize(&sources);

            THEN("the result is an empty json object") {
                REQUIRE(data.is_object());
                REQUIRE(data.empty());
            }
        }
    }
}

SCENARIO("resource sources round-trips through serialization", "[resource_sources_serialization]") {
    GIVEN("resource sources with entries") {
        ResourceSources<FakeResource> original;
        original.entries["wall_mesh"] = "meshes/wall.obj";
        original.entries["floor_mesh"] = "meshes/floor.obj";

        ResourceStore<FakeResource> store;
        auto loader = [](const std::string&) -> FakeResource { return FakeResource{0}; };
        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader);

        WHEN("the sources are serialized then deserialized") {
            auto data = entry.serialize(&original);

            ResourceSources<FakeResource> restored;
            entry.deserialize(data, &restored, nlohmann::json{});

            THEN("the restored sources have the same keys and paths") {
                REQUIRE(restored.entries.size() == original.entries.size());
                REQUIRE(restored.entries["wall_mesh"] == "meshes/wall.obj");
                REQUIRE(restored.entries["floor_mesh"] == "meshes/floor.obj");
            }
        }
    }
}
