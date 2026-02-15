#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

template<typename Component>
struct ComponentStore {
    std::vector<Component> dense_;
    std::unordered_map<uint32_t, size_t> entity_to_index_;
    std::unordered_map<size_t, uint32_t> index_to_entity_;

    void insert(uint32_t entity, Component data) {
        size_t index = dense_.size();
        dense_.push_back(data);
        entity_to_index_[entity] = index;
        index_to_entity_[index] = entity;
    }

    Component& get(uint32_t entity) {
        return dense_[entity_to_index_[entity]];
    }

    void remove(uint32_t entity) {
        size_t removed_index = entity_to_index_[entity];
        size_t last_index = dense_.size() - 1;
        uint32_t last_entity = index_to_entity_[last_index];

        dense_[removed_index] = dense_[last_index];

        entity_to_index_[last_entity] = removed_index;
        index_to_entity_[removed_index] = last_entity;

        entity_to_index_.erase(entity);
        index_to_entity_.erase(last_index);

        dense_.pop_back();
    }
};

using RemoveFn = void(*)(void*, uint32_t);

template<typename Component>
void remove_component(void* ptr, uint32_t entity) {
    static_cast<ComponentStore<Component>*>(ptr)->remove(entity);
}
