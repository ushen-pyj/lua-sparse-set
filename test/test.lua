package.cpath = package.cpath .. ";./build/?.so;../build/?.so"

local sparse_set = require("sparseset")

local function assert_eq(a, b, msg)
    if a ~= b then
        error(string.format("%s: expected %s, got %s", msg or "Assertion failed", tostring(b), tostring(a)))
    end
end

local function assert_true(a, msg)
    if not a then
        error(string.format("%s: expected true, got %s", msg or "Assertion failed", tostring(a)))
    end
end

local function assert_false(a, msg)
    if a then
        error(string.format("%s: expected false, got %s", msg or "Assertion failed", tostring(a)))
    end
end

local function test_registry()
    print("Testing Registry...")
    local reg = sparse_set.new_registry()
    
    local id1 = reg:create()
    assert_true(id1, "Should create id")
    assert_true(reg:valid(id1), "ID should be valid")
    
    local id2 = reg:create()
    assert_true(id2, "Should create second id")
    assert_true(id1 ~= id2, "IDs should be different")
    
    reg:destroy(id1)
    assert_false(reg:valid(id1), "ID1 should be invalid after destroy")
    assert_true(reg:valid(id2), "ID2 should still be valid")
    
    -- Recursion/Reuse check
    local id3 = reg:create()
    -- id3 should reuse index of id1 but with different version
    -- This specific check depends on implementation details, but we can check it's valid
    assert_true(reg:valid(id3), "Recycled ID should be valid")
    assert_true(id3 ~= id1, "Recycled ID should differ from original (versioning)")
    
    print("Registry tests passed.")
end

local function test_lua_set()
    print("Testing Lua Set (Stride 0)...")
    local reg = sparse_set.new_registry()
    local set = sparse_set.new_set() -- Stride 0 by default
    
    local id1 = reg:create()
    local id2 = reg:create()
    local id3 = reg:create()
    
    -- Insert
    set:insert(id1, "Data1")
    set:insert(id2, { key = "Data2" })
    
    assert_eq(set:size(), 2, "Size should be 2")
    assert_true(set:contains(id1), "Should contain id1")
    assert_true(set:contains(id2), "Should contain id2")
    assert_false(set:contains(id3), "Should not contain id3")
    
    -- Get
    assert_eq(set:get(id1), "Data1", "Get id1 data incorrect")
    local d2 = set:get(id2)
    assert_eq(type(d2), "table", "Get id2 data type incorrect")
    assert_eq(d2.key, "Data2", "Get id2 data content incorrect")
    
    -- Index Of
    -- Index Of
    local idx1 = set:index_of(id1)
    local idx2 = set:index_of(id2)
    assert_eq(idx1, 1, "id1 should be at index 1")
    assert_eq(idx2, 2, "id2 should be at index 2")
    assert_eq(set:index_of(id3), nil, "id3 should not have index")
    
    -- At (Bug regression test)
    local at_id1, at_data1 = set:at(idx1)
    
    -- Debug print if nil
    if at_id1 == nil then print("set:at(idx1) returned nil ID!") end
    
    assert_eq(at_id1, id1, "at(idx1) should return id1")
    assert_eq(at_data1, "Data1", "at(idx1) should return Data1")
    
    local at_id2, at_data2 = set:at(idx2)
    assert_eq(at_id2, id2, "at(idx2) should return id2")
    assert_eq(at_data2.key, "Data2", "at(idx2) should return Data2")
    
    -- Swap
    -- Assuming idx1=1, idx2=2 or vice versa. 
    set:swap(idx1, idx2)
    
    -- After swap, indices should point to different data? 
    -- No, 'idx1' is a number (e.g., 1). 'idx2' is a number (e.g., 2).
    -- swap(1, 2) means the entity at pos 1 moves to pos 2, and entity at pos 2 moves to pos 1.
    -- internal mappings verify:
    local new_idx1 = set:index_of(id1)
    local new_idx2 = set:index_of(id2)
    
    assert_eq(new_idx1, idx2, "id1 should be at old idx2")
    assert_eq(new_idx2, idx1, "id2 should be at old idx1")
    
    -- Verify data moved with ID
    local swap_id1, swap_data1 = set:at(new_idx1)
    assert_eq(swap_id1, id1, "Moved id1 check")
    assert_eq(swap_data1, "Data1", "Moved id1 data check")

    -- Iter
    local count = 0
    for i, id, data in set:iter() do
        count = count + 1
        assert_true(id == id1 or id == id2, "Iter ID matches")
    end
    assert_eq(count, 2, "Iter count incorrect")
    
    -- Remove
    set:remove(id1)
    assert_eq(set:size(), 1, "Size after remove incorrect")
    assert_false(set:contains(id1), "Should not contain id1")
    assert_true(set:contains(id2), "Should still contain id2")
    assert_eq(set:get(id1), nil, "Get removed ID should return nil")
    
    print("Lua Set tests passed.")
