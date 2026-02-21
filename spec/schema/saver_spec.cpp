#include <catch2/catch_all.hpp>
#include <cask/ecs/component_store.hpp>
#include <cask/ecs/entity_table.hpp>
#include <cask/identity/entity_registry.hpp>
#include <cask/identity/uuid.hpp>
#include <cask/schema/describe_component_store.hpp>
#include <cask/schema/describe_entity_registry.hpp>
#include <cask/schema/loader.hpp>
#include <cask/schema/saver.hpp>
#include "../support/schema_fixtures.hpp"

using fixtures::PhysicsConfig;
using fixtures::physics_config_entry;
using fixtures::Position;
using fixtures::position_entry;

SCENARIO("saver produces file-format json", "[saver]") {
    GIVEN("a physics config singleton and its registry entry") {
        PhysicsConfig config{9.8f};
        auto entry = physics_config_entry();

        cask::SerializationRegistry registry;
        registry.add("PhysicsConfig", entry);

        cask::ComponentResolver resolver = [&](const std::string&) -> void* {
            return &config;
        };

        WHEN("save is called") {
            auto result = cask::save({"PhysicsConfig"}, registry, resolver);

            THEN("the output has a components section") {
                REQUIRE(result.contains("components"));
                REQUIRE(result["components"].contains("PhysicsConfig"));
                REQUIRE(result["components"]["PhysicsConfig"]["gravity"] == Catch::Approx(9.8));
            }

            THEN("the output has a dependencies section") {
                REQUIRE(result.contains("dependencies"));
            }

            THEN("PhysicsConfig has no dependencies listed") {
                REQUIRE_FALSE(result["dependencies"].contains("PhysicsConfig"));
            }
        }
    }
}

SCENARIO("saver includes dependencies for component stores", "[saver]") {
    GIVEN("a component store entry with EntityRegistry dependency") {
        auto val_entry = position_entry();
        auto store_entry = cask::describe_component_store<Position>("Positions", val_entry);

        ComponentStore<Position> store;
        store.insert(1, Position{1.0f, 2.0f});

        cask::SerializationRegistry registry;
        registry.add("Positions", store_entry);

        cask::ComponentResolver resolver = [&](const std::string&) -> void* {
            return &store;
        };

        WHEN("save is called") {
            auto result = cask::save({"Positions"}, registry, resolver);

            THEN("the dependencies section lists EntityRegistry for Positions") {
                REQUIRE(result["dependencies"].contains("Positions"));
                REQUIRE(result["dependencies"]["Positions"].size() == 1);
                REQUIRE(result["dependencies"]["Positions"][0] == "EntityRegistry");
            }
        }
    }
}

SCENARIO("save then load round-trips component data", "[saver]") {
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

        WHEN("the state is saved then loaded into fresh instances") {
            auto file_data = cask::save({"EntityRegistry", "Positions"}, serialization_registry, save_resolver);

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

            cask::load(file_data, fresh_serialization, load_resolver);

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
        }
    }
}
