#include <catch2/catch_all.hpp>
#include <cask/ecs/component_store.hpp>
#include <cask/ecs/entity_table.hpp>
#include <cask/identity/entity_registry.hpp>
#include <cask/identity/uuid.hpp>
#include <cask/resource/resource_handle.hpp>
#include <cask/resource/resource_sources.hpp>
#include <cask/resource/resource_store.hpp>
#include <cask/schema/bundle.hpp>
#include <cask/schema/describe_component_store.hpp>
#include <cask/schema/describe_entity_registry.hpp>
#include <cask/schema/describe_resource_components.hpp>
#include <cask/schema/describe_resource_sources.hpp>
#include <cask/resource/resource_loader_registry.hpp>
#include "../support/schema_fixtures.hpp"

namespace {
struct FakeResource {
    int data;
};
}

using fixtures::PhysicsConfig;
using fixtures::physics_config_entry;
using fixtures::Position;
using fixtures::position_entry;

SCENARIO("load_bundle calls plugin loader for each plugin in order", "[bundle]") {
    GIVEN("bundle data with two plugins and a singleton component") {
        PhysicsConfig config{};
        auto entry = physics_config_entry();

        cask::SerializationRegistry serialization_registry;
        serialization_registry.add("PhysicsConfig", entry);

        cask::ComponentResolver resolver = [&](const std::string&) -> void* {
            return &config;
        };

        nlohmann::json bundle_data = {
            {"plugins", {"alpha", "beta"}},
            {"dependencies", nlohmann::json::object()},
            {"components", {
                {"PhysicsConfig", {{"gravity", 9.8}}}
            }}
        };

        std::vector<std::string> loaded_plugins;
        cask::PluginLoader plugin_loader = [&](const std::string& name) {
            loaded_plugins.push_back(name);
        };

        WHEN("load_bundle is called") {
            cask::load_bundle(bundle_data, serialization_registry, plugin_loader, resolver);

            THEN("the plugin loader is called with each plugin in order") {
                REQUIRE(loaded_plugins.size() == 2);
                REQUIRE(loaded_plugins[0] == "alpha");
                REQUIRE(loaded_plugins[1] == "beta");
            }
        }
    }
}

SCENARIO("load_bundle loads plugins before deserializing components", "[bundle]") {
    GIVEN("a plugin that installs a component store needed for deserialization") {
        EntityTable table;
        EntityRegistry entity_registry;

        auto uuid = cask::generate_uuid();
        uint32_t original_entity = entity_registry.resolve(uuid, table);

        ComponentStore<Position> original_store;
        original_store.insert(original_entity, Position{5.0f, 10.0f});

        auto val_entry = position_entry();
        auto store_entry = cask::describe_component_store<Position>("Positions", val_entry);
        auto reg_entry = cask::describe_entity_registry("EntityRegistry", table);

        EntityTable fresh_table;
        EntityRegistry fresh_registry;
        auto fresh_reg_entry = cask::describe_entity_registry("EntityRegistry", fresh_table);

        cask::SerializationRegistry serialization_registry;
        serialization_registry.add("EntityRegistry", fresh_reg_entry);
        serialization_registry.add("Positions", store_entry);

        ComponentStore<Position>* positions_store = nullptr;

        cask::ComponentResolver resolver = [&](const std::string& name) -> void* {
            if (name == "EntityRegistry") return &fresh_registry;
            if (name == "Positions") return positions_store;
            return nullptr;
        };

        cask::PluginLoader plugin_loader = [&](const std::string&) {
            positions_store = new ComponentStore<Position>();
        };

        nlohmann::json bundle_data = {
            {"plugins", {"positions_plugin"}},
            {"dependencies", {
                {"Positions", {"EntityRegistry"}}
            }},
            {"components", {
                {"EntityRegistry", reg_entry.serialize(&entity_registry)},
                {"Positions", store_entry.serialize(&original_store)}
            }}
        };

        WHEN("load_bundle is called") {
            cask::load_bundle(bundle_data, serialization_registry, plugin_loader, resolver);

            THEN("the component store has the deserialized position data") {
                REQUIRE(positions_store != nullptr);

                uint32_t fresh_entity = fresh_registry.resolve(uuid, fresh_table);
                auto& position = positions_store->get(fresh_entity);
                REQUIRE(position.x == Catch::Approx(5.0));
                REQUIRE(position.y == Catch::Approx(10.0));

                delete positions_store;
            }
        }
    }
}

