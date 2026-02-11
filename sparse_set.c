#include "sparse_set.h"
#include <string.h>

sparse_set_t* sparse_set_create(uint32_t max_val, uint32_t capacity) {
    sparse_set_t *set = (sparse_set_t*)malloc(sizeof(sparse_set_t));
    if (!set) return NULL;

    set->sparse = (uint32_t*)malloc((max_val + 1) * sizeof(uint32_t));
    set->dense = (uint32_t*)malloc(capacity * sizeof(uint32_t));
    
    if (!set->sparse || !set->dense) {
        if (set->sparse) free(set->sparse);
        if (set->dense) free(set->dense);
        free(set);
        return NULL;
    }

    set->size = 0;
    set->capacity = capacity;
    set->max_val = max_val;

    return set;
}

void sparse_set_destroy(sparse_set_t *set) {
    if (set) {
        free(set->sparse);
        free(set->dense);
        free(set);
    }
}

bool sparse_set_contains(const sparse_set_t *set, uint32_t val) {
    if (val > set->max_val) return false;
    uint32_t idx = set->sparse[val];
    return idx < set->size && set->dense[idx] == val;
}

bool sparse_set_add(sparse_set_t *set, uint32_t val) {
    if (val > set->max_val || set->size >= set->capacity || sparse_set_contains(set, val)) {
        return false;
    }

    set->dense[set->size] = val;
    set->sparse[val] = set->size;
    set->size++;
    return true;
}

bool sparse_set_remove(sparse_set_t *set, uint32_t val) {
    if (!sparse_set_contains(set, val)) {
        return false;
    }

    uint32_t idx = set->sparse[val];
    uint32_t last_val = set->dense[set->size - 1];

    set->dense[idx] = last_val;
    set->sparse[last_val] = idx;
    set->size--;
    return true;
}

void sparse_set_clear(sparse_set_t *set) {
    set->size = 0;
}

uint32_t sparse_set_size(const sparse_set_t *set) {
    return set->size;
}

uint32_t sparse_set_get(const sparse_set_t *set, uint32_t index) {
    if (index < set->size) {
        return set->dense[index];
    }
    return 0; // Or some error value
}
