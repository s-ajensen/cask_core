#include <catch2/catch_all.hpp>
#include <cask/ecs/component_store.hpp>
#include <cask/ecs/entity_table.hpp>
#include <cask/identity/entity_registry.hpp>
#include <cask/identity/uuid.hpp>
#include <cask/schema/describe_component_store.hpp>
#include <cask/schema/describe_entity_registry.hpp>
#include <cask/schema/loader.hpp>
#include "../support/schema_fixtures.hpp"

using fixtures::PhysicsConfig;
using fixtures::physics_config_entry;
using fixtures::Position;
using fixtures::position_entry;

SCENARIO("loader deserializes components in dependency order", "[loader]") {
    GIVEN("an entity registry, component store, and serialized file data") {
        EntityTable table;
        EntityRegistry registry;

        auto uuid_a = cask::generate_uuid();
        auto uuid_b = cask::generate_uuid();
        uint32_t original_a = registry.resolve(uuid_a, table);
        uint32_t original_b = registry.resolve(uuid_b, table);

        auto val_entry = position_entry();
        auto store_entry = cask::describe_component_store<Position>("Positions", val_entry);
        auto reg_entry = cask::describe_entity_registry("EntityRegistry", table);

        ComponentStore<Position> original_store;
        original_store.insert(original_a, Position{1.0f, 2.0f});
        original_store.insert(original_b, Position{3.0f, 4.0f});

        nlohmann::json file_data = {
            {"dependencies", {
                {"Positions", {"EntityRegistry"}}
            }},
            {"components", {
                {"EntityRegistry", reg_entry.serialize(&registry)},
                {"Positions", store_entry.serialize(&original_store)}
            }}
        };

        EntityTable fresh_table;
        EntityRegistry fresh_registry;
        ComponentStore<Position> fresh_store;

        auto fresh_reg_entry = cask::describe_entity_registry("EntityRegistry", fresh_table);

        cask::SerializationRegistry serialization_registry;
        serialization_registry.add("EntityRegistry", fresh_reg_entry);
        serialization_registry.add("Positions", store_entry);

        cask::ComponentResolver resolver = [&](const std::string& name) -> void* {
            if (name == "EntityRegistry") return &fresh_registry;
            if (name == "Positions") return &fresh_store;
            return nullptr;
        };

        WHEN("the file is loaded") {
            auto context = cask::load(file_data, serialization_registry, resolver);

            THEN("the entity registry has the same UUIDs") {
                REQUIRE(fresh_registry.size() == 2);
            }

            THEN("the component store has remapped entities with correct data") {
                uint32_t new_a = fresh_registry.resolve(uuid_a, fresh_table);
                uint32_t new_b = fresh_registry.resolve(uuid_b, fresh_table);

                auto& pos_a = fresh_store.get(new_a);
                REQUIRE(pos_a.x == Catch::Approx(1.0));
                REQUIRE(pos_a.y == Catch::Approx(2.0));

                auto& pos_b = fresh_store.get(new_b);
                REQUIRE(pos_b.x == Catch::Approx(3.0));
                REQUIRE(pos_b.y == Catch::Approx(4.0));
            }

            THEN("the context contains entity_remap") {
                REQUIRE(context.contains("entity_remap"));
            }
        }
    }
}

SCENARIO("loader handles singletons with no dependencies", "[loader]") {
    GIVEN("a singleton component in the file data") {
        PhysicsConfig config{};

        auto entry = physics_config_entry();

        nlohmann::json file_data = {
            {"dependencies", nlohmann::json::object()},
            {"components", {
                {"PhysicsConfig", {{"gravity", 9.8}}}
            }}
        };

        cask::SerializationRegistry serialization_registry;
        serialization_registry.add("PhysicsConfig", entry);

        cask::ComponentResolver resolver = [&](const std::string&) -> void* {
            return &config;
        };

        WHEN("the file is loaded") {
            cask::load(file_data, serialization_registry, resolver);

            THEN("the singleton has the correct values") {
                REQUIRE(config.gravity == Catch::Approx(9.8));
            }
        }
    }
}

SCENARIO("loader detects circular dependencies", "[loader]") {
    GIVEN("file data with circular dependencies") {
        nlohmann::json file_data = {
            {"dependencies", {
                {"A", {"B"}},
                {"B", {"A"}}
            }},
            {"components", {
                {"A", nlohmann::json::object()},
                {"B", nlohmann::json::object()}
            }}
        };

        cask::RegistryEntry dummy{
            nlohmann::json{{"name", "dummy"}},
            [](const void*) { return nlohmann::json{}; },
            [](const nlohmann::json&, void*, const nlohmann::json&) { return nlohmann::json::object(); },
            {}
        };

        cask::SerializationRegistry serialization_registry;
        serialization_registry.add("A", dummy);
        serialization_registry.add("B", dummy);

        int dummy_val = 0;
        cask::ComponentResolver resolver = [&](const std::string&) -> void* {
            return &dummy_val;
        };

        WHEN("load is called") {
            THEN("it throws a cycle detection error") {
                REQUIRE_THROWS(cask::load(file_data, serialization_registry, resolver));
            }
        }
    }
}
