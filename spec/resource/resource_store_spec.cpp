#include <catch2/catch_all.hpp>
#include <cask/resource/resource_handle.hpp>
#include <cask/resource/resource_store.hpp>

struct TestResource {
    int value;
};

SCENARIO("store returns a handle and get retrieves the data", "[resource_store]") {
    GIVEN("a resource store with a stored resource") {
        ResourceStore<TestResource> store;
        ResourceHandle<TestResource> handle = store.store("test_asset", TestResource{42});

        WHEN("the resource is retrieved by handle") {
            auto& resource = store.get(handle);

            THEN("the retrieved data matches what was stored") {
                REQUIRE(resource.value == 42);
            }
        }
    }
}

SCENARIO("storing the same key twice returns the same handle", "[resource_store]") {
    GIVEN("a resource store with a resource stored under a key") {
        ResourceStore<TestResource> store;
        ResourceHandle<TestResource> first_handle = store.store("shared_asset", TestResource{100});

        WHEN("the same key is stored again with different data") {
            ResourceHandle<TestResource> second_handle = store.store("shared_asset", TestResource{999});

            THEN("both calls return the same handle") {
                REQUIRE(first_handle.value == second_handle.value);
            }

            THEN("get returns the first stored data") {
                auto& resource = store.get(first_handle);
                REQUIRE(resource.value == 100);
            }
        }
    }
}
