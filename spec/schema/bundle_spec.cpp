#include <catch2/catch_all.hpp>
#include <cask/ecs/component_store.hpp>
#include <cask/ecs/entity_table.hpp>
#include <cask/identity/entity_registry.hpp>
#include <cask/identity/uuid.hpp>
#include <cask/schema/bundle.hpp>
#include <cask/schema/describe_component_store.hpp>
#include <cask/schema/describe_entity_registry.hpp>
#include "../support/schema_fixtures.hpp"

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
