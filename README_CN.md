# Lua Sparse Set - 支持存储 Lua 数据的稀疏集合

这是一个高性能的 Sparse Set 数据结构的 Lua 绑定，支持通过索引存储和获取任意 Lua 数据。

## 特性

- ✅ **O(1) 时间复杂度** 的添加、删除、查询操作
- ✅ **支持存储任意 Lua 数据类型**：字符串、表、数字、函数、布尔值等
- ✅ **通过索引快速访问数据**：`set:get(index)`
- ✅ **紧凑的内存布局**：支持稀疏索引但占用连续内存
- ✅ **自动内存管理**：通过 Lua GC 自动释放资源
- ✅ **多种访问方式**：按索引访问、按位置访问、遍历所有数据

## 数据结构说明

Sparse Set 使用两个数组实现：
- **sparse 数组**：通过索引(key)快速查找在 dense 中的位置
- **dense 数组**：紧密存储实际的索引(key)
- **data 数组**：存储对应的 Lua 数据引用

优势：
- 查询、添加、删除都是 O(1) 时间复杂度
- 可以快速遍历所有元素（只需遍历 dense 数组）
- 适合需要频繁添加/删除且需要快速遍历的场景

## 编译

```bash
make
```

## 运行测试

```bash
make test
```

## 运行示例

```bash
lua example.lua
```

## API 文档

### 创建 Sparse Set

```lua
local sparseset = require("sparseset")
local set = sparseset.new(max_index, capacity)
```

- `max_index`: 最大允许的索引值（例如 1000 表示索引范围 0-1000）
- `capacity`: 最多可以存储的元素数量

### 方法

#### `set:add(index, data)` 
添加数据到指定索引

```lua
set:add(100, "hello")               -- 存储字符串
set:add(200, {x = 10, y = 20})      -- 存储表
set:add(300, 42)                    -- 存储数字
set:add(400, function() end)        -- 存储函数
```

**返回值**: `true` 成功，`false` 失败（索引已存在或容量已满）

#### `set:get(index)`
通过索引获取数据

```lua
local data = set:get(100)  -- 返回之前存储的数据
local nil_val = set:get(999)  -- 索引不存在返回 nil
```

**返回值**: 存储的数据，或 `nil`（索引不存在）

#### `set:contains(index)`
检查索引是否存在

```lua
if set:contains(100) then
    print("Index 100 exists")
end
```

**返回值**: `true` 存在，`false` 不存在

#### `set:remove(index)`
删除指定索引的数据

```lua
set:remove(100)
```

**返回值**: `true` 成功删除，`false` 索引不存在

#### `set:size()`
获取当前存储的元素数量

```lua
local count = set:size()
```

**返回值**: 整数，当前元素数量

#### `set:at(position)`
通过位置索引访问数据（0-based）

```lua
-- 遍历所有数据
for i = 0, set:size() - 1 do
    local data = set:at(i)
    print(data)
end
```

**返回值**: 指定位置的数据，或 `nil`（位置超出范围）

#### `set:indices()`
获取所有索引的数组

```lua
local indices = set:indices()
for _, idx in ipairs(indices) do
    print("Index:", idx)
end
```

**返回值**: Lua 表，包含所有索引（1-based 数组）

#### `set:pairs()`
获取所有索引-数据对的表

```lua
local all = set:pairs()
for index, data in pairs(all) do
    print(index, data)
end
```

**返回值**: Lua 表，键为索引，值为对应的数据

#### `set:clear()`
清空所有数据

```lua
set:clear()
```

## 使用示例

### 基本用法

```lua
local sparseset = require("sparseset")

-- 创建 sparse set
local set = sparseset.new(1000, 100)

-- 添加不同类型的数据
set:add(1, "Player Name")
set:add(5, {hp = 100, mp = 50})
set:add(10, 42)

-- 获取数据
print(set:get(1))        -- "Player Name"
print(set:get(5).hp)     -- 100

-- 检查存在
if set:contains(5) then
    print("Entity 5 exists")
end

-- 删除数据
set:remove(1)
```

### ECS (Entity Component System) 场景

```lua
-- 位置组件
local positions = sparseset.new(10000, 1000)

-- 为实体添加位置组件
positions:add(1, {x = 100, y = 200})
positions:add(5, {x = 50, y = 150})
positions:add(10, {x = 300, y = 400})

-- 更新系统：遍历所有有位置的实体
for i = 0, positions:size() - 1 do
    local entity_id = positions:indices()[i + 1]
    local pos = positions:get(entity_id)
    
    -- 更新位置
    pos.x = pos.x + 10
    pos.y = pos.y + 5
    
    print(string.format("Entity %d moved to (%d, %d)", 
        entity_id, pos.x, pos.y))
end
```

### 对象池

```lua
-- 创建对象池
local pool = sparseset.new(1000, 100)

-- 分配对象
local function allocate_object(id, obj)
    if pool:add(id, obj) then
        return true
    end
    return false
end

-- 释放对象
local function free_object(id)
    pool:remove(id)
end

-- 获取对象
local function get_object(id)
    return pool:get(id)
end

-- 使用
allocate_object(1, {type = "enemy", health = 100})
allocate_object(2, {type = "player", health = 200})

local obj = get_object(1)
print(obj.type)  -- "enemy"

free_object(1)
```

## 性能特点

| 操作 | 时间复杂度 | 说明 |
|------|-----------|------|
| add | O(1) | 添加元素 |
| remove | O(1) | 删除元素（交换到末尾） |
| contains | O(1) | 检查是否存在 |
| get | O(1) | 获取数据 |
| size | O(1) | 获取大小 |
| 遍历 | O(n) | n 为实际元素数量 |

## 适用场景

1. **ECS（实体组件系统）**: 存储组件数据，实体 ID 作为索引
2. **对象池管理**: 快速分配和回收对象
3. **稀疏数据集合**: 索引范围大但实际数据少的场景
4. **需要频繁遍历的集合**: 比普通哈希表遍历更快
5. **游戏开发**: 实体管理、组件系统、资源池等

## 注意事项

1. **索引范围**: 创建时指定的 `max_index` 决定了可用的索引范围 [0, max_index]
2. **容量限制**: `capacity` 决定了最多可以存储多少个元素
3. **内存使用**: sparse 数组大小为 `(max_index + 1) * 4 bytes`，适合稀疏但索引范围可控的场景
4. **数据引用**: 存储的 Lua 数据会被正确引用，不会被 GC 回收
5. **并发安全**: 不是线程安全的，需要外部同步

## 许可证

MIT License
