#pragma once

#include <bitset>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <vector>

using Signature = std::bitset<64>;

struct EntityTable {
    uint32_t next_id_ = 0;
    std::queue<uint32_t> recycled_;
    std::unordered_map<uint32_t, Signature> signatures_;
    std::vector<uint32_t> query_results_;

    uint32_t next_entity_id() {
        if (recycled_.empty()) {
            return next_id_++;
        }
        uint32_t recycled_id = recycled_.front();
        recycled_.pop();
        return recycled_id;
    }

    uint32_t create() {
        uint32_t entity = next_entity_id();
        signatures_[entity] = Signature{};
        return entity;
    }

    void destroy(uint32_t entity) {
        signatures_.erase(entity);
        recycled_.push(entity);
    }

    bool alive(uint32_t entity) {
        return signatures_.count(entity) > 0;
    }

    void add_component(uint32_t entity, uint32_t component_bit) {
        signatures_[entity].set(component_bit);
    }

    const std::vector<uint32_t>& query(const Signature& query_sig) {
        query_results_.clear();
        for (auto& [entity, entity_sig] : signatures_) {
            if ((entity_sig & query_sig) == query_sig) {
                query_results_.push_back(entity);
            }
        }
        return query_results_;
    }
};
