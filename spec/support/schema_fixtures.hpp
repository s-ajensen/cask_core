#pragma once

#include <cask/schema/describe.hpp>

namespace fixtures {

struct Position {
    float x;
    float y;
};

inline cask::RegistryEntry position_entry() {
    return cask::describe<Position>("Position", {
        cask::field("x", &Position::x),
        cask::field("y", &Position::y)
    });
}

struct PhysicsConfig {
    float gravity;
};

inline cask::RegistryEntry physics_config_entry() {
    return cask::describe<PhysicsConfig>("PhysicsConfig", {
        cask::field("gravity", &PhysicsConfig::gravity)
    });
}

}
