#include <catch2/catch_all.hpp>
#include <cask/ecs/component_store.hpp>
#include <cask/resource/resource_handle.hpp>
#include <cask/resource/resource_store.hpp>
#include <cask/schema/describe_resource_components.hpp>

namespace {

struct FakeResource {
    int data;
};

nlohmann::json build_context(
    std::initializer_list<std::pair<std::string, uint32_t>> entity_pairs,
    const std::string& sources_name,
    std::initializer_list<std::pair<std::string, uint32_t>> resource_pairs) {
    nlohmann::json entity_remap = nlohmann::json::object();
    for (const auto& [key, value] : entity_pairs) {
        entity_remap[key] = value;
    }

    nlohmann::json resource_remap = nlohmann::json::object();
    for (const auto& [key, value] : resource_pairs) {
        resource_remap[key] = value;
    }

    return {
        {"entity_remap", entity_remap},
        {"resource_remap_" + sources_name, resource_remap}
    };
}

}

SCENARIO("resource components serialization converts handles to key strings", "[resource_components_serialization]") {
    GIVEN("a component store with resource handles and a resource store with keyed resources") {
        ResourceStore<FakeResource> resource_store;
        auto wall_handle = resource_store.store("wall_mesh", FakeResource{10});
        auto floor_handle = resource_store.store("floor_mesh", FakeResource{20});

        ComponentStore<ResourceHandle<FakeResource>> comp_store;
        comp_store.insert(42, wall_handle);
        comp_store.insert(17, floor_handle);

        auto entry = cask::describe_resource_components<FakeResource>("MeshComponents", "MeshSources", resource_store);

        WHEN("the store is serialized") {
            auto data = entry.serialize(&comp_store);

            THEN("entity 42 maps to the wall_mesh key string") {
                REQUIRE(data.contains("42"));
                REQUIRE(data["42"] == "wall_mesh");
            }

            THEN("entity 17 maps to the floor_mesh key string") {
                REQUIRE(data.contains("17"));
                REQUIRE(data["17"] == "floor_mesh");
            }
        }
    }
}

SCENARIO("resource components deserialization resolves entity and resource remaps", "[resource_components_serialization]") {
    GIVEN("json with file entity IDs and resource keys, and remap tables for both") {
        nlohmann::json data = {
            {"10", "wall_mesh"},
            {"20", "floor_mesh"}
        };

        auto context = build_context(
            {{"10", 100}, {"20", 200}},
            "MeshSources",
            {{"wall_mesh", 5}, {"floor_mesh", 7}}
        );

        ResourceStore<FakeResource> resource_store;
        auto entry = cask::describe_resource_components<FakeResource>("MeshComponents", "MeshSources", resource_store);

        WHEN("the json is deserialized") {
            ComponentStore<ResourceHandle<FakeResource>> comp_store;
            entry.deserialize(data, &comp_store, context);

            THEN("remapped entity 100 has the remapped wall_mesh handle") {
                REQUIRE(comp_store.has(100));
                REQUIRE(comp_store.get(100).value == 5);
            }

            THEN("remapped entity 200 has the remapped floor_mesh handle") {
                REQUIRE(comp_store.has(200));
                REQUIRE(comp_store.get(200).value == 7);
            }
        }
    }
}

SCENARIO("resource components deserialization with identity entity remap", "[resource_components_serialization]") {
    GIVEN("json data and identity entity remap with real resource remap") {
        nlohmann::json data = {
            {"42", "wall_mesh"}
        };

        auto context = build_context(
            {{"42", 42}},
            "MeshSources",
            {{"wall_mesh", 3}}
        );

        ResourceStore<FakeResource> resource_store;
        auto entry = cask::describe_resource_components<FakeResource>("MeshComponents", "MeshSources", resource_store);

        WHEN("the json is deserialized") {
            ComponentStore<ResourceHandle<FakeResource>> comp_store;
            entry.deserialize(data, &comp_store, context);

            THEN("entity 42 has the resource handle from resource remap") {
                REQUIRE(comp_store.has(42));
                REQUIRE(comp_store.get(42).value == 3);
            }
        }
    }
}

SCENARIO("resource components deserialization throws when entity_remap is missing", "[resource_components_serialization]") {
    GIVEN("json data and a context missing entity_remap") {
        nlohmann::json data = {
            {"42", "wall_mesh"}
        };

        nlohmann::json context = {
            {"resource_remap_MeshSources", {{"wall_mesh", 3}}}
        };

        ResourceStore<FakeResource> resource_store;
        auto entry = cask::describe_resource_components<FakeResource>("MeshComponents", "MeshSources", resource_store);

        WHEN("deserialization is attempted") {
            ComponentStore<ResourceHandle<FakeResource>> comp_store;

            THEN("it throws") {
                REQUIRE_THROWS(entry.deserialize(data, &comp_store, context));
            }
        }
    }
}

