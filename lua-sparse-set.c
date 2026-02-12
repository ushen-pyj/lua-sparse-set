#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "sparse-set.h"

#define METATABLE_NAME "SparseSet"

// Lua绑定的sparse_set结构
typedef struct {
    sparse_set_t *set;  // 底层C的sparse set，只管理索引
    // ref_table removed; using UserValue 1 instead
} lua_sparse_set_t;

static int l_create(lua_State *L) {
    uint32_t max_size = (uint32_t)luaL_checkinteger(L, 1);
    
    // Create userdata with 1 user value
    // lua_newuserdatauv is available in Lua 5.4
    lua_sparse_set_t *lset = (lua_sparse_set_t *)lua_newuserdatauv(L, sizeof(lua_sparse_set_t), 1);
    lset->set = sparse_set_create(max_size);
    
    if (!lset->set) {
        return luaL_error(L, "Failed to create sparse set");
    }

    // Create a table for storage
    lua_newtable(L);
    // Set it as the first user value of the userdata (at stack -2)
    lua_setiuservalue(L, -2, 1);

    luaL_getmetatable(L, METATABLE_NAME);
    lua_setmetatable(L, -2);
    
    return 1;
}

static int l_add(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, METATABLE_NAME);
    // arg 2 is data
    
    // C allocates index
    uint32_t index = sparse_set_add(lset->set);
    
    if (index == UINT32_MAX) {
        lua_pushnil(L);
        return 1;
    }
    
    // Get storage table (UserValue 1)
    lua_getiuservalue(L, 1, 1);
    lua_pushvalue(L, 2); // push data
    lua_rawseti(L, -2, index + 1); // table[index+1] = data
    lua_pop(L, 1); // pop table
    
    // Return index
    lua_pushinteger(L, index);
    return 1;
}

static int l_remove(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, METATABLE_NAME);
    uint32_t index = (uint32_t)luaL_checkinteger(L, 2);
    
    if (sparse_set_remove(lset->set, index)) {
        // Remove from storage table
        lua_getiuservalue(L, 1, 1);
        lua_pushnil(L);
        lua_rawseti(L, -2, index + 1);
        lua_pop(L, 1);
        
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

static int l_contains(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, METATABLE_NAME);
    uint32_t index = (uint32_t)luaL_checkinteger(L, 2);
    lua_pushboolean(L, sparse_set_contains(lset->set, index));
    return 1;
}

static int l_get(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, METATABLE_NAME);
    uint32_t index = (uint32_t)luaL_checkinteger(L, 2);
    
    // Optimization: Skip sparse_set_contains check.
    // Sync is guaranteed.
    (void)lset; // Suppress unused variable warning

    
    lua_getiuservalue(L, 1, 1); // push storage table
    lua_rawgeti(L, -1, index + 1); // push value: stack: [ud, idx, table, value]
    
    // We want to return value.
    // We can cleanup stack or just return 1 (top value).
    // To be clean, replace -2 (table) with -1 (value)
    lua_replace(L, -2); // stack: [ud, idx, value]
    return 1;
}

static int l_size(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, METATABLE_NAME);
    lua_pushinteger(L, sparse_set_size(lset->set));
    return 1;
}

static int l_clear(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, METATABLE_NAME);
    sparse_set_clear(lset->set);
    
    // Clear storage table
    lua_getiuservalue(L, 1, 1);
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        lua_pop(L, 1); // pop value
        lua_pushvalue(L, -1); // copy key
        lua_pushnil(L);
        lua_rawset(L, -4); // table[key] = nil
    }
    lua_pop(L, 1);
    
    return 0;
}

static int l_gc(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, METATABLE_NAME);
    if (lset->set) {
        sparse_set_destroy(lset->set);
        lset->set = NULL;
    }
    // UserValue is collected automatically when userdata is collected.
    return 0;
}

// Iterator function: returns next (pos, index, data) tuple
// _iter removed; using _iter_optimized


static int _iter_optimized(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)lua_touserdata(L, 1);
    int pos = lua_tointeger(L, 2);
    
    if (pos >= (int)sparse_set_size(lset->set)) {
        return 0;
    }
    
    uint32_t index = sparse_set_get_index(lset->set, pos);
    
    lua_pushinteger(L, pos + 1);
    lua_pushinteger(L, index);
    
    // Directly access upvalue table
    lua_rawgeti(L, lua_upvalueindex(1), index + 1);
    
    return 3;
}

// pairs iterator interface
static int l_iter(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, METATABLE_NAME);
    
    // Push storage table as upvalue
    lua_getiuservalue(L, 1, 1);
    
    lua_pushcclosure(L, _iter_optimized, 1);
    
    lua_pushlightuserdata(L, lset);
    lua_pushinteger(L, 0);
    
    return 3;
}

static int l_at(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, METATABLE_NAME);
    uint32_t pos = (uint32_t)luaL_checkinteger(L, 2);
    
    if (pos >= sparse_set_size(lset->set)) {
        lua_pushnil(L);
        return 1;
    }
    
    uint32_t index = sparse_set_get_index(lset->set, pos);
    
    lua_getiuservalue(L, 1, 1);
    lua_rawgeti(L, -1, index + 1);
    lua_replace(L, -2);
    return 1;
}

static int l_indices(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, METATABLE_NAME);
    sparse_set_t *set = lset->set;
    
    lua_newtable(L);
    for (uint32_t i = 0; i < set->size; i++) {
        lua_pushinteger(L, sparse_set_get_index(set, i));
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static const struct luaL_Reg set_methods[] = {
    {"add", l_add},
    {"remove", l_remove},
    {"contains", l_contains},
    {"get", l_get},
    {"size", l_size},
    {"clear", l_clear},
    {"iter", l_iter},
    {"at", l_at},
    {"indices", l_indices},
    {"__gc", l_gc},
    {NULL, NULL}
};

int luaopen_sparseset(lua_State *L) {
    luaL_newmetatable(L, METATABLE_NAME);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, set_methods, 0);
    
    lua_newtable(L);
    lua_pushcfunction(L, l_create);
    lua_setfield(L, -2, "new");
    
    return 1;
}
