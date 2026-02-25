#include <catch2/catch_all.hpp>
#include <cask/resource/resource_loader_registry.hpp>
#include <cask/resource/resource_sources.hpp>
#include <cask/resource/resource_store.hpp>
#include <cask/schema/describe_resource_sources.hpp>

namespace {

struct FakeResource {
    int data;
};

}

SCENARIO("resource sources serialization produces key-to-spec json", "[resource_sources_serialization]") {
    GIVEN("a resource sources with json spec entries") {
        ResourceSources<FakeResource> sources;
        sources.entries["wall_mesh"] = {{"loader", "obj"}, {"path", "meshes/wall.obj"}};
        sources.entries["unit_cube"] = {{"loader", "inline"}, {"data", 42}};

        ResourceStore<FakeResource> store;
        cask::ResourceLoaderRegistry<FakeResource> loader_registry;
        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader_registry);

        WHEN("the sources are serialized") {
            auto data = entry.serialize(&sources);

            THEN("each key maps to its full loader spec json") {
                REQUIRE(data.contains("wall_mesh"));
                REQUIRE(data["wall_mesh"]["loader"] == "obj");
                REQUIRE(data["wall_mesh"]["path"] == "meshes/wall.obj");
                REQUIRE(data.contains("unit_cube"));
                REQUIRE(data["unit_cube"]["loader"] == "inline");
                REQUIRE(data["unit_cube"]["data"] == 42);
            }
        }
    }
}

SCENARIO("resource sources deserialization invokes named loaders and stores results", "[resource_sources_serialization]") {
    GIVEN("json data with loader specs and a tracking registry") {
        nlohmann::json data = {
            {"wall_mesh", {{"loader", "obj"}, {"path", "meshes/wall.obj"}}},
            {"unit_cube", {{"loader", "inline"}, {"data", 42}}}
        };

        ResourceStore<FakeResource> store;
        std::vector<std::pair<std::string, nlohmann::json>> invocations;

        cask::ResourceLoaderRegistry<FakeResource> loader_registry;
        loader_registry.add("obj", [&invocations](const nlohmann::json& entry_json) {
            invocations.emplace_back("obj", entry_json);
            return FakeResource{static_cast<int>(entry_json["path"].get<std::string>().size())};
        });
        loader_registry.add("inline", [&invocations](const nlohmann::json& entry_json) {
            invocations.emplace_back("inline", entry_json);
            return FakeResource{entry_json["data"].get<int>()};
        });

        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader_registry);

        WHEN("the json is deserialized") {
            ResourceSources<FakeResource> sources;
            entry.deserialize(data, &sources, nlohmann::json{});

            THEN("each named loader was invoked with its full entry json") {
                REQUIRE(invocations.size() == 2);

                bool found_obj = false;
                bool found_inline = false;
                for (const auto& [loader_name, entry_json] : invocations) {
                    if (loader_name == "obj") {
                        found_obj = true;
                        REQUIRE(entry_json["path"] == "meshes/wall.obj");
                    }
                    if (loader_name == "inline") {
                        found_inline = true;
                        REQUIRE(entry_json["data"] == 42);
                    }
                }
                REQUIRE(found_obj);
                REQUIRE(found_inline);
            }

            THEN("the resource store contains the loaded resources") {
                auto wall_handle = store.key_to_handle_.at("wall_mesh");
                auto cube_handle = store.key_to_handle_.at("unit_cube");
                REQUIRE(store.resources_[wall_handle].data == static_cast<int>(std::string("meshes/wall.obj").size()));
                REQUIRE(store.resources_[cube_handle].data == 42);
            }

            THEN("the sources entries are populated with json specs") {
                REQUIRE(sources.entries.size() == 2);
                REQUIRE(sources.entries["wall_mesh"]["loader"] == "obj");
                REQUIRE(sources.entries["wall_mesh"]["path"] == "meshes/wall.obj");
                REQUIRE(sources.entries["unit_cube"]["loader"] == "inline");
                REQUIRE(sources.entries["unit_cube"]["data"] == 42);
            }
        }
    }
}

SCENARIO("resource sources deserialization produces resource remap context", "[resource_sources_serialization]") {
    GIVEN("json data with a single entry") {
        nlohmann::json data = {
            {"wall_mesh", {{"loader", "obj"}, {"path", "meshes/wall.obj"}}}
        };

        ResourceStore<FakeResource> store;
        cask::ResourceLoaderRegistry<FakeResource> loader_registry;
        loader_registry.add("obj", [](const nlohmann::json& entry_json) {
            return FakeResource{static_cast<int>(entry_json["path"].get<std::string>().size())};
        });

        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader_registry);

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

SCENARIO("resource sources deserialization throws for unknown loader", "[resource_sources_serialization]") {
    GIVEN("json data referencing a loader not in the registry") {
        nlohmann::json data = {
            {"wall_mesh", {{"loader", "unknown_format"}, {"path", "meshes/wall.xyz"}}}
        };

        ResourceStore<FakeResource> store;
        cask::ResourceLoaderRegistry<FakeResource> loader_registry;
        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader_registry);

        WHEN("the json is deserialized") {
            ResourceSources<FakeResource> sources;

            THEN("it throws with a message containing the loader name") {
                REQUIRE_THROWS_WITH(
                    entry.deserialize(data, &sources, nlohmann::json{}),
                    Catch::Matchers::ContainsSubstring("unknown_format")
                );
            }
        }
    }
}

