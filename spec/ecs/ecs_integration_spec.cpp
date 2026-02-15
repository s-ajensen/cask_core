#include <catch2/catch_test_macros.hpp>
#include <cask/abi.h>
#include <cask/world.hpp>
#include <cask/world/abi_internal.hpp>
#include <cask/ecs/component_store.hpp>
#include <cask/ecs/entity_compactor.hpp>
#include <cask/ecs/entity_table.hpp>
#include <cask/event/event_queue.hpp>
#include <cask/event/event_swapper.hpp>
#include "../support/test_helpers.hpp"

namespace ecs_integration {

struct Position {
    float x;
    float y;
};

struct EntityDestroyedEvent {
    uint32_t entity;
};

static EventSwapper event_swapper;
static uint32_t event_swapper_id;

static EntityTable table;
static uint32_t table_id;

static EventQueue<EntityDestroyedEvent> destroy_queue;
static uint32_t destroy_queue_id;

static EntityCompactor compactor{&table};
static uint32_t compactor_id;

static ComponentStore<Position> positions;
static uint32_t positions_id;

static uint32_t entity_a;
static uint32_t entity_b;
static uint32_t entity_c;

static int tick_count;

void event_swap_init(WorldHandle handle) {
    cask::WorldView world(handle);
    event_swapper = EventSwapper{};
    event_swapper_id = world.register_component("TickEventSwapper");
    world.bind(event_swapper_id, &event_swapper);
}

void event_swap_tick(WorldHandle handle) {
    cask::WorldView world(handle);
    auto* swapper = world.get<EventSwapper>(event_swapper_id);
    swapper->swap_all();
}

void entity_table_init(WorldHandle handle) {
    cask::WorldView world(handle);
    table = EntityTable{};
    table_id = world.register_component("EntityTable");
    world.bind(table_id, &table);
}

void destroy_events_init(WorldHandle handle) {
    cask::WorldView world(handle);
    destroy_queue = EventQueue<EntityDestroyedEvent>{};
    destroy_queue_id = world.register_component("DestroyEvents");
    world.bind(destroy_queue_id, &destroy_queue);

    auto* swapper = world.get<EventSwapper>(event_swapper_id);
    swapper->add(&destroy_queue, swap_queue<EntityDestroyedEvent>);
}

void compactor_init(WorldHandle handle) {
    cask::WorldView world(handle);
    compactor = EntityCompactor{&table};
    compactor.add(&positions, remove_component<Position>);
    compactor_id = world.register_component("EntityCompactor");
    world.bind(compactor_id, &compactor);
}

void compactor_tick(WorldHandle handle) {
    cask::WorldView world(handle);
    auto* comp = world.get<EntityCompactor>(compactor_id);
    auto* queue = world.get<EventQueue<EntityDestroyedEvent>>(destroy_queue_id);
    comp->compact(*queue);
}

void game_init(WorldHandle handle) {
    cask::WorldView world(handle);
    positions = ComponentStore<Position>{};

    auto* tbl = world.get<EntityTable>(table_id);
    entity_a = tbl->create();
    entity_b = tbl->create();
    entity_c = tbl->create();

    positions.insert(entity_a, Position{1.0f, 2.0f});
    positions.insert(entity_b, Position{3.0f, 4.0f});
    positions.insert(entity_c, Position{5.0f, 6.0f});

    positions_id = world.register_component("Positions");
    world.bind(positions_id, &positions);
}

void game_tick(WorldHandle handle) {
    cask::WorldView world(handle);
    tick_count++;
    if (tick_count == 2) {
        auto* queue = world.get<EventQueue<EntityDestroyedEvent>>(destroy_queue_id);
        queue->emit(EntityDestroyedEvent{entity_b});
    }
}

static const char* event_swap_defines[] = {"TickEventSwapper"};

static PluginInfo event_swap_plugin = {
    .name = "EventSwapPlugin",
    .defines_components = event_swap_defines,
    .requires_components = nullptr,
    .defines_count = 1,
    .requires_count = 0,
    .init_fn = event_swap_init,
    .tick_fn = event_swap_tick,
    .frame_fn = nullptr,
    .shutdown_fn = nullptr
};

static const char* entity_table_defines[] = {"EntityTable"};

static PluginInfo entity_table_plugin = {
    .name = "EntityTablePlugin",
    .defines_components = entity_table_defines,
    .requires_components = nullptr,
    .defines_count = 1,
    .requires_count = 0,
    .init_fn = entity_table_init,
    .tick_fn = nullptr,
    .frame_fn = nullptr,
    .shutdown_fn = nullptr
};

static const char* destroy_events_defines[] = {"DestroyEvents"};
static const char* destroy_events_requires[] = {"TickEventSwapper"};

static PluginInfo destroy_events_plugin = {
    .name = "DestroyEventsPlugin",
    .defines_components = destroy_events_defines,
    .requires_components = destroy_events_requires,
    .defines_count = 1,
    .requires_count = 1,
    .init_fn = destroy_events_init,
    .tick_fn = nullptr,
    .frame_fn = nullptr,
    .shutdown_fn = nullptr
};

static const char* compactor_defines[] = {"EntityCompactor"};
static const char* compactor_requires[] = {"DestroyEvents", "EntityTable"};

static PluginInfo compactor_plugin = {
    .name = "CompactorPlugin",
    .defines_components = compactor_defines,
    .requires_components = compactor_requires,
    .defines_count = 1,
    .requires_count = 2,
    .init_fn = compactor_init,
    .tick_fn = compactor_tick,
    .frame_fn = nullptr,
    .shutdown_fn = nullptr
};

static const char* game_defines[] = {"Positions"};
static const char* game_requires[] = {"EntityTable", "DestroyEvents", "EntityCompactor"};

static PluginInfo game_plugin = {
    .name = "GamePlugin",
    .defines_components = game_defines,
    .requires_components = game_requires,
    .defines_count = 1,
    .requires_count = 3,
    .init_fn = game_init,
    .tick_fn = game_tick,
    .frame_fn = nullptr,
    .shutdown_fn = nullptr
};

}

