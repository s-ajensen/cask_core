#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace cask {

using SerializeFn = std::function<nlohmann::json(const void*)>;
using DeserializeFn = std::function<nlohmann::json(const nlohmann::json&, void*, const nlohmann::json&)>;

inline std::string resource_remap_key(const std::string& name) {
    return "resource_remap_" + name;
}

struct RegistryEntry {
    nlohmann::json schema;
    SerializeFn serialize;
    DeserializeFn deserialize;
    std::vector<std::string> dependencies;
};

struct SerializationRegistry {
    std::unordered_map<std::string, RegistryEntry> entries_;

    void add(const std::string& name, RegistryEntry entry) {
        entries_.emplace(name, std::move(entry));
    }

    const RegistryEntry& get(const std::string& name) const {
        auto found = entries_.find(name);
        if (found == entries_.end()) {
            throw std::runtime_error("No registry entry for: " + name);
        }
        return found->second;
    }

    bool has(const std::string& name) const {
        return entries_.find(name) != entries_.end();
    }
};

}
