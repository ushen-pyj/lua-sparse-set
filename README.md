# lua-sparse-set

一个面向 ECS 场景的 Lua C 扩展：

- `registry` 负责生成/回收版本化 ID。
- `set` 负责 O(1) 插入、查询、删除与迭代。

## API

### 模块函数

- `sparseset.new_registry()`：创建一个新的 ID 注册表（不接受参数）。
- `sparseset.new_set([stride])`：创建一个稀疏集合。

### 类型常量

- `sparseset.TYPE_INT = 1`
- `sparseset.TYPE_FLOAT = 2`
- `sparseset.TYPE_DOUBLE = 3`
- `sparseset.TYPE_BYTE = 4`
- `sparseset.TYPE_BOOL = 5`

### Registry 方法

- `reg:create()`：分配一个新 ID。
  - 成功：返回 `id`
  - 失败：返回 `nil, "oom"`
- `reg:destroy(id)`：回收一个 ID。
- `reg:valid(id)`：检查 ID 是否有效。

### Sparse Set 方法

#### 通用方法

- `set:insert(id, value)`：插入或更新 ID 对应的数据。
  - 成功插入新元素：返回 `true`
  - 成功更新已有元素：返回 `false`
  - 失败：返回 `nil, "oom"`
- `set:remove(id)`：移除 ID 及其关联值。
  - 存在并删除成功：`true`
  - 不存在：`false`
- `set:get(id)`：获取 ID 关联值。
  - 存在：返回值
  - 不存在：`nil`
- `set:contains(id)`：检查集合是否包含该 ID。
- `set:size()`：集合元素数量。
- `set:index_of(id)`：返回位置（从 1 开始），不存在返回 `nil`。
- `set:at(index)`：返回 `id, value`。
- `set:swap(i, j)`：交换两个位置。
- `set:iter()`：返回迭代器（`index, id, value`）。

#### 定长二进制模式 (`stride > 0`) 专用

- `set:get_field(id, offset, type)`：按偏移读取字段。
  - `id` 不存在：返回 `nil`
  - `stride == 0`、类型非法、越界：抛出错误
- `set:set_field(id, offset, type, value)`：按偏移写字段。
  - 写入成功：`true`
  - `id` 不存在：`false`
  - `stride == 0`、类型非法、越界：抛出错误

字段边界规则：`offset + sizeof(type) <= stride`，否则报错。

`TYPE_BOOL` 占 1 字节：

- 写入时按 Lua truthy 规则映射为 `0/1`
- 读取时返回 Lua `boolean`

## 两种使用模式

### 1. Lua 值模式（默认）

```lua
local sparse_set = require("sparseset")

local reg = sparse_set.new_registry()
local set = sparse_set.new_set()

local id = reg:create()
set:insert(id, { hp = 100 })

print(set:get(id).hp) -- 100
```

### 2. 定长二进制模式（高性能组件存储）

```lua
local sparse_set = require("sparseset")

local reg = sparse_set.new_registry()
local set = sparse_set.new_set(8) -- 例如两个 int

local id = reg:create()
set:insert(id, string.pack("ii", 10, 20))

set:set_field(id, 4, sparse_set.TYPE_INT, 99)
print(set:get_field(id, 4, sparse_set.TYPE_INT)) -- 99
```
