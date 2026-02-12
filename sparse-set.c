#include "sparse-set.h"
#include <string.h>

registry_t* registry_create(uint32_t max_size) {
    registry_t *reg = (registry_t*)malloc(sizeof(registry_t));
    if (!reg) return NULL;

    reg->generations = (uint32_t*)calloc(max_size, sizeof(uint32_t));
    reg->recycle = (uint32_t*)malloc(max_size * sizeof(uint32_t));
    
    if (!reg->generations || !reg->recycle) {
        if (reg->generations) free(reg->generations);
        if (reg->recycle) free(reg->recycle);
        free(reg);
        return NULL;
    }

    reg->capacity = max_size;
    reg->recycle_count = 0;
    reg->next_index = 0;
    return reg;
}

void registry_destroy(registry_t *reg) {
    if (reg) {
        free(reg->generations);
        free(reg->recycle);
        free(reg);
    }
}

sparse_set_id_t registry_create_id(registry_t *reg) {
    uint32_t index;
    uint32_t version;

    if (reg->recycle_count > 0) {
        index = reg->recycle[--reg->recycle_count];
        version = reg->generations[index];
    } else {
        if (reg->next_index >= reg->capacity) {
            return ID_NULL;
        }
        index = reg->next_index++;
        version = 0;
        reg->generations[index] = 0;
    }
    
    return ID_MAKE(index, version);
}

void registry_recycle(registry_t *reg, sparse_set_id_t id) {
    uint32_t index = ID_INDEX(id);
    if (!registry_valid(reg, id)) return;
    reg->generations[index]++;
    reg->recycle[reg->recycle_count++] = index;
}

bool registry_valid(const registry_t *reg, sparse_set_id_t id) {
    uint32_t index = ID_INDEX(id);
    uint32_t version = ID_VERSION(id);
    if (index >= reg->capacity) return false;
    return reg->generations[index] == version;
}

sparse_set_t* sparse_set_create(uint32_t max_size) {
    sparse_set_t *set = (sparse_set_t*)malloc(sizeof(sparse_set_t));
    if (!set) return NULL;

    set->sparse = (uint32_t*)malloc(max_size * sizeof(uint32_t));
    set->dense = (sparse_set_id_t*)malloc(max_size * sizeof(sparse_set_id_t));
    
    if (!set->sparse || !set->dense) {
        if (set->sparse) free(set->sparse);
        if (set->dense) free(set->dense);
        free(set);
        return NULL;
    }

    memset(set->sparse, 0xFF, max_size * sizeof(uint32_t));
    set->size = 0;
    set->capacity = max_size;
    return set;
}

void sparse_set_destroy(sparse_set_t *set) {
    if (set) {
        free(set->sparse);
        free(set->dense);
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
