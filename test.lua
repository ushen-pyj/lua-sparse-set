local sparseset = require("sparseset")

local function test_sparse_set()
    print("Testing Sparse Set...")
    
    local set = sparseset.new(100, 10) -- max_val=100, capacity=10
    
    -- Test add
    assert(set:add(10) == true)
    assert(set:add(20) == true)
    assert(set:add(10) == false) -- already exists
    assert(set:size() == 2)
    
    -- Test contains
    assert(set:contains(10) == true)
    assert(set:contains(20) == true)
    assert(set:contains(30) == false)
    
    -- Test values
    local vals = set:values()
    assert(#vals == 2)
    assert((vals[1] == 10 and vals[2] == 20) or (vals[1] == 20 and vals[2] == 10))

    -- Test remove
    assert(set:remove(10) == true)
    assert(set:contains(10) == false)
    assert(set:size() == 1)
    assert(set:remove(10) == false) -- already removed
    
    -- Test capacity
    for i = 1, 9 do
        set:add(i)
    end
    assert(set:size() == 10)
    assert(set:add(100) == false) -- should be full (capacity is 10)
    
    -- Test clear
    set:clear()
    assert(set:size() == 0)
    assert(set:contains(20) == false)
    
    print("All tests passed!")
end

test_sparse_set()
