#ifndef SPARSE_SET_H
#define SPARSE_SET_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>

typedef uint64_t sparse_set_id_t;

#define ID_NULL UINT64_MAX
#define ID_INDEX(id) ((uint32_t)(id & 0xFFFFFFFF))
#define ID_VERSION(id) ((uint32_t)((id >> 32) & 0xFFFFFFFF))
#define ID_MAKE(index, version) (((uint64_t)(version) << 32) | (index))

#define SPARSE_SET_PAGE_SIZE 4096
#define SPARSE_SET_PAGE_MASK (SPARSE_SET_PAGE_SIZE - 1)
#define SPARSE_SET_PAGE_SHIFT 12

typedef struct {
    uint32_t **generations;
    uint32_t generations_capacity;
    uint32_t *recycle;
    uint32_t recycle_count;
    uint32_t recycle_capacity;
    uint32_t next_index;
} registry_t;

typedef struct {
    uint32_t **sparse;
    uint32_t sparse_capacity;
    sparse_set_id_t *dense;
    uint32_t size;
    uint32_t dense_capacity;
    uint32_t stride;
    uint8_t *data;
} sparse_set_t;

static inline sparse_set_t* 
get_set(lua_State *L){
    return (sparse_set_t*)lua_touserdata(L, 1);
}

static inline registry_t*
get_reg(lua_State *L){
    return (registry_t*)lua_touserdata(L, 1);
}

#define SPARSE_SET_DEFAULT_CAPACITY 64
#define SPARSE_SET_MAX_SIZE 0xFFFFF

bool sparse_set_init(sparse_set_t *set);
void sparse_set_deinit(sparse_set_t *set);

bool registry_init(registry_t *reg);
void registry_deinit(registry_t *reg);

registry_t* registry_create();
void registry_destroy(registry_t *reg);
sparse_set_id_t registry_create_id(registry_t *reg);
void registry_recycle(registry_t *reg, sparse_set_id_t id);
bool registry_valid(const registry_t *reg, sparse_set_id_t id);

sparse_set_t* sparse_set_create();
void sparse_set_destroy(sparse_set_t *set);

#define SPARSE_SET_INVALID_POS UINT32_MAX

bool sparse_set_contains(const sparse_set_t *set, sparse_set_id_t id);
uint32_t sparse_set_insert(sparse_set_t *set, sparse_set_id_t id);
bool sparse_set_remove(sparse_set_t *set, sparse_set_id_t id);
void sparse_set_clear(sparse_set_t *set);

uint32_t sparse_set_size(const sparse_set_t *set);
sparse_set_id_t sparse_set_get_id(const sparse_set_t *set, uint32_t pos);
uint32_t sparse_set_index_of(const sparse_set_t *set, sparse_set_id_t id);
void sparse_set_swap_at(sparse_set_t *set, uint32_t a, uint32_t b);

bool sparse_set_set_stride(sparse_set_t *set, uint32_t stride);
void* sparse_set_get_data(const sparse_set_t *set, uint32_t pos);

typedef struct {
    const sparse_set_t *set;
    uint32_t current_pos;
} sparse_set_iter_t;

sparse_set_iter_t sparse_set_iter(const sparse_set_t *set);
bool sparse_set_iter_next(sparse_set_iter_t *iter, sparse_set_id_t *out_id);

#endif