SCENARIO("resource components deserialization throws when resource_remap is missing", "[resource_components_serialization]") {
    GIVEN("json data and a context missing resource_remap") {
        nlohmann::json data = {
            {"42", "wall_mesh"}
        };

        nlohmann::json context = {
            {"entity_remap", {{"42", 42}}}
        };

        ResourceStore<FakeResource> resource_store;
        auto entry = cask::describe_resource_components<FakeResource>("MeshComponents", "MeshSources", resource_store);

        WHEN("deserialization is attempted") {
            ComponentStore<ResourceHandle<FakeResource>> comp_store;

            THEN("it throws") {
                REQUIRE_THROWS(entry.deserialize(data, &comp_store, context));
            }
        }
    }
}

SCENARIO("resource components entry has correct schema metadata", "[resource_components_serialization]") {
    GIVEN("a described resource components entry") {
        ResourceStore<FakeResource> resource_store;
        auto entry = cask::describe_resource_components<FakeResource>("MeshComponents", "MeshSources", resource_store);

        THEN("it identifies as a component_store container") {
            REQUIRE(entry.schema["container"] == "component_store");
        }

        THEN("the value_type is resource_handle") {
            REQUIRE(entry.schema["value_type"] == "resource_handle");
        }

        THEN("the schema name matches the registration name") {
            REQUIRE(entry.schema["name"] == "MeshComponents");
        }
    }
}

SCENARIO("resource components entry depends on EntityRegistry and sources name", "[resource_components_serialization]") {
    GIVEN("a described resource components entry") {
        ResourceStore<FakeResource> resource_store;
        auto entry = cask::describe_resource_components<FakeResource>("MeshComponents", "MeshSources", resource_store);

        THEN("the entry has two dependencies") {
            REQUIRE(entry.dependencies.size() == 2);
        }

        THEN("it depends on EntityRegistry") {
            REQUIRE(std::find(entry.dependencies.begin(), entry.dependencies.end(), "EntityRegistry") != entry.dependencies.end());
        }

        THEN("it depends on the sources name") {
            REQUIRE(std::find(entry.dependencies.begin(), entry.dependencies.end(), "MeshSources") != entry.dependencies.end());
        }
    }
}

SCENARIO("empty resource components store serializes to empty json", "[resource_components_serialization]") {
    GIVEN("an empty component store of resource handles") {
        ComponentStore<ResourceHandle<FakeResource>> comp_store;

        ResourceStore<FakeResource> resource_store;
        auto entry = cask::describe_resource_components<FakeResource>("MeshComponents", "MeshSources", resource_store);

        WHEN("the empty store is serialized") {
            auto data = entry.serialize(&comp_store);

            THEN("the result is an empty json object") {
                REQUIRE(data.is_object());
                REQUIRE(data.empty());
            }
        }
    }
}

SCENARIO("resource components round-trips through serialization", "[resource_components_serialization]") {
    GIVEN("a component store with resource handle entries") {
        ResourceStore<FakeResource> resource_store;
        auto wall_handle = resource_store.store("wall_mesh", FakeResource{10});
        auto floor_handle = resource_store.store("floor_mesh", FakeResource{20});

        ComponentStore<ResourceHandle<FakeResource>> original;
        original.insert(42, wall_handle);
        original.insert(17, floor_handle);

        auto entry = cask::describe_resource_components<FakeResource>("MeshComponents", "MeshSources", resource_store);

        WHEN("the store is serialized then deserialized with correct remaps") {
            auto data = entry.serialize(&original);

            auto context = build_context(
                {{"42", 42}, {"17", 17}},
                "MeshSources",
                {{"wall_mesh", wall_handle.value}, {"floor_mesh", floor_handle.value}}
            );

            ComponentStore<ResourceHandle<FakeResource>> restored;
            entry.deserialize(data, &restored, context);

            THEN("entity 42 has the same resource handle") {
                REQUIRE(restored.has(42));
                REQUIRE(restored.get(42).value == wall_handle.value);
            }

            THEN("entity 17 has the same resource handle") {
                REQUIRE(restored.has(17));
                REQUIRE(restored.get(17).value == floor_handle.value);
            }
        }
    }
}
