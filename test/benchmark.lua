
package.cpath = package.cpath .. ";./build/?.so"
local sparse_set = require("sparseset")
local size = 1000000
local hash_key = {}
for i = 1, size do
    hash_key[i] = "hash_key_" .. i
end

local function benchmark(name, fn_insert, fn_read, fn_iter, fn_remove)
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

    -- Remove
    start = os.clock()
    fn_remove()
    results.remove = os.clock() - start
    
    return results
end

local arr = {}
local res_array = benchmark("Lua Array", 
    function()
        local tinsert = table.insert
        for i, v in ipairs(hash_key) do
            tinsert(arr, v)
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
    end,
    function()
        local tremove = table.remove
        for i = 1, size do
            tremove(arr)
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
    end,
    function()
        for _, v in ipairs(hash_key) do
            hash[v] = nil
        end
    end
)

local reg = sparse_set.new_registry(size)
local set = sparse_set.new_set()
local entities = {}
local create = reg.create
local insert = set.insert
local res_sparse = benchmark("Sparse Set",
    function()
        for i = 1, size do
            local e = create(reg)
            entities[i] = e
            insert(set, e, i)
        end
    end,
    function()
        local get = set.get
        for i = 1, size do
            s = get(set, entities[i])
        end
    end,
    function()
        for i, v in set:iter() do
        end
    end,
    function()
        local remove = set.remove
        for i = 1, size do
            remove(set, entities[i])
        end
    end
)

local reg_c = sparse_set.new_registry(size)
-- Stride 8 bytes (e.g. 2 ints)
local set_c = sparse_set.new_set(8)
local entities_c = {}
local create_c = reg_c.create
local insert_c = set_c.insert
local packed_data = string.pack("ii", 123, 456)
local unpack = string.unpack

local res_c_comp = benchmark("C Component",
    function()
        for i = 1, size do
            local e = create_c(reg_c)
            entities_c[i] = e
            insert_c(set_c, e, packed_data)
        end
    end,
    function()
        local get = set_c.get
        for i = 1, size do
            local raw = get(set_c, entities_c[i])
            local a, b = unpack("ii", raw)
        end
    end,
    function()
        for i, v in set_c:iter() do
        end
    end,
    function()
        local remove = set_c.remove
        for i = 1, size do
            remove(set_c, entities_c[i])
        end
    end
)

-- Benchmark for get_field/set_field specifically
local reg_f = sparse_set.new_registry(size)
local set_f = sparse_set.new_set(8)
local entities_f = {}
for i = 1, size do
    local e = reg_f:create()
    entities_f[i] = e
    set_f:insert(e, packed_data)
end
local get_field = set_f.get_field
local set_field = set_f.set_field

local c_type = {
    INT = 1,
    FLOAT = 2,
    DOUBLE = 3,
    BYTE = 4,
    BOOL = 5,
}

local res_field_access = benchmark("C Field Access",
    function()
         local type_int = c_type.INT
         for i = 1, size do
            set_field(set_f, entities_f[i], 0, type_int, i)
        end
    end,
    function()
        local type_int = c_type.INT
        for i = 1, size do
            local v = get_field(set_f, entities_f[i], 0, type_int)
        end
    end,
    function() end, -- skip iter
    function() end  -- skip remove
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
        print(format_row("", "Remove", res.remove))
        print(divider)
    end
end

print_table({res_array, res_hash, res_sparse, res_c_comp, res_field_access})