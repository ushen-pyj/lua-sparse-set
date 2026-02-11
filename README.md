# Lua Sparse Set

A high-performance Sparse Set data structure implementation for Lua, with support for storing arbitrary Lua data.

[中文文档](README_CN.md)

## Features

- ✅ **O(1) operations** for add, remove, and query
- ✅ **Store any Lua data type**: strings, tables, numbers, functions, booleans, etc.
- ✅ **Fast index-based access**: `set:get(index)`
- ✅ **Memory efficient**: sparse indices with compact storage
- ✅ **Automatic memory management**: handled by Lua GC
- ✅ **Multiple access patterns**: by index, by position, iterate all

## What is a Sparse Set?

Sparse Set uses two arrays:
- **sparse array**: maps index (key) to position in dense array
- **dense array**: stores actual indices (keys) compactly
- **data array**: stores corresponding Lua data references

Advantages:
- O(1) query, add, and remove operations
- Fast iteration over all elements (only iterate dense array)
- Perfect for scenarios with frequent add/remove and need for fast iteration

## Build

```bash
make
```

## Run Tests

```bash
make test
```

## Run Examples

```bash
lua example.lua
```

## API Reference

### Create Sparse Set

```lua
local sparseset = require("sparseset")
local set = sparseset.new(max_index, capacity)
```

- `max_index`: Maximum allowed index value (e.g., 1000 means indices 0-1000)
- `capacity`: Maximum number of elements that can be stored

### Methods

#### `set:add(index, data)`
Add data at specified index

```lua
set:add(100, "hello")               -- store string
set:add(200, {x = 10, y = 20})      -- store table
set:add(300, 42)                    -- store number
set:add(400, function() end)        -- store function
```

**Returns**: `true` on success, `false` if index exists or capacity full

#### `set:get(index)`
Get data by index

```lua
local data = set:get(100)     -- returns stored data
local nil_val = set:get(999)  -- returns nil if not exists
```

**Returns**: stored data, or `nil` if index doesn't exist

#### `set:contains(index)`
Check if index exists

```lua
if set:contains(100) then
    print("Index 100 exists")
end
```

**Returns**: `true` if exists, `false` otherwise

#### `set:remove(index)`
Remove data at index

```lua
set:remove(100)
```

**Returns**: `true` if removed, `false` if index doesn't exist

#### `set:size()`
Get current number of elements

```lua
local count = set:size()
```

**Returns**: integer, current element count

#### `set:at(position)`
Access data by position (0-based)

```lua
-- iterate all data
for i = 0, set:size() - 1 do
    local data = set:at(i)
    print(data)
end
```

**Returns**: data at position, or `nil` if out of range

#### `set:indices()`
Get array of all indices

```lua
local indices = set:indices()
for _, idx in ipairs(indices) do
    print("Index:", idx)
end
```

**Returns**: Lua table containing all indices (1-based array)

#### `set:pairs()`
Get table of all index-data pairs

```lua
local all = set:pairs()
for index, data in pairs(all) do
    print(index, data)
end
```

**Returns**: Lua table with indices as keys and data as values

#### `set:clear()`
Clear all data

```lua
set:clear()
```

## Usage Examples

### Basic Usage

```lua
local sparseset = require("sparseset")

-- create sparse set
local set = sparseset.new(1000, 100)

-- add different types of data
set:add(1, "Player Name")
set:add(5, {hp = 100, mp = 50})
set:add(10, 42)

-- get data
print(set:get(1))        -- "Player Name"
print(set:get(5).hp)     -- 100

-- check existence
if set:contains(5) then
    print("Entity 5 exists")
end

-- remove data
set:remove(1)
```

### ECS (Entity Component System)

```lua
-- position component
local positions = sparseset.new(10000, 1000)

-- add position components to entities
positions:add(1, {x = 100, y = 200})
positions:add(5, {x = 50, y = 150})
positions:add(10, {x = 300, y = 400})

-- update system: iterate all entities with positions
for i = 0, positions:size() - 1 do
    local entity_id = positions:indices()[i + 1]
    local pos = positions:get(entity_id)
    
    -- update position
    pos.x = pos.x + 10
    pos.y = pos.y + 5
    
    print(string.format("Entity %d moved to (%d, %d)", 
        entity_id, pos.x, pos.y))
end
```

### Object Pool

```lua
-- create object pool
local pool = sparseset.new(1000, 100)

-- allocate object
local function allocate_object(id, obj)
    if pool:add(id, obj) then
        return true
    end
    return false
end

-- free object
local function free_object(id)
    pool:remove(id)
end

-- get object
local function get_object(id)
    return pool:get(id)
end

-- usage
allocate_object(1, {type = "enemy", health = 100})
allocate_object(2, {type = "player", health = 200})

local obj = get_object(1)
print(obj.type)  -- "enemy"

free_object(1)
```

## Performance

| Operation | Time Complexity | Notes |
|-----------|----------------|-------|
| add | O(1) | Add element |
| remove | O(1) | Remove element (swap with last) |
| contains | O(1) | Check existence |
| get | O(1) | Get data |
| size | O(1) | Get size |
| iterate | O(n) | n is actual element count |

## Use Cases

1. **ECS (Entity Component System)**: Store component data, entity ID as index
2. **Object Pool Management**: Fast allocation and deallocation
3. **Sparse Data Collections**: Large index range with few actual elements
4. **Frequently Iterated Collections**: Faster iteration than hash tables
5. **Game Development**: Entity management, component systems, resource pools

## Notes

1. **Index Range**: `max_index` determines valid index range [0, max_index]
2. **Capacity Limit**: `capacity` determines maximum number of elements
3. **Memory Usage**: sparse array size is `(max_index + 1) * 4 bytes`, suitable for sparse but bounded indices
4. **Data References**: Stored Lua data is properly referenced and won't be GC'd
5. **Thread Safety**: Not thread-safe, requires external synchronization

## License

MIT License
