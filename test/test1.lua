-- Add build folder to cpath for loading .so files
package.cpath = package.cpath .. ";./build/?.so"

local sparseset = require("sparseset")

local function test_sparse_set()
    print("Testing Sparse Set with Lua Data...")
    
    local set = sparseset.new(10) -- max_size=10
    
    -- Test add with different data types (C自动分配index)
    local idx1 = set:add("hello")
    local idx2 = set:add({x = 100, y = 200})
    local idx3 = set:add(42)
    assert(idx1 == 0)
    assert(idx2 == 1)
    assert(idx3 == 2)
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
    -- Test at (position-based access)
    local data0 = set:at(0)
    local data1 = set:at(1)
    local data2 = set:at(2)
    assert(data0 == "hello")
    assert(data1.x == 100 and data1.y == 200)
    assert(data2 == 42)
    assert(set:at(3) == nil) -- out of range
    
    -- Test indices
    local indices = set:indices()
    assert(#indices == 3)
    print("Indices:", table.concat(indices, ", "))
    
    -- Test iter
    local all_data = {}
    for _, index, data in set:iter() do
        all_data[index] = data
    end
    
    assert(all_data[idx1] == "hello", all_data[idx1])
    assert(all_data[idx2].x == 100)
    assert(all_data[idx3] == 42)
    
    -- Test remove
    assert(set:remove(idx1) == true)
    assert(set:contains(idx1) == false)
    assert(set:get(idx1) == nil)
    assert(set:size() == 2)
    assert(set:remove(idx1) == false) -- already removed
    
    -- Test add after remove - 应该复用idx1
    local idx4 = set:add("reused")
    print("After remove and add, new index:", idx4, "(should try to reuse)", idx1)
    assert(idx4 == idx1, "Should reuse freed index")
    local idx5 = set:add("reused2")
    assert(idx5 == 3, "Should get new index " .. idx5 )
    
    -- Test add more complex data
    local idx5 = set:add(function() return "closure" end)
    local fn = set:get(idx5)
    assert(fn() == "closure")
    
    -- Test capacity
    for i = 1, 5 do
        local idx = set:add("item" .. i)
        if idx == nil then
            print("Capacity full at item", i)
            break
        end
    end
    assert(set:size() <= 10)
    
    -- 当容量满时，应该返回nil
    if set:size() == 10 then
        local overflow_idx = set:add("overflow")
        assert(overflow_idx == nil, "Should return nil when full")
    end
    
    -- Test clear
    set:clear()
    assert(set:size() == 0)
    assert(set:contains(idx2) == false)
    assert(set:get(idx2) == nil)
    
    print("All test.lua tests passed!")
end

return {
    test_sparse_set = test_sparse_set
}
