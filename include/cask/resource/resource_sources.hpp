#pragma once

#include <string>
#include <unordered_map>

template<typename Resource>
struct ResourceSources {
    std::unordered_map<std::string, std::string> entries;
};
