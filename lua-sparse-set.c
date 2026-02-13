#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <string.h>
#include "sparse-set.h"

#define REGISTRY_METATABLE "SparseRegistry"
#define SET_METATABLE "SparseSet"

#define TYPE_INT 1
#define TYPE_FLOAT 2
#define TYPE_DOUBLE 3
#define TYPE_BYTE 4
#define TYPE_BOOL 5


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
    int stride = 0;
    if (lua_gettop(L) >= 1) {
        stride = luaL_checkinteger(L, 1);
    }

    sparse_set_t *set = (sparse_set_t *)lua_newuserdatauv(L, sizeof(sparse_set_t), 1);
    if (!sparse_set_init(set)) return luaL_error(L, "Failed to create set");

    if (stride > 0) {
        if (!sparse_set_set_stride(set, stride)) {
            return luaL_error(L, "Failed to set stride");
        }
    }
    
    lua_newtable(L);
    lua_setiuservalue(L, -2, 1);

    luaL_getmetatable(L, SET_METATABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_set_insert(lua_State *L) {
    sparse_set_t *set = get_set(L);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    
    uint32_t pos = sparse_set_index_of(set, id);
    bool is_new = false;
    
    if (pos == SPARSE_SET_INVALID_POS) {
        pos = sparse_set_insert(set, id);
        if (pos == SPARSE_SET_INVALID_POS) {
            lua_pushboolean(L, false);
            return 1;
        }
        is_new = true;
    }
    
    if (set->stride > 0) {
        if (!lua_isnil(L, 3)) {
            size_t len;
            const char *data = luaL_checklstring(L, 3, &len);
            if (len != set->stride) {
                return luaL_error(L, "Data size mismatch, expected %d got %d", set->stride, (int)len);
            }
            void *ptr = sparse_set_get_data(set, pos);
            if (ptr) {
                memcpy(ptr, data, len);
            }
        }
    } else {
        lua_getiuservalue(L, 1, 1);
        lua_pushvalue(L, 3);
        lua_rawseti(L, -2, pos + 1);
        lua_pop(L, 1);
    }

    lua_pushboolean(L, is_new);
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
        if (set->stride > 0) {
            void *ptr = sparse_set_get_data(set, pos);
            if (ptr) {
                lua_pushlstring(L, (const char *)ptr, set->stride);
            } else {
                lua_pushnil(L);
            }
        } else {
            lua_getiuservalue(L, 1, 1);
            lua_rawgeti(L, -1, pos + 1);
        }
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
    
    if (set->stride > 0) {
        void *ptr = sparse_set_get_data(set, pos);
        if (ptr) {
            lua_pushlstring(L, (const char *)ptr, set->stride);
        } else {
            lua_pushnil(L);
        }
    } else {
        lua_rawgeti(L, lua_upvalueindex(1), pos + 1);
    }
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

static int l_set_index_of(lua_State *L) {
    sparse_set_t *set = get_set(L);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    uint32_t pos = sparse_set_index_of(set, id);
    if (pos == SPARSE_SET_INVALID_POS) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, pos + 1);
    }
    return 1;
}

static int l_set_swap(lua_State *L) {
    sparse_set_t *set = get_set(L);
    lua_Integer a_lua = luaL_checkinteger(L, 2);
    lua_Integer b_lua = luaL_checkinteger(L, 3);
    
    if (a_lua < 1 || b_lua < 1) return luaL_error(L, "Index out of bounds");
    uint32_t a = (uint32_t)(a_lua - 1);
    uint32_t b = (uint32_t)(b_lua - 1);
    
    if (a >= set->size || b >= set->size) {
        return luaL_error(L, "Index out of bounds");
    }
    
    if (a != b) {
        sparse_set_swap_at(set, a, b);
        
        lua_getiuservalue(L, 1, 1);
        
        lua_rawgeti(L, -1, a_lua);
        lua_rawgeti(L, -2, b_lua);
        
        lua_rawseti(L, -3, a_lua);
        lua_rawseti(L, -2, b_lua);
        
        lua_pop(L, 1);
    }
    return 0;
}

