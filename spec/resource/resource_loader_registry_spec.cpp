#include <catch2/catch_all.hpp>
#include <cask/resource/resource_loader_registry.hpp>

namespace {
struct FakeResource {
    int data;
};
}

SCENARIO("add and get a loader", "[resource_loader_registry]") {
    GIVEN("a registry with a loader registered") {
        cask::ResourceLoaderRegistry<FakeResource> registry;
        registry.add("obj", [](const nlohmann::json& entry) {
            return FakeResource{entry["value"].get<int>()};
        });

        WHEN("the loader is retrieved and invoked") {
            auto& loader = registry.get("obj");
            auto result = loader(nlohmann::json{{"loader", "obj"}, {"value", 42}});

            THEN("it produces the expected resource") {
                REQUIRE(result.data == 42);
            }
        }
    }
}

SCENARIO("has returns true for registered loaders", "[resource_loader_registry]") {
    GIVEN("a registry with a loader registered") {
        cask::ResourceLoaderRegistry<FakeResource> registry;
        registry.add("obj", [](const nlohmann::json&) {
            return FakeResource{0};
        });

        WHEN("has is called with the registered name") {
            THEN("it returns true") {
                REQUIRE(registry.has("obj"));
            }
        }
    }
}

SCENARIO("has returns false for unregistered loaders", "[resource_loader_registry]") {
    GIVEN("a fresh registry") {
        cask::ResourceLoaderRegistry<FakeResource> registry;

        WHEN("has is called with a name that was never registered") {
            THEN("it returns false") {
                REQUIRE_FALSE(registry.has("nonexistent"));
            }
        }
    }
}

SCENARIO("get throws for missing loader", "[resource_loader_registry]") {
    GIVEN("a fresh registry") {
        cask::ResourceLoaderRegistry<FakeResource> registry;

        WHEN("get is called with an unregistered name") {
            THEN("it throws with a message containing the loader name") {
                REQUIRE_THROWS_WITH(registry.get("nonexistent"), Catch::Matchers::ContainsSubstring("nonexistent"));
            }
        }
    }
}

SCENARIO("multiple loaders coexist", "[resource_loader_registry]") {
    GIVEN("a registry with two loaders registered") {
        cask::ResourceLoaderRegistry<FakeResource> registry;
        registry.add("obj", [](const nlohmann::json& entry) {
            return FakeResource{entry["value"].get<int>()};
        });
        registry.add("inline", [](const nlohmann::json& entry) {
            return FakeResource{entry["inline_value"].get<int>() * 10};
        });

        WHEN("each loader is retrieved and invoked") {
            auto& obj_loader = registry.get("obj");
            auto& inline_loader = registry.get("inline");
            auto obj_result = obj_loader(nlohmann::json{{"loader", "obj"}, {"value", 5}});
            auto inline_result = inline_loader(nlohmann::json{{"loader", "inline"}, {"inline_value", 3}});

            THEN("each produces the correct resource") {
                REQUIRE(obj_result.data == 5);
                REQUIRE(inline_result.data == 30);
            }
        }
    }
}