SCENARIO("load_bundle returns context with entity_remap", "[bundle]") {
    GIVEN("bundle data containing an entity registry") {
        EntityTable original_table;
        EntityRegistry original_registry;

        auto uuid = cask::generate_uuid();
        original_registry.resolve(uuid, original_table);

        auto reg_entry = cask::describe_entity_registry("EntityRegistry", original_table);

        EntityTable fresh_table;
        EntityRegistry fresh_registry;
        auto fresh_reg_entry = cask::describe_entity_registry("EntityRegistry", fresh_table);

        cask::SerializationRegistry serialization_registry;
        serialization_registry.add("EntityRegistry", fresh_reg_entry);

        cask::ComponentResolver resolver = [&](const std::string&) -> void* {
            return &fresh_registry;
        };

        cask::PluginLoader plugin_loader = [](const std::string&) {};

        nlohmann::json bundle_data = {
            {"plugins", nlohmann::json::array()},
            {"dependencies", nlohmann::json::object()},
            {"components", {
                {"EntityRegistry", reg_entry.serialize(&original_registry)}
            }}
        };

        WHEN("load_bundle is called") {
            auto context = cask::load_bundle(bundle_data, serialization_registry, plugin_loader, resolver);

            THEN("the context contains entity_remap") {
                REQUIRE(context.contains("entity_remap"));
            }
        }
    }
}

SCENARIO("load_bundle handles absent plugins gracefully", "[bundle]") {
    PhysicsConfig config{};
    auto entry = physics_config_entry();

    cask::SerializationRegistry serialization_registry;
    serialization_registry.add("PhysicsConfig", entry);

    cask::ComponentResolver resolver = [&](const std::string&) -> void* {
        return &config;
    };

    std::vector<std::string> loaded_plugins;
    cask::PluginLoader plugin_loader = [&](const std::string& name) {
        loaded_plugins.push_back(name);
    };

    auto physics_component = nlohmann::json{{"PhysicsConfig", {{"gravity", 9.8}}}};
    auto no_dependencies = nlohmann::json::object();

    GIVEN("bundle data without a plugins key") {
        nlohmann::json bundle_data = {
            {"dependencies", no_dependencies},
            {"components", physics_component}
        };

        WHEN("load_bundle is called") {
            cask::load_bundle(bundle_data, serialization_registry, plugin_loader, resolver);

            THEN("no plugins are loaded") {
                REQUIRE(loaded_plugins.empty());
            }

            THEN("component data is deserialized correctly") {
                REQUIRE(config.gravity == Catch::Approx(9.8));
            }
        }
    }

    GIVEN("bundle data with an empty plugins array") {
        nlohmann::json bundle_data = {
            {"plugins", nlohmann::json::array()},
            {"dependencies", no_dependencies},
            {"components", physics_component}
        };

        WHEN("load_bundle is called") {
            cask::load_bundle(bundle_data, serialization_registry, plugin_loader, resolver);

            THEN("no plugins are loaded") {
                REQUIRE(loaded_plugins.empty());
            }

            THEN("component data is deserialized correctly") {
                REQUIRE(config.gravity == Catch::Approx(9.8));
            }
        }
    }
}

SCENARIO("save_bundle produces bundle json", "[bundle]") {
    PhysicsConfig config{9.8f};
    auto entry = physics_config_entry();

    cask::SerializationRegistry registry;
    registry.add("PhysicsConfig", entry);

    cask::ComponentResolver resolver = [&](const std::string&) -> void* {
        return &config;
    };

    std::vector<std::string> component_names = {"PhysicsConfig"};

    GIVEN("plugin names") {
        auto result = cask::save_bundle({"alpha", "beta"}, component_names, registry, resolver);

        THEN("the output contains the plugins list") {
            REQUIRE(result.contains("plugins"));
            REQUIRE(result["plugins"] == nlohmann::json({"alpha", "beta"}));
        }

        THEN("the output contains the components section") {
            REQUIRE(result.contains("components"));
        }

        THEN("the output contains the dependencies section") {
            REQUIRE(result.contains("dependencies"));
        }
    }

    GIVEN("no plugin names") {
        auto result = cask::save_bundle({}, component_names, registry, resolver);

        THEN("the output contains an empty plugins array") {
            REQUIRE(result.contains("plugins"));
            REQUIRE(result["plugins"].is_array());
            REQUIRE(result["plugins"].empty());
        }
    }
}

