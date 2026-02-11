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

bool sparse_set_contains(const sparse_set_t *set, uint32_t index) {
    if (index > set->max_val) return false;
    uint32_t pos = set->sparse[index];
    return pos < set->size && set->dense[pos] == index;
}

bool sparse_set_add(sparse_set_t *set, uint32_t index) {
    if (index > set->max_val || set->size >= set->capacity || sparse_set_contains(set, index)) {
        return false;
    }

    set->dense[set->size] = index;
    set->sparse[index] = set->size;
    set->size++;
    return true;
}

bool sparse_set_remove(sparse_set_t *set, uint32_t index) {
    if (!sparse_set_contains(set, index)) {
        return false;
    }

    uint32_t pos = set->sparse[index];
    uint32_t last_index = set->dense[set->size - 1];

    set->dense[pos] = last_index;
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

uint32_t sparse_set_get_index(const sparse_set_t *set, uint32_t pos) {
    if (pos < set->size) {
        return set->dense[pos];
    }
    return 0; // Or some error value
}
