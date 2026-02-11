-- Add build folder to cpath for loading .so files
package.cpath = package.cpath .. ";./build/?.so"

local sparseset = require("sparseset")

local function test_sparse_set()
    print("Testing Sparse Set with Lua Data...")
    
    local set = sparseset.new(100, 10) -- max_val=100, capacity=10
    
    -- Test add with different data types
    assert(set:add(10, "hello") == true)
    assert(set:add(20, {x = 100, y = 200}) == true)
    assert(set:add(30, 42) == true)
    assert(set:add(10, "world") == false) -- already exists
    assert(set:size() == 3)
    
    -- Test get
    assert(set:get(10) == "hello")
    local t = set:get(20)
    assert(t.x == 100 and t.y == 200)
    assert(set:get(30) == 42)
    assert(set:get(99) == nil) -- not exists
    
    -- Test contains
    assert(set:contains(10) == true)
    assert(set:contains(20) == true)
    assert(set:contains(30) == true)
    assert(set:contains(99) == false)
    
    -- Test at (position-based access)
    local data0 = set:at(0)
    local data1 = set:at(1)
    local data2 = set:at(2)
    assert(data0 ~= nil)
    assert(data1 ~= nil)
    assert(data2 ~= nil)
    assert(set:at(3) == nil) -- out of range
    
    -- Test indices
    local indices = set:indices()
    assert(#indices == 3)
    print("Indices:", table.concat(indices, ", "))
    
    -- Test pairs
    local all_data = set:pairs()
    assert(all_data[10] == "hello")
    assert(all_data[20].x == 100)
    assert(all_data[30] == 42)
    
    -- Test remove
    assert(set:remove(10) == true)
    assert(set:contains(10) == false)
    assert(set:get(10) == nil)
    assert(set:size() == 2)
    assert(set:remove(10) == false) -- already removed
    
    -- Test add more complex data
    set:add(5, function() return "closure" end)
    local fn = set:get(5)
    assert(fn() == "closure")
    
    -- Test capacity
    for i = 1, 7 do
        set:add(40 + i, "item" .. i)
    end
    assert(set:size() == 10)
    assert(set:add(100, "overflow") == false) -- should be full (capacity is 10)
    
    -- Test clear
    set:clear()
    assert(set:size() == 0)
    assert(set:contains(20) == false)
    assert(set:get(20) == nil)
    
    print("All tests passed!")
end

test_sparse_set()