SCENARIO("save_bundle then load_bundle round-trips component data", "[bundle]") {
    GIVEN("an entity registry and component store with data") {
        EntityTable table;
        EntityRegistry entity_registry;

        auto uuid_a = cask::generate_uuid();
        auto uuid_b = cask::generate_uuid();
        uint32_t entity_a = entity_registry.resolve(uuid_a, table);
        uint32_t entity_b = entity_registry.resolve(uuid_b, table);

        auto val_entry = position_entry();
        auto store_entry = cask::describe_component_store<Position>("Positions", val_entry);
        auto reg_entry = cask::describe_entity_registry("EntityRegistry", table);

        ComponentStore<Position> store;
        store.insert(entity_a, Position{1.0f, 2.0f});
        store.insert(entity_b, Position{3.0f, 4.0f});

        cask::SerializationRegistry serialization_registry;
        serialization_registry.add("EntityRegistry", reg_entry);
        serialization_registry.add("Positions", store_entry);

        cask::ComponentResolver save_resolver = [&](const std::string& name) -> void* {
            if (name == "EntityRegistry") return &entity_registry;
            if (name == "Positions") return &store;
            return nullptr;
        };

        std::vector<std::string> plugin_names = {"alpha", "beta"};
        std::vector<std::string> component_names = {"EntityRegistry", "Positions"};

        WHEN("save_bundle then load_bundle is called on the result") {
            auto bundle_data = cask::save_bundle(plugin_names, component_names, serialization_registry, save_resolver);

            EntityTable fresh_table;
            EntityRegistry fresh_registry;
            ComponentStore<Position> fresh_store;

            auto fresh_reg_entry = cask::describe_entity_registry("EntityRegistry", fresh_table);
            cask::SerializationRegistry fresh_serialization;
            fresh_serialization.add("EntityRegistry", fresh_reg_entry);
            fresh_serialization.add("Positions", store_entry);

            cask::ComponentResolver load_resolver = [&](const std::string& name) -> void* {
                if (name == "EntityRegistry") return &fresh_registry;
                if (name == "Positions") return &fresh_store;
                return nullptr;
            };

            std::vector<std::string> loaded_plugins;
            cask::PluginLoader plugin_loader = [&](const std::string& name) {
                loaded_plugins.push_back(name);
            };

            cask::load_bundle(bundle_data, fresh_serialization, plugin_loader, load_resolver);

            THEN("the fresh registry has the same UUIDs") {
                REQUIRE(fresh_registry.size() == 2);
            }

            THEN("the fresh store has correct position data at remapped entities") {
                uint32_t new_a = fresh_registry.resolve(uuid_a, fresh_table);
                uint32_t new_b = fresh_registry.resolve(uuid_b, fresh_table);

                auto& pos_a = fresh_store.get(new_a);
                REQUIRE(pos_a.x == Catch::Approx(1.0));
                REQUIRE(pos_a.y == Catch::Approx(2.0));

                auto& pos_b = fresh_store.get(new_b);
                REQUIRE(pos_b.x == Catch::Approx(3.0));
                REQUIRE(pos_b.y == Catch::Approx(4.0));
            }

            THEN("the plugin loader received the correct plugins") {
                REQUIRE(loaded_plugins.size() == 2);
                REQUIRE(loaded_plugins[0] == "alpha");
                REQUIRE(loaded_plugins[1] == "beta");
            }
        }
    }
}