SCENARIO("resource sources entry has correct schema metadata", "[resource_sources_serialization]") {
    GIVEN("a described resource sources entry") {
        ResourceStore<FakeResource> store;
        cask::ResourceLoaderRegistry<FakeResource> loader_registry;
        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader_registry);

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
        cask::ResourceLoaderRegistry<FakeResource> loader_registry;
        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader_registry);

        THEN("the dependencies are empty") {
            REQUIRE(entry.dependencies.empty());
        }
    }
}

SCENARIO("empty resource sources serializes to empty json", "[resource_sources_serialization]") {
    GIVEN("an empty resource sources") {
        ResourceSources<FakeResource> sources;

        ResourceStore<FakeResource> store;
        cask::ResourceLoaderRegistry<FakeResource> loader_registry;
        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader_registry);

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
    GIVEN("resource sources with json spec entries") {
        ResourceSources<FakeResource> original;
        original.entries["wall_mesh"] = {{"loader", "obj"}, {"path", "meshes/wall.obj"}};
        original.entries["unit_cube"] = {{"loader", "inline"}, {"data", 42}};

        ResourceStore<FakeResource> store;
        cask::ResourceLoaderRegistry<FakeResource> loader_registry;
        loader_registry.add("obj", [](const nlohmann::json& entry_json) {
            return FakeResource{static_cast<int>(entry_json["path"].get<std::string>().size())};
        });
        loader_registry.add("inline", [](const nlohmann::json& entry_json) {
            return FakeResource{entry_json["data"].get<int>()};
        });

        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader_registry);

        WHEN("the sources are serialized then deserialized") {
            auto data = entry.serialize(&original);

            ResourceSources<FakeResource> restored;
            entry.deserialize(data, &restored, nlohmann::json{});

            THEN("the restored sources have the same json spec entries") {
                REQUIRE(restored.entries.size() == original.entries.size());
                REQUIRE(restored.entries["wall_mesh"]["loader"] == "obj");
                REQUIRE(restored.entries["wall_mesh"]["path"] == "meshes/wall.obj");
                REQUIRE(restored.entries["unit_cube"]["loader"] == "inline");
                REQUIRE(restored.entries["unit_cube"]["data"] == 42);
            }
        }
    }
}

SCENARIO("resource sources deserialization supports multiple loader types", "[resource_sources_serialization]") {
    GIVEN("json data with entries using different loaders") {
        nlohmann::json data = {
            {"wall_mesh", {{"loader", "obj"}, {"path", "meshes/wall.obj"}}},
            {"default_value", {{"loader", "inline"}, {"data", 99}}}
        };

        ResourceStore<FakeResource> store;
        std::vector<std::string> invoked_loaders;

        cask::ResourceLoaderRegistry<FakeResource> loader_registry;
        loader_registry.add("obj", [&invoked_loaders](const nlohmann::json& entry_json) {
            invoked_loaders.push_back("obj");
            return FakeResource{static_cast<int>(entry_json["path"].get<std::string>().size())};
        });
        loader_registry.add("inline", [&invoked_loaders](const nlohmann::json& entry_json) {
            invoked_loaders.push_back("inline");
            return FakeResource{entry_json["data"].get<int>()};
        });

        auto entry = cask::describe_resource_sources<FakeResource>("MeshSources", store, loader_registry);

        WHEN("the json is deserialized") {
            ResourceSources<FakeResource> sources;
            entry.deserialize(data, &sources, nlohmann::json{});

            THEN("both loader types were invoked") {
                REQUIRE(invoked_loaders.size() == 2);
                REQUIRE(std::find(invoked_loaders.begin(), invoked_loaders.end(), "obj") != invoked_loaders.end());
                REQUIRE(std::find(invoked_loaders.begin(), invoked_loaders.end(), "inline") != invoked_loaders.end());
            }

            THEN("both resources are stored correctly") {
                auto wall_handle = store.key_to_handle_.at("wall_mesh");
                auto default_handle = store.key_to_handle_.at("default_value");
                REQUIRE(store.resources_[wall_handle].data == static_cast<int>(std::string("meshes/wall.obj").size()));
                REQUIRE(store.resources_[default_handle].data == 99);
            }
        }
    }
}
