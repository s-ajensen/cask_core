cmake_minimum_required(VERSION 3.14)
project(cask_core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    cask_redux
    GIT_REPOSITORY git@github.com:s-ajensen/cask_redux.git
    GIT_TAG master
)
FetchContent_MakeAvailable(cask_redux)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

add_library(cask_core INTERFACE)
target_include_directories(cask_core INTERFACE include)
target_link_libraries(cask_core INTERFACE cask_redux)

add_executable(cask_core_tests
    spec/event/event_queue_spec.cpp
    spec/event/event_swapper_spec.cpp
    spec/event/event_integration_spec.cpp
    spec/ecs/component_store_spec.cpp
    spec/ecs/entity_table_spec.cpp
    spec/ecs/entity_compactor_spec.cpp
    spec/ecs/ecs_integration_spec.cpp
)
target_link_libraries(cask_core_tests PRIVATE cask_core Catch2::Catch2WithMain)

include(CTest)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(cask_core_tests)
