-- Add build folder to cpath for loading .so files
package.cpath = package.cpath .. ";../build/?.so"

local sparseset = require("sparseset")

local function test_basic_operations()
    print("Testing basic operations...")
    
    -- 创建一个sparse set: max_size=50 (C自动分配索引)
    local set = sparseset.new(50)
    
    -- 1. 添加不同类型的数据 (C自动分配索引)
    local idx1 = set:add("hello world")
    local idx2 = set:add({name = "player1", hp = 100, mp = 50})
    local idx3 = set:add(42)
    local idx4 = set:add(function(x) return x * 2 end)
    local idx5 = set:add(true)
    
    assert(idx1 ~= nil)
    assert(idx2 ~= nil)
    assert(idx3 ~= nil)
    assert(idx4 ~= nil)
    assert(idx5 ~= nil)
    print("Allocated indices:", idx1, idx2, idx3, idx4, idx5)
    assert(set:size() == 5)
    
    -- 2. 通过索引获取数据
    assert(set:get(idx1) == "hello world")
    local player = set:get(idx2)
    assert(player.name == "player1")
    assert(player.hp == 100)
    assert(player.mp == 50)
    assert(set:get(idx3) == 42)
    assert(set:get(idx4)(5) == 10) -- function test
    assert(set:get(idx5) == true)
    assert(set:get(999) == nil) -- 不存在
    
    -- 3. 检查索引是否存在
    assert(set:contains(idx1) == true)
    assert(set:contains(idx2) == true)
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
    
    -- 6. 遍历所有数据 (pairs方法)
    local all = set:pairs()
    assert(all[idx1] == "hello world")
    assert(all[idx2].name == "player1")
    assert(all[idx3] == 42)
    assert(all[idx5] == true)
    
    -- 7. 删除数据
    assert(set:size() == 5)
    assert(set:remove(idx2) == true)
    assert(set:size() == 4)
    assert(set:contains(idx2) == false)
    assert(set:get(idx2) == nil)
    assert(set:remove(idx2) == false) -- already removed
    
    -- 8. 清空数据
    set:clear()
    assert(set:size() == 0)
    assert(set:contains(idx1) == false)
    
    print("Basic operations test passed!")
end

local function test_ecs_scenario()
    print("Testing ECS scenario...")
    
    -- 使用场景示例 - ECS 组件存储
    local entities = sparseset.new(100)
    
    -- 添加位置组件 (C自动分配索引)
    local entity1 = entities:add({x = 10, y = 20})
    local entity2 = entities:add({x = 100, y = 200})
    local entity3 = entities:add({x = 50, y = 150})
    
    assert(entity1 ~= nil)
    assert(entity2 ~= nil)
    assert(entity3 ~= nil)
    assert(entities:size() == 3)
    
    -- 验证实体位置
    local pos1 = entities:get(entity1)
    assert(pos1.x == 10 and pos1.y == 20)
    
    local pos2 = entities:get(entity2)
    assert(pos2.x == 100 and pos2.y == 200)
    
    local pos3 = entities:get(entity3)
    assert(pos3.x == 50 and pos3.y == 150)
    
    -- 遍历所有实体
    local count = 0
    for i = 0, entities:size() - 1 do
        local idx = entities:indices()[i + 1]
        local pos = entities:get(idx)
        assert(pos.x ~= nil and pos.y ~= nil)
        count = count + 1
    end
    assert(count == 3)
    
    print("ECS scenario test passed!")
end

return {
    test_basic_operations = test_basic_operations,
    test_ecs_scenario = test_ecs_scenario
}