end

local function test_c_set()
    print("Testing C Set (Stride > 0)...")
    local reg = sparse_set.new_registry()
    -- Stride = 8 bytes (e.g., 2 ints)
    local stride = 8
    local set = sparse_set.new_set(stride)
    
    local id1 = reg:create()
    local id2 = reg:create()
    
    -- Insert
    -- Pack 2 ints: 10, 20
    local data1 = string.pack("ii", 10, 20)
    set:insert(id1, data1)
    
    -- Pack 2 ints: 30, 40
    local data2 = string.pack("ii", 30, 40)
    set:insert(id2, data2)
    
    assert_eq(set:size(), 2, "C Set size incorrect")
    
    -- Get (Raw)
    local raw1 = set:get(id1)
    local v1, v2 = string.unpack("ii", raw1)
    assert_eq(v1, 10, "Get raw data1 mismatch")
    assert_eq(v2, 20, "Get raw data2 mismatch")
    
    -- Set Field / Get Field
    -- offset 0, type INT (1)
    local TYPE_INT = 1
    local TYPE_BYTE = 4 -- Just guessing types from previous context. 
    -- wait, let's verify types from C code:
    -- #define TYPE_INT 1
    -- #define TYPE_FLOAT 2
    -- #define TYPE_DOUBLE 3
    -- #define TYPE_BYTE 4
    -- #define TYPE_BOOL 5
    
    -- Read back 10 as float? No, stick to int.
    local f1 = set:get_field(id1, 0, TYPE_INT)
    assert_eq(f1, 10, "get_field int mismatch")
    
    -- Modify second int (offset 4)
    set:set_field(id1, 4, TYPE_INT, 99)
    local f2 = set:get_field(id1, 4, TYPE_INT)
    assert_eq(f2, 99, "set_field verification failed")
    
    -- Swap
    local idx1 = set:index_of(id1)
    local idx2 = set:index_of(id2)
    
    set:swap(idx1, idx2)
    
    -- Verify IDs swapped
    assert_eq(set:index_of(id1), idx2, "Swap C Set: ID location mismatch")
    
    -- Verify Data moved (C memory copy check)
    -- id1 is now at idx2. Let's read from idx2
    local at_id_new_pos, at_data_raw = set:at(idx2)
    assert_eq(at_id_new_pos, id1, "At after swap ID mismatch")
    local u1, u2 = string.unpack("ii", at_data_raw)
    assert_eq(u1, 10, "Data move mismatch 1")
    assert_eq(u2, 99, "Data move mismatch 2") -- was modified to 99
    
    -- Remove
    set:remove(id1)
    assert_false(set:contains(id1), "Remove failed")
    assert_eq(set:size(), 1, "Size after remove failed")
    
    -- Remaining data check (id2)
    local rem_raw = set:get(id2)
    local r1, r2 = string.unpack("ii", rem_raw)
    assert_eq(r1, 30, "Remaining data integrity check 1")
    assert_eq(r2, 40, "Remaining data integrity check 2")
    
    -- Iter Check for C Set
    local count = 0
    for i, id, data in set:iter() do
        count = count + 1
        assert_true(type(data) == "string", "Iter data should be string (binary pack)")
        assert_eq(#data, stride, "Iter data length mismatch")
    end
    assert_eq(count, 1, "Iter count should be 1 after removal")

    print("C Set tests passed.")
end

local function run_tests()
    test_registry()
    print("--------------------------------")
    test_lua_set()
    print("--------------------------------")
    test_c_set()
    print("--------------------------------")
    print("ALL TESTS PASSED")
end

run_tests()
