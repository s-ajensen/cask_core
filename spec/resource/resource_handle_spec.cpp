#include <catch2/catch_all.hpp>
#include <cask/resource/resource_handle.hpp>

struct MeshTag {};
struct TextureTag {};

SCENARIO("a resource handle wraps a uint32_t value", "[resource_handle]") {
    GIVEN("a resource handle constructed with a value") {
        ResourceHandle<MeshTag> handle{42};

        WHEN("the value is accessed") {
            uint32_t result = handle.value;

            THEN("it equals the constructed value") {
                REQUIRE(result == 42);
            }
        }
    }
}

SCENARIO("handles with different tags are distinct types", "[resource_handle]") {
    GIVEN("two handles with different tags and different values") {
        ResourceHandle<MeshTag> mesh_handle{1};
        ResourceHandle<TextureTag> texture_handle{2};

        WHEN("each value is accessed") {
            uint32_t mesh_value = mesh_handle.value;
            uint32_t texture_value = texture_handle.value;

            THEN("each handle retains its own value") {
                REQUIRE(mesh_value == 1);
                REQUIRE(texture_value == 2);
            }
        }
    }
}
