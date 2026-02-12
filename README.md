# lua-sparse-set
## API

### 模块函数

- `sparseset.new_registry(max_size)`: 创建一个新的 ID 注册表。
- `sparseset.new_set(max_size)`: 创建一个新的稀疏集合。

### Registry 方法

- `reg:create()`: 分配一个新 ID。
- `reg:destroy(id)`: 回收一个 ID。
- `reg:valid(id)`: 检查 ID 是否有效。

### Sparse Set 方法

- `set:insert(id, value)`: 插入 ID 并关联一个 Lua 值。
- `set:remove(id)`: 移除 ID 及其关联值。
- `set:get(id)`: 获取 ID 关联的 Lua 值。
- `set:contains(id)`: 检查集合是否包含该 ID。
- `set:size()`: 获取集合中的元素数量。
- `set:iter()`: 返回一个迭代器，用于遍历集合。

## 示例

```lua
local sparse_set = require("sparseset")

local size = 100
local reg = sparse_set.new_registry(size)
local set = sparse_set.new_set(size)

-- 创建 ID
local id1 = reg:create()
local id2 = reg:create()

-- 插入数据 (ID, Value)
set:insert(id1, "hello")
set:insert(id2, { world = true })

-- 获取数据
print(set:get(id1)) -- "hello"

-- 检查包含
if set:contains(id2) then
    print("set contains id2")
end

-- 遍历 (索引, ID, 数据)
for i, id, data in set:iter() do
    print(i, id, data)
end

-- 移除
set:remove(id1)
reg:destroy(id1)
```