SCENARIO("save_bundle then load_bundle round-trips resource component data", "[bundle]") {
    GIVEN("entities linked to resources via component store") {
        EntityTable table;
        EntityRegistry entity_registry;

        auto uuid_a = cask::generate_uuid();
        auto uuid_b = cask::generate_uuid();
        uint32_t entity_a = entity_registry.resolve(uuid_a, table);
        uint32_t entity_b = entity_registry.resolve(uuid_b, table);

        ResourceStore<FakeResource> resource_store;
        auto wall_handle = resource_store.store("wall_mesh", FakeResource{42});
        auto floor_handle = resource_store.store("floor_mesh", FakeResource{99});

        ResourceSources<FakeResource> resource_sources;
        resource_sources.entries["wall_mesh"] = {{"loader", "obj"}, {"path", "assets/wall.obj"}};
        resource_sources.entries["floor_mesh"] = {{"loader", "obj"}, {"path", "assets/floor.obj"}};

        ComponentStore<ResourceHandle<FakeResource>> component_store;
        component_store.insert(entity_a, wall_handle);
        component_store.insert(entity_b, floor_handle);

        cask::ResourceLoaderRegistry<FakeResource> save_loader_registry;
        auto reg_entry = cask::describe_entity_registry("EntityRegistry", table);
        auto sources_entry = cask::describe_resource_sources<FakeResource>(
            "MeshSources", resource_store, save_loader_registry);
        auto components_entry = cask::describe_resource_components<FakeResource>(
            "MeshComponents", "MeshSources", resource_store);

        cask::SerializationRegistry save_registry;
        save_registry.add("EntityRegistry", reg_entry);
        save_registry.add("MeshSources", sources_entry);
        save_registry.add("MeshComponents", components_entry);

        cask::ComponentResolver save_resolver = [&](const std::string& name) -> void* {
            if (name == "EntityRegistry") return &entity_registry;
            if (name == "MeshSources") return &resource_sources;
            if (name == "MeshComponents") return &component_store;
            return nullptr;
        };

        std::vector<std::string> component_names = {"EntityRegistry", "MeshSources", "MeshComponents"};

        WHEN("save_bundle then load_bundle is called on the result") {
            auto bundle_data = cask::save_bundle({"mesh_plugin"}, component_names, save_registry, save_resolver);

            EntityTable fresh_table;
            EntityRegistry fresh_registry;
            ResourceStore<FakeResource> fresh_resource_store;
            ResourceSources<FakeResource> fresh_sources;
            ComponentStore<ResourceHandle<FakeResource>> fresh_component_store;

            std::vector<std::string> loaded_paths;
            cask::ResourceLoaderRegistry<FakeResource> fresh_loader_registry;
            fresh_loader_registry.add("obj", [&loaded_paths](const nlohmann::json& entry_json) -> FakeResource {
                std::string path = entry_json["path"].get<std::string>();
                loaded_paths.push_back(path);
                int derived = 0;
                for (char character : path) { derived += static_cast<int>(character); }
                return FakeResource{derived};
            });

            auto fresh_reg_entry = cask::describe_entity_registry("EntityRegistry", fresh_table);
            auto fresh_sources_entry = cask::describe_resource_sources<FakeResource>(
                "MeshSources", fresh_resource_store, fresh_loader_registry);
            auto fresh_components_entry = cask::describe_resource_components<FakeResource>(
                "MeshComponents", "MeshSources", fresh_resource_store);

            cask::SerializationRegistry load_registry;
            load_registry.add("EntityRegistry", fresh_reg_entry);
            load_registry.add("MeshSources", fresh_sources_entry);
            load_registry.add("MeshComponents", fresh_components_entry);

            cask::ComponentResolver load_resolver = [&](const std::string& name) -> void* {
                if (name == "EntityRegistry") return &fresh_registry;
                if (name == "MeshSources") return &fresh_sources;
                if (name == "MeshComponents") return &fresh_component_store;
                return nullptr;
            };

            cask::PluginLoader plugin_loader = [](const std::string&) {};

            cask::load_bundle(bundle_data, load_registry, plugin_loader, load_resolver);

            THEN("the fresh registry has the same two UUIDs") {
                REQUIRE(fresh_registry.size() == 2);
            }

            THEN("the loader was called with the correct paths") {
                REQUIRE(loaded_paths.size() == 2);
                bool has_wall = std::find(loaded_paths.begin(), loaded_paths.end(), "assets/wall.obj") != loaded_paths.end();
                bool has_floor = std::find(loaded_paths.begin(), loaded_paths.end(), "assets/floor.obj") != loaded_paths.end();
                REQUIRE(has_wall);
                REQUIRE(has_floor);
            }

            THEN("the resource store has the loaded data") {
                auto fresh_wall_handle = ResourceHandle<FakeResource>{fresh_resource_store.key_to_handle_.at("wall_mesh")};
                auto fresh_floor_handle = ResourceHandle<FakeResource>{fresh_resource_store.key_to_handle_.at("floor_mesh")};

                int wall_data = fresh_resource_store.get(fresh_wall_handle).data;
                int floor_data = fresh_resource_store.get(fresh_floor_handle).data;

                int expected_wall = 0;
                for (char character : std::string("assets/wall.obj")) {
                    expected_wall += static_cast<int>(character);
                }
                int expected_floor = 0;
                for (char character : std::string("assets/floor.obj")) {
                    expected_floor += static_cast<int>(character);
                }

                REQUIRE(wall_data == expected_wall);
                REQUIRE(floor_data == expected_floor);
            }

            THEN("the component store links remapped entities to remapped resource handles") {
                uint32_t new_a = fresh_registry.resolve(uuid_a, fresh_table);
                uint32_t new_b = fresh_registry.resolve(uuid_b, fresh_table);

                REQUIRE(fresh_component_store.has(new_a));
                REQUIRE(fresh_component_store.has(new_b));

                auto handle_a = fresh_component_store.get(new_a);
                auto handle_b = fresh_component_store.get(new_b);

                REQUIRE(fresh_resource_store.key(handle_a) == "wall_mesh");
                REQUIRE(fresh_resource_store.key(handle_b) == "floor_mesh");
            }
        }
    }
}
