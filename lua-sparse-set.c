#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "sparse-set.h"

#define METATABLE_NAME "SparseSet"

// Lua绑定的sparse_set结构
typedef struct {
    sparse_set_t *set;  // 底层C的sparse set，只管理索引
    int ref_table;      // Lua注册表中存储数据的表引用
} lua_sparse_set_t;

static int l_create(lua_State *L) {
    uint32_t max_size = (uint32_t)luaL_checkinteger(L, 1);
    
    lua_sparse_set_t *lset = (lua_sparse_set_t *)lua_newuserdata(L, sizeof(lua_sparse_set_t));
    lset->set = sparse_set_create(max_size);
    
    if (!lset->set) {
        return luaL_error(L, "Failed to create sparse set");
    }

    // 创建一个表用于存储Lua数据引用
    lua_newtable(L);
    lset->ref_table = luaL_ref(L, LUA_REGISTRYINDEX);

    luaL_getmetatable(L, METATABLE_NAME);
    lua_setmetatable(L, -2);
    
    return 1;
}

static int l_add(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, METATABLE_NAME);
    // 第2个参数是要存储的数据(可以是任何Lua类型)
    
    // C自动分配索引
    uint32_t index = sparse_set_add(lset->set);
    
    if (index == UINT32_MAX) {
        lua_pushnil(L);  // 失败返回nil
        return 1;
    }
    
    // 将数据存储到引用表中
    lua_rawgeti(L, LUA_REGISTRYINDEX, lset->ref_table);  // 栈: ref_table
    lua_pushvalue(L, 2);  // 复制数据，栈: ref_table, data
    lua_rawseti(L, -2, index + 1);  // ref_table[index+1] = data (因为Lua数组1-based)
    lua_pop(L, 1); // 弹出ref_table
    
    // 返回分配的索引
    lua_pushinteger(L, index);
    return 1;
}

static int l_remove(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, METATABLE_NAME);
    uint32_t index = (uint32_t)luaL_checkinteger(L, 2);
    
    if (sparse_set_remove(lset->set, index)) {
        // 从引用表中移除数据
        lua_rawgeti(L, LUA_REGISTRYINDEX, lset->ref_table);
        lua_pushnil(L);
        lua_rawseti(L, -2, index + 1); // ref_table[index+1] = nil
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
    // The ref_table is kept in sync with the set.
    // If index is invalid or removed, ref_table[index+1] will be nil.
    
    // Get data from Lua table (using index+1 because Lua arrays are 1-based)
    lua_rawgeti(L, LUA_REGISTRYINDEX, lset->ref_table);
    lua_rawgeti(L, -1, index + 1); // get ref_table[index+1]
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
    
    // 清空引用表
    lua_rawgeti(L, LUA_REGISTRYINDEX, lset->ref_table);
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        lua_pop(L, 1); // 弹出value
        lua_pushvalue(L, -1); // 复制key
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
    // 释放引用表
    luaL_unref(L, LUA_REGISTRYINDEX, lset->ref_table);
    return 0;
}

// 迭代器函数：返回下一个(index, data)对
// Iterator function: returns next (pos, index, data) tuple
static int _iter(lua_State *L) {
    // Upvalue 1: ref_table (Lua table)
    // Argument 1: lset (lightuserdata)
    // Argument 2: pos (integer)
    
    lua_sparse_set_t *lset = (lua_sparse_set_t *)lua_touserdata(L, 1);
    int pos = lua_tointeger(L, 2);
    
    if (pos >= (int)sparse_set_size(lset->set)) {
        return 0;  // End of iteration
    }
    
    // Get sparse index for current dense position
    uint32_t index = sparse_set_get_index(lset->set, pos);
    
    // Return next position (control variable)
    lua_pushinteger(L, pos + 1);
    
    // Return index (iteration var 1)
    lua_pushinteger(L, index);
    
    // Get and return data (iteration var 2)
    // Using Upvalue 1 for ref_table avoids registry lookup
    lua_pushvalue(L, lua_upvalueindex(1)); 
    lua_rawgeti(L, -1, index + 1);  // get ref_table[index+1]
    lua_remove(L, -2);  // remove ref_table, leave data
    
    return 3;  // Returns: next_pos, index, data
}

// pairs iterator interface
static int l_iter(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, METATABLE_NAME);
    
    // Push ref_table to stack to use as upvalue
    lua_rawgeti(L, LUA_REGISTRYINDEX, lset->ref_table);
    
    // Push iterator function as closure with 1 upvalue (ref_table)
    lua_pushcclosure(L, _iter, 1);
    
    lua_pushlightuserdata(L, lset);
    lua_pushinteger(L, 0);
    
    return 3;
}

// 通过位置索引获取数据 (0-based)
static int l_at(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, METATABLE_NAME);
    uint32_t pos = (uint32_t)luaL_checkinteger(L, 2);
    
    if (pos >= sparse_set_size(lset->set)) {
        lua_pushnil(L);
        return 1;
    }
    
    uint32_t index = sparse_set_get_index(lset->set, pos);
    
    // 获取数据
    lua_rawgeti(L, LUA_REGISTRYINDEX, lset->ref_table);
    lua_rawgeti(L, -1, index + 1);
    return 1;
}

// 获取所有索引(keys)
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
