#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "sparse-set.h"

#define REGISTRY_METATABLE "SparseRegistry"
#define SET_METATABLE "SparseSet"

static int l_reg_create(lua_State *L) {
    registry_t *reg = (registry_t *)lua_newuserdatauv(L, sizeof(registry_t), 0);
    if (!registry_init(reg)) return luaL_error(L, "Failed to create registry");

    luaL_getmetatable(L, REGISTRY_METATABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_reg_create_id(lua_State *L) {
    registry_t *reg = get_reg(L);
    sparse_set_id_t id = registry_create_id(reg);
    if (id == ID_NULL) lua_pushnil(L);
    else lua_pushinteger(L, id);
    return 1;
}

static int l_reg_destroy_id(lua_State *L) {
    registry_t *reg = get_reg(L);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    registry_recycle(reg, id);
    return 0;
}

static int l_reg_valid(lua_State *L) {
    registry_t *reg = get_reg(L);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    lua_pushboolean(L, registry_valid(reg, id));
    return 1;
}

static int l_reg_gc(lua_State *L) {
    registry_t *reg = get_reg(L);
    registry_deinit(reg);
    return 0;
}

static int l_set_create(lua_State *L) {
    sparse_set_t *set = (sparse_set_t *)lua_newuserdatauv(L, sizeof(sparse_set_t), 1);
    if (!sparse_set_init(set)) return luaL_error(L, "Failed to create set");
    
    lua_newtable(L);
    lua_setiuservalue(L, -2, 1);

    luaL_getmetatable(L, SET_METATABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_set_insert(lua_State *L) {
    sparse_set_t *set = get_set(L);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    
    uint32_t pos = sparse_set_insert(set, id);
    if (pos == SPARSE_SET_INVALID_POS) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    lua_getiuservalue(L, 1, 1);
    lua_pushvalue(L, 3);
    lua_rawseti(L, -2, pos + 1);
    lua_pop(L, 1);

    lua_pushboolean(L, true);
    return 1;
}

static int l_set_remove(lua_State *L) {
    sparse_set_t *set = get_set(L);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    
    if (!sparse_set_contains(set, id)) {
        lua_pushboolean(L, false);
        return 1;
    }

    uint32_t index = ID_INDEX(id);
    uint32_t page_idx = index >> SPARSE_SET_PAGE_SHIFT;
    uint32_t offset = index & SPARSE_SET_PAGE_MASK;
    uint32_t pos = set->sparse[page_idx][offset];
    uint32_t last_pos = set->size - 1;

    if (pos != last_pos) {
        lua_getiuservalue(L, 1, 1);
        lua_rawgeti(L, -1, last_pos + 1);
        lua_rawseti(L, -2, pos + 1);
        lua_pop(L, 1);
    }
    
    lua_getiuservalue(L, 1, 1);
    lua_pushnil(L);
    lua_rawseti(L, -2, last_pos + 1);
    lua_pop(L, 1);

    sparse_set_remove(set, id);
    lua_pushboolean(L, true);
    return 1;
}

static int l_set_get(lua_State *L) {
    sparse_set_t *set = get_set(L);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    
    uint32_t index = ID_INDEX(id);
    if (sparse_set_contains(set, id)) {
        uint32_t page_idx = index >> SPARSE_SET_PAGE_SHIFT;
        uint32_t offset = index & SPARSE_SET_PAGE_MASK;
        uint32_t pos = set->sparse[page_idx][offset];
        lua_getiuservalue(L, 1, 1);
        lua_rawgeti(L, -1, pos + 1);
        return 1;
    }
    return 0;
}

static int l_set_contains(lua_State *L) {
    sparse_set_t *set = get_set(L);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    lua_pushboolean(L, sparse_set_contains(set, id));
    return 1;
}

static int l_set_size(lua_State *L) {
    sparse_set_t *set = get_set(L);
    lua_pushinteger(L, sparse_set_size(set));
    return 1;
}

static int l_set_gc(lua_State *L) {
    sparse_set_t *set = get_set(L);
    sparse_set_deinit(set);
    return 0;
}

static int _iter_optimized(lua_State *L) {
    sparse_set_t *set = (sparse_set_t *)lua_touserdata(L, 1);
    int pos = lua_tointeger(L, 2);
    if (pos >= (int)sparse_set_size(set)) return 0;
    
    sparse_set_id_t id = sparse_set_get_id(set, pos);
    lua_pushinteger(L, pos + 1);
    lua_pushinteger(L, id);
    lua_rawgeti(L, lua_upvalueindex(1), pos + 1);
    return 3;
}

static int l_set_iter(lua_State *L) {
    sparse_set_t *set = get_set(L);
    lua_getiuservalue(L, 1, 1);
    lua_pushcclosure(L, _iter_optimized, 1);
    lua_pushlightuserdata(L, set);
    lua_pushinteger(L, 0);
    return 3;
}

static const struct luaL_Reg reg_methods[] = {
    {"create", l_reg_create_id},
    {"destroy", l_reg_destroy_id},
    {"valid", l_reg_valid},
    {NULL, NULL}
};

static const struct luaL_Reg set_methods[] = {
    {"insert", l_set_insert},
    {"remove", l_set_remove},
    {"contains", l_set_contains},
    {"get", l_set_get},
    {"size", l_set_size},
    {"iter", l_set_iter},
    {NULL, NULL}
};

int luaopen_sparseset(lua_State *L) {
    luaL_newmetatable(L, REGISTRY_METATABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_reg_gc);
    lua_setfield(L, -2, "__gc");
    luaL_setfuncs(L, reg_methods, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, SET_METATABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_set_gc);
    lua_setfield(L, -2, "__gc");
    luaL_setfuncs(L, set_methods, 0);
    lua_pop(L, 1);

    lua_newtable(L);
    lua_pushcfunction(L, l_reg_create);
    lua_setfield(L, -2, "new_registry");
    lua_pushcfunction(L, l_set_create);
    lua_setfield(L, -2, "new_set");
    
    return 1;
}
