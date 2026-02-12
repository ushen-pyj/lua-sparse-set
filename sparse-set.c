#include "sparse-set.h"
#include <string.h>
#include <stdlib.h>

bool sparse_set_init(sparse_set_t *set, uint32_t max_size) {
    set->sparse = (uint32_t*)malloc(max_size * sizeof(uint32_t));
    set->dense = (sparse_set_id_t*)malloc(max_size * sizeof(sparse_set_id_t));
    
    if (!set->sparse || !set->dense) {
        if (set->sparse) free(set->sparse);
        if (set->dense) free(set->dense);
        return false;
    }

    memset(set->sparse, 0xFF, max_size * sizeof(uint32_t));
    set->size = 0;
    set->capacity = max_size;
    return true;
}

void sparse_set_deinit(sparse_set_t *set) {
    if (set) {
        if (set->sparse) free(set->sparse);
        if (set->dense) free(set->dense);
        set->sparse = NULL;
        set->dense = NULL;
    }
}

sparse_set_t* sparse_set_create(uint32_t max_size) {
    sparse_set_t *set = (sparse_set_t*)malloc(sizeof(sparse_set_t));
    if (!set) return NULL;

    if (!sparse_set_init(set, max_size)) {
        free(set);
        return NULL;
    }

    return set;
}

void sparse_set_destroy(sparse_set_t *set) {
    if (set) {
        sparse_set_deinit(set);
        free(set);
    }
}

bool sparse_set_contains(const sparse_set_t *set, sparse_set_id_t id) {
    uint32_t index = ID_INDEX(id);
    if (index >= set->capacity) return false;
    
    uint32_t pos = set->sparse[index];
    return pos < set->size && set->dense[pos] == id;
}

uint32_t sparse_set_insert(sparse_set_t *set, sparse_set_id_t id) {
    uint32_t index = ID_INDEX(id);
    if (index >= set->capacity) return SPARSE_SET_INVALID_POS;
    
    uint32_t pos = set->sparse[index];
    if (pos < set->size && ID_INDEX(set->dense[pos]) == index) {
        if (set->dense[pos] == id) return SPARSE_SET_INVALID_POS;
        set->dense[pos] = id;
        return pos;
    }
    
    if (set->size >= set->capacity) return SPARSE_SET_INVALID_POS;

    uint32_t new_pos = set->size;
    set->dense[new_pos] = id;
    set->sparse[index] = new_pos;
    set->size++;
    return new_pos;
}

bool sparse_set_remove(sparse_set_t *set, sparse_set_id_t id) {
    if (!sparse_set_contains(set, id)) return false;
    
    uint32_t index = ID_INDEX(id);
    uint32_t pos = set->sparse[index];
    uint32_t last_pos = set->size - 1;
    sparse_set_id_t last_id = set->dense[last_pos];
    uint32_t last_index = ID_INDEX(last_id);

    set->dense[pos] = last_id;
    set->sparse[last_index] = pos;
    
    set->size--;
    return true;
}

void sparse_set_clear(sparse_set_t *set) {
    set->size = 0;
}

uint32_t sparse_set_size(const sparse_set_t *set) {
    return set->size;
}

sparse_set_id_t sparse_set_get_id(const sparse_set_t *set, uint32_t pos) {
    if (pos < set->size) {
        return set->dense[pos];
    }
    return ID_NULL;
}

sparse_set_iter_t sparse_set_iter(const sparse_set_t *set) {
    sparse_set_iter_t iter = { .set = set, .current_pos = 0 };
    return iter;
}

bool sparse_set_iter_next(sparse_set_iter_t *iter, sparse_set_id_t *out_id) {
    if (!iter || !iter->set || iter->current_pos >= iter->set->size) {
        return false;
    }
    if (out_id) {
        *out_id = sparse_set_get_id(iter->set, iter->current_pos);
    }
    iter->current_pos++;
    return true;
}
