#pragma once

#include <cask/resource/resource_handle.hpp>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

template<typename Resource>
struct ResourceStore {
    std::vector<Resource> resources_;
    std::unordered_map<std::string, uint32_t> key_to_handle_;

    ResourceHandle<Resource> store(const std::string& key, Resource data) {
        auto existing = key_to_handle_.find(key);
        if (existing != key_to_handle_.end()) {
            return ResourceHandle<Resource>{existing->second};
        }
        uint32_t raw_handle = resources_.size();
        resources_.push_back(std::move(data));
        key_to_handle_[key] = raw_handle;
        return ResourceHandle<Resource>{raw_handle};
    }

    Resource& get(ResourceHandle<Resource> handle) {
        return resources_[handle.value];
    }
};
