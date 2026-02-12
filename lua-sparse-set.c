#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "sparse-set.h"

#define REGISTRY_METATABLE "SparseRegistry"
#define SET_METATABLE "SparseSet"

typedef struct {
    registry_t *reg;
} lua_registry_t;

static int l_reg_create(lua_State *L) {
    uint32_t max_size = (uint32_t)luaL_checkinteger(L, 1);
    lua_registry_t *lr = (lua_registry_t *)lua_newuserdatauv(L, sizeof(lua_registry_t), 0);
    lr->reg = registry_create(max_size);
    if (!lr->reg) return luaL_error(L, "Failed to create registry");
    luaL_getmetatable(L, REGISTRY_METATABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_reg_create_id(lua_State *L) {
    lua_registry_t *lr = (lua_registry_t *)luaL_checkudata(L, 1, REGISTRY_METATABLE);
    sparse_set_id_t id = registry_create_id(lr->reg);
    if (id == ID_NULL) lua_pushnil(L);
    else lua_pushinteger(L, id);
    return 1;
}

static int l_reg_destroy_id(lua_State *L) {
    lua_registry_t *lr = (lua_registry_t *)luaL_checkudata(L, 1, REGISTRY_METATABLE);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    registry_recycle(lr->reg, id);
    return 0;
}

static int l_reg_valid(lua_State *L) {
    lua_registry_t *lr = (lua_registry_t *)luaL_checkudata(L, 1, REGISTRY_METATABLE);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    lua_pushboolean(L, registry_valid(lr->reg, id));
    return 1;
}

static int l_reg_gc(lua_State *L) {
    lua_registry_t *lr = (lua_registry_t *)luaL_checkudata(L, 1, REGISTRY_METATABLE);
    if (lr->reg) registry_destroy(lr->reg);
    return 0;
}

typedef struct {
    sparse_set_t *set;
} lua_sparse_set_t;

static int l_set_create(lua_State *L) {
    uint32_t max_size = (uint32_t)luaL_checkinteger(L, 1);
    lua_sparse_set_t *lset = (lua_sparse_set_t *)lua_newuserdatauv(L, sizeof(lua_sparse_set_t), 1);
    lset->set = sparse_set_create(max_size);
    if (!lset->set) return luaL_error(L, "Failed to create set");
    
    lua_newtable(L);
    lua_setiuservalue(L, -2, 1);

    luaL_getmetatable(L, SET_METATABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_set_insert(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, SET_METATABLE);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    
    sparse_set_t *set = lset->set;
    uint32_t index = ID_INDEX(id);
    if (index >= set->capacity) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    uint32_t pos = set->sparse[index];
    if (pos < set->size && ID_INDEX(set->dense[pos]) == index) {
        if (set->dense[pos] == id) {
            lua_pushboolean(L, false);
            return 1;
        }
        set->dense[pos] = id;
    } else {
        if (set->size >= set->capacity) {
            lua_pushboolean(L, false);
            return 1;
        }
        pos = set->size++;
        set->dense[pos] = id;
        set->sparse[index] = pos;
    }
    
    lua_getiuservalue(L, 1, 1);
    lua_pushvalue(L, 3);
    lua_rawseti(L, -2, pos + 1);
    lua_pop(L, 1);

    lua_pushboolean(L, true);
    return 1;
}

static int l_set_remove(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, SET_METATABLE);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    
    sparse_set_t *set = lset->set;
    uint32_t index = ID_INDEX(id);
    if (index >= set->capacity) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    uint32_t pos = set->sparse[index];
    if (pos >= set->size || set->dense[pos] != id) {
        lua_pushboolean(L, false);
        return 1;
    }

    uint32_t last_pos = --set->size;
    lua_getiuservalue(L, 1, 1);
    if (pos != last_pos) {
        sparse_set_id_t last_id = set->dense[last_pos];
        set->dense[pos] = last_id;
        set->sparse[ID_INDEX(last_id)] = pos;
        
        lua_rawgeti(L, -1, last_pos + 1);
        lua_rawseti(L, -2, pos + 1);
    }
    
    lua_pushnil(L);
    lua_rawseti(L, -2, last_pos + 1);
    lua_pop(L, 1);

    set->sparse[index] = 0xFFFFFFFF;
    lua_pushboolean(L, true);
    return 1;
}

static int l_set_get(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, SET_METATABLE);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    
    sparse_set_t *set = lset->set;
    uint32_t index = ID_INDEX(id);
    if (index < set->capacity) {
        uint32_t pos = set->sparse[index];
        if (pos < set->size && set->dense[pos] == id) {
            lua_getiuservalue(L, 1, 1);
            lua_rawgeti(L, -1, pos + 1);
            return 1;
        }
    }
    return 0;
}

static int l_set_contains(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, SET_METATABLE);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    lua_pushboolean(L, sparse_set_contains(lset->set, id));
    return 1;
}

static int l_set_size(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, SET_METATABLE);
    lua_pushinteger(L, sparse_set_size(lset->set));
    return 1;
}

static int l_set_gc(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, SET_METATABLE);
    if (lset->set) sparse_set_destroy(lset->set);
    return 0;
}

static int _iter_optimized(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)lua_touserdata(L, 1);
    int pos = lua_tointeger(L, 2);
    if (pos >= (int)sparse_set_size(lset->set)) return 0;
    
    sparse_set_id_t id = sparse_set_get_id(lset->set, pos);
    lua_pushinteger(L, pos + 1);
    lua_pushinteger(L, id);
    lua_rawgeti(L, lua_upvalueindex(1), pos + 1);
    return 3;
}

static int l_set_iter(lua_State *L) {
    lua_sparse_set_t *lset = (lua_sparse_set_t *)luaL_checkudata(L, 1, SET_METATABLE);
    lua_getiuservalue(L, 1, 1);
    lua_pushcclosure(L, _iter_optimized, 1);
    lua_pushlightuserdata(L, lset);
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
