#include <catch2/catch_test_macros.hpp>
#include <cask/abi.h>
#include <cask/world.hpp>
#include <cask/world/abi_internal.hpp>
#include <cask/event/event_queue.hpp>
#include <cask/event/event_swapper.hpp>
#include "../support/test_helpers.hpp"

struct DamageEvent {
    int amount;
};

static EventSwapper event_swapper;
static uint32_t event_swapper_id;

static EventQueue<DamageEvent> damage_queue;
static uint32_t damage_events_id;

static int total_damage_received;

void event_swap_init(WorldHandle handle) {
    cask::WorldView world(handle);
    event_swapper = EventSwapper{};
    event_swapper_id = world.register_component("EventSwapper");
    world.bind(event_swapper_id, &event_swapper);
}

void event_swap_tick(WorldHandle handle) {
    cask::WorldView world(handle);
    auto* swapper = world.get<EventSwapper>(event_swapper_id);
    swapper->swap_all();
}

void damage_events_init(WorldHandle handle) {
    cask::WorldView world(handle);
    damage_queue = EventQueue<DamageEvent>{};
    damage_events_id = world.register_component("DamageEvents");
    world.bind(damage_events_id, &damage_queue);

    auto* swapper = world.get<EventSwapper>(event_swapper_id);
    swapper->add(&damage_queue, swap_queue<DamageEvent>);
}

void damage_producer_tick(WorldHandle handle) {
    cask::WorldView world(handle);
    auto* queue = world.get<EventQueue<DamageEvent>>(damage_events_id);
    queue->emit(DamageEvent{10});
}

void damage_consumer_tick(WorldHandle handle) {
    cask::WorldView world(handle);
    auto* queue = world.get<EventQueue<DamageEvent>>(damage_events_id);
    for (const auto& event : queue->poll()) {
        total_damage_received += event.amount;
    }
}

static const char* swap_defines[] = {"EventSwapper"};

static PluginInfo event_swap_plugin = {
    .name = "EventSwapPlugin",
    .defines_components = swap_defines,
    .requires_components = nullptr,
    .defines_count = 1,
    .requires_count = 0,
    .init_fn = event_swap_init,
    .tick_fn = event_swap_tick,
    .frame_fn = nullptr,
    .shutdown_fn = nullptr
};

static const char* damage_events_defines[] = {"DamageEvents"};
static const char* damage_events_requires[] = {"EventSwapper"};

static PluginInfo damage_events_plugin = {
    .name = "DamageEventsPlugin",
    .defines_components = damage_events_defines,
    .requires_components = damage_events_requires,
    .defines_count = 1,
    .requires_count = 1,
    .init_fn = damage_events_init,
    .tick_fn = nullptr,
    .frame_fn = nullptr,
    .shutdown_fn = nullptr
};

static const char* producer_requires[] = {"DamageEvents"};

static PluginInfo damage_producer_plugin = {
    .name = "DamageProducerPlugin",
    .defines_components = nullptr,
    .requires_components = producer_requires,
    .defines_count = 0,
    .requires_count = 1,
    .init_fn = nullptr,
    .tick_fn = damage_producer_tick,
    .frame_fn = nullptr,
    .shutdown_fn = nullptr
};

static const char* consumer_requires[] = {"DamageEvents"};

static PluginInfo damage_consumer_plugin = {
    .name = "DamageConsumerPlugin",
    .defines_components = nullptr,
    .requires_components = consumer_requires,
    .defines_count = 0,
    .requires_count = 1,
    .init_fn = nullptr,
    .tick_fn = damage_consumer_tick,
    .frame_fn = nullptr,
    .shutdown_fn = nullptr
};

struct InputEvent {
    int key_code;
};

