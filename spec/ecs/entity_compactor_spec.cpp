#include <catch2/catch_all.hpp>
#include <cask/ecs/entity_compactor.hpp>
#include <cask/ecs/component_store.hpp>
#include <cask/ecs/entity_table.hpp>
#include <cask/event/event_queue.hpp>

struct Position { float x; float y; };
struct Velocity { float dx; float dy; };
struct EntityDestroyedEvent { uint32_t entity; };

SCENARIO("compact removes destroyed entity from all registered stores", "[entity_compactor]") {
    GIVEN("three entities with components in two stores and a destruction event queued") {
        EntityTable table;
        auto entity_a = table.create();
        auto entity_b = table.create();
        auto entity_c = table.create();

        ComponentStore<Position> positions;
        positions.insert(entity_a, Position{1.0f, 2.0f});
        positions.insert(entity_b, Position{3.0f, 4.0f});
        positions.insert(entity_c, Position{5.0f, 6.0f});

        ComponentStore<Velocity> velocities;
        velocities.insert(entity_a, Velocity{0.1f, 0.2f});
        velocities.insert(entity_b, Velocity{0.3f, 0.4f});
        velocities.insert(entity_c, Velocity{0.5f, 0.6f});

        EventQueue<EntityDestroyedEvent> destroy_queue;
        destroy_queue.emit(EntityDestroyedEvent{entity_b});
        destroy_queue.swap();

        EntityCompactor compactor{&table};
        compactor.add(&positions, remove_component<Position>);
        compactor.add(&velocities, remove_component<Velocity>);

        WHEN("compact is called with the destruction event queue") {
            compactor.compact(destroy_queue);

            THEN("both stores have two entries remaining") {
                REQUIRE(positions.dense_.size() == 2);
                REQUIRE(velocities.dense_.size() == 2);
            }

            THEN("entity A retains correct position data") {
                auto& pos = positions.get(entity_a);
                REQUIRE(pos.x == 1.0f);
                REQUIRE(pos.y == 2.0f);
            }

            THEN("entity C retains correct position data") {
                auto& pos = positions.get(entity_c);
                REQUIRE(pos.x == 5.0f);
                REQUIRE(pos.y == 6.0f);
            }

            THEN("entity A retains correct velocity data") {
                auto& vel = velocities.get(entity_a);
                REQUIRE(vel.dx == 0.1f);
                REQUIRE(vel.dy == 0.2f);
            }

            THEN("entity C retains correct velocity data") {
                auto& vel = velocities.get(entity_c);
                REQUIRE(vel.dx == 0.5f);
                REQUIRE(vel.dy == 0.6f);
            }

            THEN("the destroyed entity is no longer alive in the table") {
                REQUIRE_FALSE(table.alive(entity_b));
            }
        }
    }
}
