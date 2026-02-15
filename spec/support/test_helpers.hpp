#pragma once

#include <cask/engine/engine.hpp>
#include <cask/plugin/registry.hpp>

inline void wire_systems(PluginRegistry& registry, Engine& engine) {
    registry.initialize(engine.world());
    for (const auto* plugin : registry.plugins()) {
        System system;
        system.tick_fn = plugin->tick_fn;
        system.frame_fn = plugin->frame_fn;
        if (system.tick_fn || system.frame_fn) {
            engine.add_system(system);
        }
    }
}

struct FakeClock {
    float current_time = 0.0f;

    float get_time() {
        return current_time;
    }
};