static int l_set_at(lua_State *L) {
    sparse_set_t *set = get_set(L);
    lua_Integer index = luaL_checkinteger(L, 2);
    if (index < 1 || index > set->size) return 0;
    
    sparse_set_id_t id = set->dense[index - 1];
    
    lua_pushinteger(L, id);
    
    if (set->stride > 0) {
        void *ptr = sparse_set_get_data(set, index - 1);
        if (ptr) {
            lua_pushlstring(L, (const char *)ptr, set->stride);
        } else {
            lua_pushnil(L);
        }
    } else {
        lua_getiuservalue(L, 1, 1);
        lua_rawgeti(L, -1, index);
        lua_remove(L, -2);
    }
    return 2;
}

static int l_set_get_field(lua_State *L) {
    sparse_set_t *set = get_set(L);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    int offset = luaL_checkinteger(L, 3);
    int type = luaL_checkinteger(L, 4);
    if (type == TYPE_BOOL) type = TYPE_BYTE;
    
    uint32_t index = sparse_set_index_of(set, id);
    if (index == SPARSE_SET_INVALID_POS) return 0;
    
    void *base = sparse_set_get_data(set, index);
    if (!base) return 0;
    
    if (offset < 0 || (uint32_t)offset >= set->stride) {
        return luaL_error(L, "Offset out of bounds");
    }
    
    uint8_t *ptr = (uint8_t*)base + offset;
    
    switch (type) {
        case TYPE_INT: {
            int val;
            memcpy(&val, ptr, sizeof(int));
            lua_pushinteger(L, val);
            break;
        }
        case TYPE_FLOAT: {
            float val;
            memcpy(&val, ptr, sizeof(float));
            lua_pushnumber(L, val);
            break;
        }
        case TYPE_DOUBLE: {
            double val;
            memcpy(&val, ptr, sizeof(double));
            lua_pushnumber(L, val);
            break;
        }
        case TYPE_BYTE: {
            uint8_t val = *ptr;
            lua_pushinteger(L, val);
            break;
        }
        default:
            return luaL_error(L, "Unknown type %d", type);
    }
    return 1;
}

static int l_set_set_field(lua_State *L) {
    sparse_set_t *set = get_set(L);
    sparse_set_id_t id = (sparse_set_id_t)luaL_checkinteger(L, 2);
    int offset = luaL_checkinteger(L, 3);
    int type = luaL_checkinteger(L, 4);
    if (type == TYPE_BOOL) type = TYPE_BYTE;

    uint32_t index = sparse_set_index_of(set, id);
    if (index == SPARSE_SET_INVALID_POS) return 0;
    
    void *base = sparse_set_get_data(set, index);
    if (!base) return 0;
    
    if (offset < 0 || (uint32_t)offset >= set->stride) {
        return luaL_error(L, "Offset out of bounds");
    }
    
    uint8_t *ptr = (uint8_t*)base + offset;
    
    switch (type) {
        case TYPE_INT: {
            int val = (int)luaL_checkinteger(L, 5);
            memcpy(ptr, &val, sizeof(int));
            break;
        }
        case TYPE_FLOAT: {
            float val = (float)luaL_checknumber(L, 5);
            memcpy(ptr, &val, sizeof(float));
            break;
        }
        case TYPE_DOUBLE: {
            double val = (double)luaL_checknumber(L, 5);
            memcpy(ptr, &val, sizeof(double));
            break;
        }
        case TYPE_BYTE: {
            int val = luaL_checkinteger(L, 5);
            *ptr = (uint8_t)val;
            break;
        }
        default:
            return luaL_error(L, "Unknown type %d", type);
    }
    return 0;
}

static const struct luaL_Reg reg_methods[] = {
    {"create", l_reg_create_id},
    {"destroy", l_reg_destroy_id},
    {"valid", l_reg_valid},
    {NULL, NULL}
};

static const struct luaL_Reg set_methods[] = {
    {"at", l_set_at},
    {"index_of", l_set_index_of},
    {"swap", l_set_swap},
    {"insert", l_set_insert},
    {"remove", l_set_remove},
    {"contains", l_set_contains},
    {"get", l_set_get},
    {"size", l_set_size},
    {"iter", l_set_iter},
    {"get_field", l_set_get_field},
    {"set_field", l_set_set_field},
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
