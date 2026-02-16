# cask_lib

Header-only C++20 library providing ECS, events, and resource management for the Cask game engine.

## Events

### `EventQueue<Event>`

Double-buffered typed event queue. Events emitted during the current frame are readable next frame.

```cpp
EventQueue<DamageEvent> damage_events;
damage_events.emit(DamageEvent{entity, 30.0f});  // write to current buffer
damage_events.swap();                              // rotate buffers
for (auto& event : damage_events.poll()) { ... }   // read previous buffer
```

### `EventSwapper`

Type-erased batch swap for all registered event queues. Call once per tick to rotate every queue.

```cpp
EventSwapper swapper;
swapper.add(&damage_events, swap_queue<DamageEvent>);
swapper.add(&spawn_events, swap_queue<SpawnEvent>);
swapper.swap_all();
```

## ECS

### `ComponentStore<Component>`

Packed array with entity-to-index mapping. Stores components contiguously for cache-friendly iteration, with O(1) insert, get, and swap-remove.

```cpp
ComponentStore<Position> positions;
positions.insert(entity, Position{0, 0, 0});
Position& pos = positions.get(entity);
positions.remove(entity);
```

### `EntityTable`

Entity ID allocation with signature-based queries. Manages creation, destruction, ID recycling, and component bitset signatures.

```cpp
EntityTable table;
uint32_t entity = table.create();
table.add_component(entity, POSITION_BIT);
table.add_component(entity, VELOCITY_BIT);

Signature query_sig;
query_sig.set(POSITION_BIT);
query_sig.set(VELOCITY_BIT);
for (uint32_t entity : table.query(query_sig)) { ... }
```

### `Interpolated<ValueType>`

Double-snapshot container for frame interpolation. Holds `previous` and `current` values; `advance()` copies current into previous.

```cpp
Interpolated<float> health{100.0f, 100.0f};
health.current = 70.0f;
health.advance();  // previous = 70.0f, current = 70.0f
```

### `FrameAdvancer`

Type-erased batch advance for all registered interpolated values. Call once per tick.

```cpp
FrameAdvancer advancer;
advancer.add(&health, advance_interpolated<float>);
advancer.advance_all();
```

### `EntityCompactor`

Deferred entity destruction driven by events. Removes components from all registered stores and destroys the entity.

```cpp
EntityCompactor compactor;
compactor.table_ = &table;
compactor.add(&positions, remove_component<Position>);
compactor.add(&velocities, remove_component<Velocity>);
compactor.compact(destroy_events);
```

## Resources

### `ResourceHandle<Tag>`

Strongly-typed handle wrapping a `uint32_t`. The tag parameter produces distinct types per resource, preventing accidental misuse across stores.

```cpp
using MeshHandle = ResourceHandle<MeshData>;       // defined in mesh_data.hpp
using TextureHandle = ResourceHandle<TextureData>; // defined in texture_data.hpp
```

### `ResourceStore<Resource>`

Generic keyed resource storage with deduplication. Stores resources by string key, returns typed handles, and deduplicates on repeated keys.

```cpp
ResourceStore<MeshData> meshes;
MeshHandle handle = meshes.store("cube.obj", mesh_data);
MeshHandle same = meshes.store("cube.obj", other_data);  // returns same handle
MeshData& mesh = meshes.get(handle);
```

### `MeshData`

Self-validating CPU-side mesh geometry. Separate flat arrays per attribute (SOA) for renderer flexibility.

- `positions` - `vector<float>`, 3 per vertex (required, must be non-empty and divisible by 3)
- `indices` - `vector<uint32_t>` (required, must be non-empty)
- `normals` - `vector<float>`, 3 per vertex (optional, must match positions size if present)
- `uvs` - `vector<float>`, 2 per vertex (optional, must match vertex count if present)

```cpp
MeshData mesh(positions, indices);                    // minimal
MeshData mesh(positions, indices, normals, uvs);      // full
mesh.has_normals();  // false if normals empty
mesh.has_uvs();      // false if uvs empty
```

### `TextureData`

Self-validating CPU-side pixel data. Raw RGBA bytes in scanline order.

- `width` - pixel columns (must be > 0)
- `height` - pixel rows (must be > 0)
- `channels` - 1 (grayscale), 3 (RGB), or 4 (RGBA)
- `pixels` - `vector<uint8_t>` (size must equal width * height * channels)

```cpp
TextureData texture(width, height, channels, pixels);
```

## Entity-Resource Association

Resources and entities are decoupled. `ResourceStore` holds resources as world-level components. `ComponentStore` associates entities with resource handles.

```cpp
ResourceStore<MeshData> mesh_store;
ComponentStore<MeshHandle> mesh_refs;

MeshHandle handle = mesh_store.store("cube.obj", mesh_data);
mesh_refs.insert(entity, handle);

MeshHandle h = mesh_refs.get(entity);
MeshData& mesh = mesh_store.get(h);
```