namespace frame_swap_test {

static EventSwapper frame_event_swapper;
static uint32_t frame_event_swapper_id;

static EventSwapper tick_event_swapper;
static uint32_t tick_event_swapper_id;

static EventQueue<InputEvent> input_queue;
static uint32_t input_events_id;

static int frame_events_received;

void frame_swap_init(WorldHandle handle) {
    cask::WorldView world(handle);
    frame_event_swapper = EventSwapper{};
    frame_event_swapper_id = world.register_component("FrameEventSwapper");
    world.bind(frame_event_swapper_id, &frame_event_swapper);
}

void frame_swap_frame(WorldHandle handle, float) {
    cask::WorldView world(handle);
    auto* swapper = world.get<EventSwapper>(frame_event_swapper_id);
    swapper->swap_all();
}

void tick_swap_init(WorldHandle handle) {
    cask::WorldView world(handle);
    tick_event_swapper = EventSwapper{};
    tick_event_swapper_id = world.register_component("TickEventSwapper");
    world.bind(tick_event_swapper_id, &tick_event_swapper);
}

void tick_swap_tick(WorldHandle handle) {
    cask::WorldView world(handle);
    auto* swapper = world.get<EventSwapper>(tick_event_swapper_id);
    swapper->swap_all();
}

void input_events_init(WorldHandle handle) {
    cask::WorldView world(handle);
    input_queue = EventQueue<InputEvent>{};
    input_events_id = world.register_component("InputEvents");
    world.bind(input_events_id, &input_queue);

    auto* swapper = world.get<EventSwapper>(frame_event_swapper_id);
    swapper->add(&input_queue, swap_queue<InputEvent>);
}

void input_producer_frame(WorldHandle handle, float) {
    cask::WorldView world(handle);
    auto* queue = world.get<EventQueue<InputEvent>>(input_events_id);
    queue->emit(InputEvent{1});
}

void input_consumer_frame(WorldHandle handle, float) {
    cask::WorldView world(handle);
    auto* queue = world.get<EventQueue<InputEvent>>(input_events_id);
    for (const auto& event : queue->poll()) {
        frame_events_received += event.key_code;
    }
}

static const char* frame_swap_defines[] = {"FrameEventSwapper"};

static PluginInfo frame_event_swap_plugin = {
    .name = "FrameEventSwapPlugin",
    .defines_components = frame_swap_defines,
    .requires_components = nullptr,
    .defines_count = 1,
    .requires_count = 0,
    .init_fn = frame_swap_init,
    .tick_fn = nullptr,
    .frame_fn = frame_swap_frame,
    .shutdown_fn = nullptr
};

static const char* tick_swap_defines[] = {"TickEventSwapper"};

static PluginInfo tick_event_swap_plugin = {
    .name = "TickEventSwapPlugin",
    .defines_components = tick_swap_defines,
    .requires_components = nullptr,
    .defines_count = 1,
    .requires_count = 0,
    .init_fn = tick_swap_init,
    .tick_fn = tick_swap_tick,
    .frame_fn = nullptr,
    .shutdown_fn = nullptr
};

static const char* input_events_defines[] = {"InputEvents"};
static const char* input_events_requires[] = {"FrameEventSwapper"};

static PluginInfo input_events_plugin = {
    .name = "InputEventsPlugin",
    .defines_components = input_events_defines,
    .requires_components = input_events_requires,
    .defines_count = 1,
    .requires_count = 1,
    .init_fn = input_events_init,
    .tick_fn = nullptr,
    .frame_fn = nullptr,
    .shutdown_fn = nullptr
};

static const char* input_producer_requires[] = {"InputEvents"};

static PluginInfo input_producer_plugin = {
    .name = "InputProducerPlugin",
    .defines_components = nullptr,
    .requires_components = input_producer_requires,
    .defines_count = 0,
    .requires_count = 1,
    .init_fn = nullptr,
    .tick_fn = nullptr,
    .frame_fn = input_producer_frame,
    .shutdown_fn = nullptr
};

static const char* input_consumer_requires[] = {"InputEvents"};

static PluginInfo input_consumer_plugin = {
    .name = "InputConsumerPlugin",
    .defines_components = nullptr,
    .requires_components = input_consumer_requires,
    .defines_count = 0,
    .requires_count = 1,
    .init_fn = nullptr,
    .tick_fn = nullptr,
    .frame_fn = input_consumer_frame,
    .shutdown_fn = nullptr
};

}

SCENARIO("frame events swap per frame independently of tick timing", "[event_integration]") {
    GIVEN("separate tick and frame event swappers with input events on the frame swapper") {
        frame_swap_test::frame_events_received = 0;

        PluginRegistry registry;
        registry.add(&frame_swap_test::input_consumer_plugin);
        registry.add(&frame_swap_test::input_producer_plugin);
        registry.add(&frame_swap_test::tick_event_swap_plugin);
        registry.add(&frame_swap_test::input_events_plugin);
        registry.add(&frame_swap_test::frame_event_swap_plugin);

        Engine engine;
        wire_systems(registry, engine);

        WHEN("three frames run without any ticks firing") {
            FakeClock clock;
            clock.current_time = 0.0f;
            engine.step(clock, 1.0f);

            clock.current_time = 0.3f;
            engine.step(clock, 1.0f);

            clock.current_time = 0.6f;
            engine.step(clock, 1.0f);

            THEN("the consumer received frame events from prior frames") {
                REQUIRE(frame_swap_test::frame_events_received == 2);
            }
        }
    }
}

SCENARIO("event system flows through plugin dependency ordering", "[event_integration]") {
    GIVEN("plugins registered in wrong order with the registry") {
        total_damage_received = 0;

        PluginRegistry registry;
        registry.add(&damage_consumer_plugin);
        registry.add(&damage_producer_plugin);
        registry.add(&damage_events_plugin);
        registry.add(&event_swap_plugin);

        Engine engine;
        wire_systems(registry, engine);

        WHEN("the engine runs two ticks") {
            FakeClock clock;
            clock.current_time = 0.0f;
            engine.step(clock, 1.0f);

            clock.current_time = 1.0f;
            engine.step(clock, 1.0f);

            clock.current_time = 2.0f;
            engine.step(clock, 1.0f);

            THEN("the consumer received damage events from the first tick") {
                REQUIRE(total_damage_received == 10);
            }
        }
    }
}
