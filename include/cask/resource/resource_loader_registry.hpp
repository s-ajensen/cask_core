#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace cask {

template<typename Resource>
struct ResourceLoaderRegistry {
    using LoaderFn = std::function<Resource(const nlohmann::json&)>;

    std::unordered_map<std::string, LoaderFn> loaders_;

    void add(const std::string& name, LoaderFn loader) {
        loaders_.emplace(name, std::move(loader));
    }

    const LoaderFn& get(const std::string& name) const {
        auto found = loaders_.find(name);
        if (found == loaders_.end()) {
            throw std::runtime_error("No loader registered for: " + name);
        }
        return found->second;
    }

    bool has(const std::string& name) const {
        return loaders_.find(name) != loaders_.end();
    }
};

}
