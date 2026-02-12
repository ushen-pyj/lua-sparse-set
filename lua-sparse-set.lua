-- lua_sparse_set.lua
-- Pure Lua implementation of Sparse Set

local SparseSet = {}
SparseSet.__index = SparseSet

function SparseSet.new(max_size) -- max_size is optional, mimicking the C API structure
    local self = setmetatable({}, SparseSet)
    self.dense = {}   -- Stores the actual data (or keys if you just want a set of IDs)
    self.sparse = {}  -- Maps external ID -> dense index
    self.n = 0        -- Current number of elements
    -- In a pure Lua "Set" of arbitrary data, we usually need to 
    -- generate an ID for each added item to mimic the C behavior 
    -- (where add() returns an ID).
    
    -- However, to match the C implementation exactly:
    -- The C implem returned an auto-incrementing/recycled integer ID.
    -- Let's mimic that.
    self.data = {}      -- Stores the actual Lua values: data[dense_index] = value
    self.reverse = {}   -- dense[dense_index] = external_id (needed for removal/swap)
    
    -- We need a way to allocate IDs. 
    -- A simple counter is enough if we don't recycle, but C implem recycled.
    -- For simplicity and speed in Lua, let's just use a counter for now, 
    -- or if we want to be exact, we need a free list.
    -- But wait, the C implementation's "dense" array actually stored the *External IDs*.
    -- And "Data" was stored in a separate Lua registry table keyed by External ID.
    
    -- Let's replicate the structure for fairness:
    -- 1. dense array: stores External IDs.
    -- 2. sparse array: stores index in dense array for a given External ID.
    -- 3. data store: stores the actual values keyed by External ID.
    
    -- To support ID recycling like the C version:
    -- The C version initialized dense with 0..max_size.
    -- Let's do a dynamic approach for Lua to avoid pre-allocation overhead.
    self.max_idx = 0 -- The highest ID ever issued
    self.free_ids = {} -- Stack of recycled IDs
    
    return self
end

function SparseSet:add(value)
    local id
    if #self.free_ids > 0 then
        id = table.remove(self.free_ids)
    else
        id = self.max_idx
        self.max_idx = self.max_idx + 1
    end
    
    -- Add to sparse set structure
    local dense_idx = self.n + 1 -- Lua 1-based
    self.n = dense_idx
    
    self.dense[dense_idx] = id
    self.sparse[id] = dense_idx
    self.data[id] = value
    
    return id
end

function SparseSet:remove(id)
    if not self.sparse[id] then return false end
    
    local dense_idx = self.sparse[id]
    local last_idx = self.n
    local last_id = self.dense[last_idx]
    
    -- Swap with last element
    self.dense[dense_idx] = last_id
    self.sparse[last_id] = dense_idx
    
    -- Remove the item
    self.sparse[id] = nil
    self.dense[last_idx] = nil
    self.data[id] = nil
    self.n = self.n - 1
    
    -- Recycle ID
    table.insert(self.free_ids, id)
    
    return true
end

function SparseSet:contains(id)
    return self.sparse[id] ~= nil
end

function SparseSet:get(id)
    return self.data[id]
end

function SparseSet:size()
    return self.n
end

function SparseSet:clear()
    self.dense = {}
    self.sparse = {}
    self.data = {}
    self.n = 0
    self.max_idx = 0
    self.free_ids = {}
end

-- Iterator
-- Optimized: Returns a closure that upvalues the necessary tables for faster access
function SparseSet:iter()
    local dense = self.dense
    local data = self.data
    local n = self.n
    local idx = 0
    
    return function()
        idx = idx + 1
        if idx > n then return nil end
        local id = dense[idx]
        return idx, id, data[id]
    end
end

-- Access by dense index (0-based in C, let's keep 0-based API for consistency or 1-based for Lua?)
-- The C API `at(pos)` was 0-based.
-- If we want to drop-in replace, strictly speaking `at(0)` should return first element.
function SparseSet:at(pos) 
    -- pos is 0-based index in dense array
    local idx = pos + 1
    if idx > self.n then return nil end
    local id = self.dense[idx]
    return self.data[id]
end

-- Indices
function SparseSet:indices()
    local res = {}
    for i = 1, self.n do
        res[i] = self.dense[i]
    end
    return res
end

return SparseSet
