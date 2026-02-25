#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

template<typename Resource>
struct ResourceSources {
    std::unordered_map<std::string, nlohmann::json> entries;
};
