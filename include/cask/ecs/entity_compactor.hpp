#pragma once

#include <vector>
#include <cask/ecs/component_store.hpp>
#include <cask/ecs/entity_table.hpp>
#include <cask/event/event_queue.hpp>

struct EntityCompactor {
    struct Entry {
        void* store;
        RemoveFn fn;
    };

    EntityTable* table_;
    std::vector<Entry> entries_;

    void add(void* store, RemoveFn fn) {
        entries_.push_back(Entry{store, fn});
    }

    template<typename Event>
    void compact(EventQueue<Event>& queue) {
        for (auto& event : queue.poll()) {
            for (auto& entry : entries_) {
                entry.fn(entry.store, event.entity);
            }
            table_->destroy(event.entity);
        }
    }
};
