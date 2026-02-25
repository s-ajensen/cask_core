cmake_minimum_required(VERSION 3.14)
project(cask_core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    cask_engine
    GIT_REPOSITORY git@github.com:s-ajensen/cask_engine.git
    GIT_TAG master
)
FetchContent_MakeAvailable(cask_engine)

FetchContent_Declare(
    stduuid
    GIT_REPOSITORY https://github.com/mariusbancila/stduuid.git
    GIT_TAG v1.2.3
)
set(UUID_USING_CXX20_SPAN ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(stduuid)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

add_library(cask_core INTERFACE)
target_include_directories(cask_core INTERFACE include)
target_link_libraries(cask_core INTERFACE cask_engine stduuid nlohmann_json::nlohmann_json)

add_executable(cask_core_tests
    spec/event/event_queue_spec.cpp
    spec/event/event_swapper_spec.cpp
    spec/event/event_integration_spec.cpp
    spec/ecs/component_store_spec.cpp
    spec/ecs/entity_table_spec.cpp
    spec/ecs/entity_compactor_spec.cpp
    spec/ecs/ecs_integration_spec.cpp
    spec/ecs/interpolated_spec.cpp
    spec/ecs/frame_advancer_spec.cpp
    spec/resource/resource_store_spec.cpp
    spec/resource/mesh_data_spec.cpp
    spec/resource/texture_data_spec.cpp
    spec/resource/resource_handle_spec.cpp
    spec/resource/resource_loader_registry_spec.cpp
    spec/identity/uuid_spec.cpp
    spec/identity/entity_registry_spec.cpp
    spec/schema/type_name_spec.cpp
    spec/schema/serialization_registry_spec.cpp
    spec/schema/describe_spec.cpp
    spec/schema/component_store_serialization_spec.cpp
    spec/schema/entity_registry_serialization_spec.cpp
    spec/schema/resource_sources_serialization_spec.cpp
    spec/schema/resource_components_serialization_spec.cpp
    spec/schema/loader_spec.cpp
    spec/schema/saver_spec.cpp
    spec/schema/bundle_spec.cpp
    spec/schema/cask_component_spec.cpp
)
target_link_libraries(cask_core_tests PRIVATE cask_core Catch2::Catch2WithMain)

include(CTest)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(cask_core_tests)
