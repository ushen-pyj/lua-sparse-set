package.cpath = package.cpath .. ";./build/?.so"

local sparse_set = require("sparseset")

local test_insert = function()
    local size = 100
    local reg = sparse_set.new_registry(size)
    local set = sparse_set.new_set(size)
    
    local id1 = reg:create()
    local id2 = reg:create()
    assert(id1 == 0, "id1 should be 1 now = " .. id1)
    assert(id2 == 1, "id2 should be 2 now = " .. id2)
    set:insert(id1, "hello")
    set:insert(id2, {hello = 1})

    for _, id, data in set:iter() do
        if id == id1 then
            assert(data == "hello", "data should be 'hello'")
        elseif id == id2 then
            assert(data and data.hello == 1, "data should be {hello = 1}")
        else
            assert(false, "id should be id1 or id2")
        end
    end

    assert(set:get(id1) == "hello", "set:get(id1) should be 'hello'")
    assert(set:get(id2) and set:get(id2).hello == 1, "set:get(id2) should be {hello = 1}")
    set:remove(id1)
    set:remove(id2)
    assert(set:get(id1) == nil, "set:get(id1) should be nil")
    assert(set:get(id2) == nil, "set:get(id2) should be nil")
    reg:destroy(id1)
    reg:destroy(id2)
    assert(reg:valid(id1) == false, "reg:valid(id1) should be false")
    assert(reg:valid(id2) == false, "reg:valid(id2) should be false")
    print("test_insert passed")
end

local test_id_pool = function()
    local size = 100
    local reg = sparse_set.new_registry(size)
    local set = sparse_set.new_set(size)
    
    local id1 = reg:create()
    local id2 = reg:create()
    local id3 = reg:create()
    local id4 = reg:create()
    set:insert(id1, "hello")
    set:insert(id2, {hello = 1})
    set:insert(id3, "world")
    set:insert(id4, {world = 2})
    
    reg:destroy(id3)
    set:remove(id3)

    local id5 = reg:create()
    assert(id5 == ((1 << 32) | 2), "id5 should be " .. ((1 << 32) | 2))
    set:insert(id5, "hello")
    print("test_id_pool passed")
end

test_insert()
test_id_pool()
