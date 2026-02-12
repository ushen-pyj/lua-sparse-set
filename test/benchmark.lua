
-- test/benchmark.lua
-- Performance benchmark for Lua Array, Hash Table, and Sparse Set

-- Add build folder to cpath
package.cpath = package.cpath .. ";./build/?.so"

local sparseset
local ok, lib = pcall(require, "sparseset")
if not ok then
    print("Error loading sparseset module: " .. lib)
    print("Make sure you are running from the project root and the module is built.")
    os.exit(1)
end
sparseset = lib

local COUNT = 1000000 -- 1 million items
-- Adjust COUNT if it's too slow on the target machine
-- COUNT = 100000

local TEST_DATA = {x = 100, y = 200}
local os_clock = os.clock

local function print_header(title)
    print("\n" .. string.rep("=", 60))
    print(string.format("BENCHMARK: %s (%d items)", title, COUNT))
    print(string.rep("=", 60))
    print(string.format("%-20s %-20s %-15s", "Operation", "Time (sec)", "Ops/sec"))
    print(string.rep("-", 60))
end

local function print_result(op, time)
    local ops = COUNT / time
    local ops_str
    if ops > 1000000 then
        ops_str = string.format("%.2f M", ops / 1000000)
    else
        ops_str = string.format("%.2f K", ops / 1000)
    end
    print(string.format("%-20s %-20.4f %s/s", op, time, ops_str))
end

local function run_benchmark()
    print("Preparing test data...")
    -- Pre-generate string keys for Hash test to measure table performance only
    local hash_keys = {}
    for i = 1, COUNT do
        hash_keys[i] = "key_" .. i
    end
    
    -- Pre-allocate arrays for collecting results to minimize table resize overhead during read tests
    local array_t = {} 
    -- For pre-allocation simulation (optional in Lua, but we usually start empty)

    -------------------------------------------------------
    -- 1. Lua Array (Sequential Integer Keys)
    -------------------------------------------------------
    print_header("Lua Standard Array (Dense)")
    
    -- INSERT
    local t1 = os_clock()
    for i = 1, COUNT do
        array_t[i] = TEST_DATA
    end
    local d1 = os_clock() - t1
    print_result("Insert", d1)
    
    -- READ
    local t2 = os_clock()
    local dummy = 0
    for i = 1, COUNT do
        local v = array_t[i]
        if v then dummy = dummy + 1 end
    end
    local d2 = os_clock() - t2
    print_result("Read (Random/Seq)", d2)
    
    -- ITERATION (ipairs)
    local t3 = os_clock()
    dummy = 0
    for i, v in ipairs(array_t) do
        dummy = dummy + 1
    end
    local d3 = os_clock() - t3
    print_result("Iterate (ipairs)", d3)

    -- CLEANUP
    array_t = nil
    collectgarbage()

    -------------------------------------------------------
    -- 2. Lua Hash Table (String Keys)
    -------------------------------------------------------
    print_header("Lua Hash Table (String Keys)")
    
    local hash_t = {}
    
    -- INSERT
    t1 = os_clock()
    for _, k in ipairs(hash_keys) do
        hash_t[k] = TEST_DATA
    end
    d1 = os_clock() - t1
    print_result("Insert", d1)
    
    -- READ
    t2 = os_clock()
    dummy = 0
    for _, k in ipairs(hash_keys) do
        local v = hash_t[k]
        if v then dummy = dummy + 1 end
    end
    d2 = os_clock() - t2
    print_result("Read (Key lookup)", d2)
    
    -- ITERATION (pairs)
    t3 = os_clock()
    dummy = 0
    for k, v in pairs(hash_t) do
        dummy = dummy + 1
    end
    d3 = os_clock() - t3
    print_result("Iterate (pairs)", d3)

    -- CLEANUP
    hash_t = nil
    collectgarbage()

    -------------------------------------------------------
    -- 3. Sparse Set (C Extension)
    -------------------------------------------------------
    print_header("Sparse Set (C Extension)")
    
    local set = sparseset.new(COUNT)
    local indices = {}
    
    -- INSERT
    -- Note: We store indices to be able to read back.
    -- The overhead of 'indices[i] = ...' is included, which is fair as you usually need to keep the handle.
    t1 = os_clock()
    for i in ipairs(hash_keys) do
        indices[i] = set:add(TEST_DATA)
    end
    d1 = os_clock() - t1
    print_result("Insert (+store idx)", d1)
    
    -- READ
    t2 = os_clock()
    dummy = 0
    for i in ipairs(hash_keys) do
        local v = set:get(indices[i])
        if v then dummy = dummy + 1 end
    end
    d2 = os_clock() - t2
    print_result("Read (by index)", d2)
    
    -- ITERATION
    t3 = os_clock()
    dummy = 0
    for _, idx, val in set:iter() do
        dummy = dummy + 1
    end
    d3 = os_clock() - t3
    print_result("Iterate", d3)
    
    -- CLEANUP
    set:clear() -- Explicit clear testing (optional)
    set = nil
    indices = nil
    collectgarbage()

    -------------------------------------------------------
    -- 4. Sparse Set (Pure Lua)
    -------------------------------------------------------
    print_header("Sparse Set (Pure Lua)")
    
    local lua_sparseset = require("lua-sparse-set")
    local set = lua_sparseset.new(COUNT)
    local indices = {}
    
    -- INSERT
    t1 = os_clock()
    for i in ipairs(hash_keys) do
        indices[i] = set:add(TEST_DATA)
    end
    d1 = os_clock() - t1
    print_result("Insert (+store idx)", d1)
    
    -- READ
    t2 = os_clock()
    dummy = 0
    for i in ipairs(hash_keys) do
        local v = set:get(indices[i])
        if v then dummy = dummy + 1 end
    end
    d2 = os_clock() - t2
    print_result("Read (by index)", d2)
    
    -- ITERATION
    t3 = os_clock()
    dummy = 0
    for _, idx, val in set:iter() do
        dummy = dummy + 1
    end
    d3 = os_clock() - t3
    print_result("Iterate", d3)
    
    -- CLEANUP
    set = nil
    indices = nil
    collectgarbage()

    print("\nBenchmark completed.\n")
end

run_benchmark()
