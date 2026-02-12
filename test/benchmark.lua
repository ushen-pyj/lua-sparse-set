
package.cpath = package.cpath .. ";./build/?.so"
local sparse_set = require("sparseset")
local size = 1000000
local hash_key = {}
for i = 1, size do
    hash_key[i] = "hash_key_" .. i
end

local function benchmark(name, fn_insert, fn_read, fn_iter)
    local results = { name = name }
    
    -- Insert
    local start = os.clock()
    fn_insert()
    results.insert = os.clock() - start
    
    -- Read
    start = os.clock()
    fn_read()
    results.read = os.clock() - start
    
    -- Iter
    start = os.clock()
    fn_iter()
    results.iter = os.clock() - start
    
    return results
end

local arr = {}
local res_array = benchmark("Lua Array", 
    function()
        for i, v in ipairs(hash_key) do
            table.insert(arr, v)
        end
    end,
    function()
        local s
        for i = 1, size do
            s = arr[i]
        end
    end,
    function()
        for i, v in ipairs(arr) do
        end
    end
)

local hash = {}
local res_hash = benchmark("Lua Hash",
    function()
        for i, v in ipairs(hash_key) do
            hash[v] = i
        end
    end,
    function()
        local s
        for _, v in ipairs(hash_key) do
            s = hash[v]
        end
    end,
    function()
        for i, v in pairs(hash) do
        end
    end
)

local reg = sparse_set.new_registry(size)
local set = sparse_set.new_set(size)
local entities = {}
local res_sparse = benchmark("Sparse Set",
    function()
        for i = 1, size do
            local e = reg:create()
            entities[i] = e
            set:insert(e, i)
        end
    end,
    function()
        local s
        for i = 1, size do
            s = set:get(entities[i])
        end
    end,
    function()
        for i, v in set:iter() do
        end
    end
)

local function print_table(all_results)
    local header = string.format("%-15s | %-10s | %-10s | %-15s | %-15s", 
        "Test Case", "Op", "Time(s)", "Throughput", "Avg(ns/op)")
    local divider = string.rep("-", #header)
    
    print("\n" .. divider)
    print(header)
    print(divider)
    
    local function format_row(name, op, time)
        local throughput = size / time
        local avg = (time / size) * 1e9
        
        local throughput_str
        if throughput > 1e6 then
            throughput_str = string.format("%.2f M/s", throughput / 1e6)
        elseif throughput > 1e3 then
            throughput_str = string.format("%.2f K/s", throughput / 1e3)
        else
            throughput_str = string.format("%.2f /s", throughput)
        end
        
        return string.format("%-15s | %-10s | %-10.4f | %-15s | %-15.2f", 
            name, op, time, throughput_str, avg)
    end
    
    for _, res in ipairs(all_results) do
        print(format_row(res.name, "Insert", res.insert))
        print(format_row("", "Read", res.read))
        print(format_row("", "Iter", res.iter))
        print(divider)
    end
end

print_table({res_array, res_hash, res_sparse})