#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "sparse_set.h"

#define METATABLE_NAME "SparseSet"

static int l_create(lua_State *L) {
    uint32_t max_val = (uint32_t)luaL_checkinteger(L, 1);
    uint32_t capacity = (uint32_t)luaL_checkinteger(L, 2);
    
    sparse_set_t **set_ptr = (sparse_set_t **)lua_newuserdata(L, sizeof(sparse_set_t *));
    *set_ptr = sparse_set_create(max_val, capacity);
    
    if (!*set_ptr) {
        return luaL_error(L, "Failed to create sparse set");
    }

    luaL_getmetatable(L, METATABLE_NAME);
    lua_setmetatable(L, -2);
    
    return 1;
}

static int l_add(lua_State *L) {
    sparse_set_t **set_ptr = (sparse_set_t **)luaL_checkudata(L, 1, METATABLE_NAME);
    uint32_t val = (uint32_t)luaL_checkinteger(L, 2);
    lua_pushboolean(L, sparse_set_add(*set_ptr, val));
    return 1;
}

static int l_remove(lua_State *L) {
    sparse_set_t **set_ptr = (sparse_set_t **)luaL_checkudata(L, 1, METATABLE_NAME);
    uint32_t val = (uint32_t)luaL_checkinteger(L, 2);
    lua_pushboolean(L, sparse_set_remove(*set_ptr, val));
    return 1;
}

static int l_contains(lua_State *L) {
    sparse_set_t **set_ptr = (sparse_set_t **)luaL_checkudata(L, 1, METATABLE_NAME);
    uint32_t val = (uint32_t)luaL_checkinteger(L, 2);
    lua_pushboolean(L, sparse_set_contains(*set_ptr, val));
    return 1;
}

static int l_size(lua_State *L) {
    sparse_set_t **set_ptr = (sparse_set_t **)luaL_checkudata(L, 1, METATABLE_NAME);
    lua_pushinteger(L, sparse_set_size(*set_ptr));
    return 1;
}

static int l_clear(lua_State *L) {
    sparse_set_t **set_ptr = (sparse_set_t **)luaL_checkudata(L, 1, METATABLE_NAME);
    sparse_set_clear(*set_ptr);
    return 0;
}

static int l_gc(lua_State *L) {
    sparse_set_t **set_ptr = (sparse_set_t **)luaL_checkudata(L, 1, METATABLE_NAME);
    if (*set_ptr) {
        sparse_set_destroy(*set_ptr);
        *set_ptr = NULL;
    }
    return 0;
}

static int l_values(lua_State *L) {
    sparse_set_t **set_ptr = (sparse_set_t **)luaL_checkudata(L, 1, METATABLE_NAME);
    sparse_set_t *set = *set_ptr;
    lua_newtable(L);
    for (uint32_t i = 0; i < set->size; i++) {
        lua_pushinteger(L, set->dense[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static const struct luaL_Reg set_methods[] = {
    {"add", l_add},
    {"remove", l_remove},
    {"contains", l_contains},
    {"size", l_size},
    {"clear", l_clear},
    {"values", l_values},
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