SCENARIO("entity destruction flows through the full ECS pipeline", "[ecs_integration]") {
    GIVEN("five plugins registered in wrong order with dependency resolution") {
        ecs_integration::tick_count = 0;
        ecs_integration::event_swapper = EventSwapper{};
        ecs_integration::table = EntityTable{};
        ecs_integration::destroy_queue = EventQueue<ecs_integration::EntityDestroyedEvent>{};
        ecs_integration::compactor = EntityCompactor{&ecs_integration::table};
        ecs_integration::positions = ComponentStore<ecs_integration::Position>{};
        ecs_integration::entity_a = 0;
        ecs_integration::entity_b = 0;
        ecs_integration::entity_c = 0;

        PluginRegistry registry;
        registry.add(&ecs_integration::game_plugin);
        registry.add(&ecs_integration::compactor_plugin);
        registry.add(&ecs_integration::destroy_events_plugin);
        registry.add(&ecs_integration::entity_table_plugin);
        registry.add(&ecs_integration::event_swap_plugin);

        Engine engine;
        wire_systems(registry, engine);

        WHEN("the engine runs three ticks") {
            FakeClock clock;

            clock.current_time = 0.0f;
            engine.step(clock, 1.0f);

            clock.current_time = 1.0f;
            engine.step(clock, 1.0f);

            clock.current_time = 2.0f;
            engine.step(clock, 1.0f);

            clock.current_time = 3.0f;
            engine.step(clock, 1.0f);

            THEN("the position store has two entries after compaction") {
                REQUIRE(ecs_integration::positions.dense_.size() == 2);
            }

            THEN("entity B is no longer alive in the table") {
                REQUIRE_FALSE(ecs_integration::table.alive(ecs_integration::entity_b));
            }

            THEN("entity A retains correct position data") {
                auto& pos = ecs_integration::positions.get(ecs_integration::entity_a);
                REQUIRE(pos.x == 1.0f);
                REQUIRE(pos.y == 2.0f);
            }

            THEN("entity C retains correct position data") {
                auto& pos = ecs_integration::positions.get(ecs_integration::entity_c);
                REQUIRE(pos.x == 5.0f);
                REQUIRE(pos.y == 6.0f);
            }
        }
    }
}
