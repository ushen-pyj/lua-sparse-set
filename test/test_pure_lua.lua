-- test/test_pure_lua.lua
local sparseset = require("lua-sparse-set")

local function test_sparse_set_pure()
    print("Testing Sparse Set (Pure Lua) with Lua Data...")
    
    local set = sparseset.new(10) -- max_size=10 (soft limit in Lua version)
    
    -- Test add with different data types
    local idx1 = set:add("hello")
    local idx2 = set:add({x = 100, y = 200})
    local idx3 = set:add(42)
    assert(idx1 == 0, "Expected idx1=0, got "..tostring(idx1))
    assert(idx2 == 1, "Expected idx2=1, got "..tostring(idx2))
    assert(idx3 == 2, "Expected idx3=2, got "..tostring(idx3))
    print("Allocated indices:", idx1, idx2, idx3)
    assert(set:size() == 3)
    
    -- Test get
    assert(set:get(idx1) == "hello")
    local t = set:get(idx2)
    assert(t.x == 100 and t.y == 200)
    assert(set:get(idx3) == 42)
    assert(set:get(99) == nil) -- not exists
    
    -- Test contains
    assert(set:contains(idx1) == true)
    assert(set:contains(idx2) == true)
    assert(set:contains(idx3) == true)
    assert(set:contains(99) == false)
    
    -- Test at (position-based access)
    local data0 = set:at(0)
    local data1 = set:at(1)
    local data2 = set:at(2)
    assert(data0 == "hello", "at(0) failed")
    assert(data1.x == 100 and data1.y == 200, "at(1) failed")
    assert(data2 == 42, "at(2) failed")
    assert(set:at(3) == nil) -- out of range
    
    -- Test indices
    local indices = set:indices()
    assert(#indices == 3)
    print("Indices:", table.concat(indices, ", "))
    
    -- Test iter
    local all_data = {}
    for _, id, data in set:iter() do
        all_data[id] = data
    end
    
    assert(all_data[idx1] == "hello", all_data[idx1])
    assert(all_data[idx2].x == 100)
    assert(all_data[idx3] == 42)
    
    -- Test remove
    print("Removing idx1:", idx1)
    assert(set:remove(idx1) == true)
    assert(set:contains(idx1) == false)
    assert(set:get(idx1) == nil)
    assert(set:size() == 2)
    assert(set:remove(idx1) == false) -- already removed
    
    -- Test add after remove - 应该复用idx1 (0)
    local idx4 = set:add("reused")
    print("After remove and add, new index:", idx4, "(should reuse 0)")
    assert(idx4 == 0, "Should reuse freed index 0")
    
    -- Add another one, should be 3
    local idx5 = set:add("new_item")
    assert(idx5 == 3, "Should get new index 3, got " .. tostring(idx5))
    
    -- Test clear
    set:clear()
    assert(set:size() == 0)
    assert(set:contains(idx2) == false)
    assert(set:get(idx2) == nil)
    
    print("All pure lua tests passed!")
end

test_sparse_set_pure()
