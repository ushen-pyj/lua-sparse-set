-- Add build folder to cpath for loading .so files
package.cpath = package.cpath .. ";./build/?.so"

local sparseset = require("sparseset")

local function test_basic_operations()
    -- 创建一个sparse set: max_index=1000, capacity=50
    local set = sparseset.new(1000, 50)
    
    -- 1. 添加不同类型的数据
    assert(set:add(100, "hello world") == true)
    assert(set:add(200, {name = "player1", hp = 100, mp = 50}) == true)
    assert(set:add(300, 42) == true)
    assert(set:add(400, function(x) return x * 2 end) == true)
    assert(set:add(500, true) == true)
    assert(set:size() == 5)
    
    -- 2. 通过索引获取数据
    assert(set:get(100) == "hello world")
    local player = set:get(200)
    assert(player.name == "player1")
    assert(player.hp == 100)
    assert(player.mp == 50)
    assert(set:get(300) == 42)
    assert(set:get(400)(5) == 10) -- function test
    assert(set:get(500) == true)
    assert(set:get(999) == nil) -- 不存在
    
    -- 3. 检查索引是否存在
    assert(set:contains(100) == true)
    assert(set:contains(999) == false)
    
    -- 4. 通过位置访问 (at方法)
    for i = 0, set:size() - 1 do
        local data = set:at(i)
        assert(data ~= nil)
    end
    assert(set:at(5) == nil) -- out of range
    
    -- 5. 获取所有索引
    local indices = set:indices()
    assert(#indices == 5)
    assert(indices[1] == 100 or indices[2] == 100 or indices[3] == 100 or indices[4] == 100 or indices[5] == 100)
    
    -- 6. 遍历所有数据 (pairs方法)
    local all = set:pairs()
    assert(all[100] == "hello world")
    assert(all[200].name == "player1")
    assert(all[300] == 42)
    assert(all[500] == true)
    
    -- 7. 删除数据
    assert(set:size() == 5)
    assert(set:remove(200) == true)
    assert(set:size() == 4)
    assert(set:contains(200) == false)
    assert(set:get(200) == nil)
    assert(set:remove(200) == false) -- already removed
    
    -- 8. 清空数据
    set:clear()
    assert(set:size() == 0)
    assert(set:contains(100) == false)
end

local function test_ecs_scenario()
    -- 使用场景示例 - ECS 组件存储
    local entities = sparseset.new(10000, 100)
    
    -- 添加位置组件
    assert(entities:add(1, {x = 10, y = 20}) == true)
    assert(entities:add(5, {x = 100, y = 200}) == true)
    assert(entities:add(10, {x = 50, y = 150}) == true)
    assert(entities:size() == 3)
    
    -- 验证实体位置
    local pos1 = entities:get(1)
    assert(pos1.x == 10 and pos1.y == 20)
    
    local pos5 = entities:get(5)
    assert(pos5.x == 100 and pos5.y == 200)
    
    local pos10 = entities:get(10)
    assert(pos10.x == 50 and pos10.y == 150)
    
    -- 遍历所有实体
    local count = 0
    for i = 0, entities:size() - 1 do
        local idx = entities:indices()[i + 1]
        local pos = entities:get(idx)
        assert(pos.x ~= nil and pos.y ~= nil)
        count = count + 1
    end
    assert(count == 3)
end

print("Running sparse set tests...")
test_basic_operations()
test_ecs_scenario()
print("All tests passed!")
