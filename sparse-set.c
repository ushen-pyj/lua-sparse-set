
#include "sparse-set.h"
#include <string.h>
#include <stdlib.h>

bool sparse_set_init(sparse_set_t *set) {
    set->sparse = (uint32_t**)calloc(SPARSE_SET_DEFAULT_CAPACITY, sizeof(uint32_t*));
    set->dense = (sparse_set_id_t*)malloc(SPARSE_SET_DEFAULT_CAPACITY * sizeof(sparse_set_id_t));
    
    if (!set->sparse || !set->dense) {
        if (set->sparse) free(set->sparse);
        if (set->dense) free(set->dense);
        return false;
    }

    set->size = 0;
    set->dense_capacity = SPARSE_SET_DEFAULT_CAPACITY;
    set->sparse_capacity = SPARSE_SET_DEFAULT_CAPACITY;
    set->stride = 0;
    set->data = NULL;
    return true;
}

void sparse_set_deinit(sparse_set_t *set) {
    if (set) {
        if (set->sparse) {
            for (uint32_t i = 0; i < set->sparse_capacity; i++) {
                if (set->sparse[i]) free(set->sparse[i]);
            }
            free(set->sparse);
        }
        if (set->dense) free(set->dense);
        if (set->data) free(set->data);
        set->sparse = NULL;
        set->dense = NULL;
        set->data = NULL;
        set->stride = 0;
    }
}

static bool sparse_set_grow_dense(sparse_set_t *set) {
    uint32_t new_capacity = set->dense_capacity * 2;
    sparse_set_id_t *new_dense = (sparse_set_id_t*)realloc(set->dense, new_capacity * sizeof(sparse_set_id_t));
    if (!new_dense) return false;
    set->dense = new_dense;
    
    if (set->stride > 0) {
        uint8_t *new_data = (uint8_t*)realloc(set->data, new_capacity * set->stride);
        if (!new_data) {
            return false;
        }
        set->data = new_data;
    }
    
    set->dense_capacity = new_capacity;
    return true;
}

static bool sparse_set_ensure_page(sparse_set_t *set, uint32_t page_idx) {
    if (page_idx >= set->sparse_capacity) {
        uint32_t new_capacity = set->sparse_capacity;
        while (new_capacity <= page_idx) new_capacity *= 2;
        if (new_capacity > (SPARSE_SET_MAX_SIZE >> SPARSE_SET_PAGE_SHIFT) + 1) {
            new_capacity = (SPARSE_SET_MAX_SIZE >> SPARSE_SET_PAGE_SHIFT) + 1;
        }
        if (page_idx >= new_capacity) return false;

        uint32_t **new_sparse = (uint32_t**)realloc(set->sparse, new_capacity * sizeof(uint32_t*));
        if (!new_sparse) return false;
        
        memset(new_sparse + set->sparse_capacity, 0, (new_capacity - set->sparse_capacity) * sizeof(uint32_t*));
        set->sparse = new_sparse;
        set->sparse_capacity = new_capacity;
    }

    if (!set->sparse[page_idx]) {
        set->sparse[page_idx] = (uint32_t*)malloc(SPARSE_SET_PAGE_SIZE * sizeof(uint32_t));
        if (!set->sparse[page_idx]) return false;
        memset(set->sparse[page_idx], 0xFF, SPARSE_SET_PAGE_SIZE * sizeof(uint32_t));
    }
    return true;
}

sparse_set_t* sparse_set_create() {
    sparse_set_t *set = (sparse_set_t*)malloc(sizeof(sparse_set_t));
    if (!set) return NULL;
    if (!sparse_set_init(set)) {
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
    uint32_t page_idx = index >> SPARSE_SET_PAGE_SHIFT;
    if (page_idx >= set->sparse_capacity || !set->sparse[page_idx]) return false;
    
    uint32_t pos = set->sparse[page_idx][index & SPARSE_SET_PAGE_MASK];
    return pos < set->size && set->dense[pos] == id;
}

uint32_t sparse_set_insert(sparse_set_t *set, sparse_set_id_t id) {
    uint32_t index = ID_INDEX(id);
    uint32_t page_idx = index >> SPARSE_SET_PAGE_SHIFT;
    
    if (!sparse_set_ensure_page(set, page_idx)) {
        return SPARSE_SET_INVALID_POS;
    }
    
    uint32_t *page = set->sparse[page_idx];
    uint32_t offset = index & SPARSE_SET_PAGE_MASK;
    uint32_t pos = page[offset];
    
    if (pos < set->size && ID_INDEX(set->dense[pos]) == index) {
        if (set->dense[pos] == id) return SPARSE_SET_INVALID_POS;
        set->dense[pos] = id;
        return pos;
    }
    
    if (set->size >= set->dense_capacity) {
        if (!sparse_set_grow_dense(set)) return SPARSE_SET_INVALID_POS;
    }

    uint32_t new_pos = set->size;
    set->dense[new_pos] = id;
    
    if (set->stride > 0) {
        memset(set->data + new_pos * set->stride, 0, set->stride);
    }
    
    page[offset] = new_pos;
    set->size++;
    return new_pos;
}

bool sparse_set_remove(sparse_set_t *set, sparse_set_id_t id) {
    if (!sparse_set_contains(set, id)) return false;
    
    uint32_t index = ID_INDEX(id);
    uint32_t page_idx = index >> SPARSE_SET_PAGE_SHIFT;
    uint32_t offset = index & SPARSE_SET_PAGE_MASK;
    
    uint32_t pos = set->sparse[page_idx][offset];
    uint32_t last_pos = set->size - 1;
    sparse_set_id_t last_id = set->dense[last_pos];
    uint32_t last_index = ID_INDEX(last_id);

    set->dense[pos] = last_id;

    uint32_t last_page_idx = last_index >> SPARSE_SET_PAGE_SHIFT;
    uint32_t last_offset = last_index & SPARSE_SET_PAGE_MASK;
    set->sparse[last_page_idx][last_offset] = pos;
    
    if (set->stride > 0) {
        memcpy(set->data + pos * set->stride, set->data + last_pos * set->stride, set->stride);
    }
    
    set->sparse[page_idx][offset] = SPARSE_SET_INVALID_POS;
    
    set->size--;
    return true;
}

void sparse_set_clear(sparse_set_t *set) {
    for (uint32_t i = 0; i < set->sparse_capacity; i++) {
        if (set->sparse[i]) {
            memset(set->sparse[i], 0xFF, SPARSE_SET_PAGE_SIZE * sizeof(uint32_t));
        }
    }
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

uint32_t sparse_set_index_of(const sparse_set_t *set, sparse_set_id_t id) {
    uint32_t index = ID_INDEX(id);
    uint32_t page_idx = index >> SPARSE_SET_PAGE_SHIFT;
    
    if (page_idx >= set->sparse_capacity || !set->sparse[page_idx]) {
        return SPARSE_SET_INVALID_POS;
    }
    
    uint32_t pos = set->sparse[page_idx][index & SPARSE_SET_PAGE_MASK];
    if (pos < set->size && set->dense[pos] == id) {
        return pos;
    }
    return SPARSE_SET_INVALID_POS;
}

void sparse_set_swap_at(sparse_set_t *set, uint32_t a, uint32_t b) {
    if (a == b || a >= set->size || b >= set->size) {
        return;
    }
    
    sparse_set_id_t id_a = set->dense[a];
    sparse_set_id_t id_b = set->dense[b];
    
    set->dense[a] = id_b;
    set->dense[b] = id_a;
    
    uint32_t idx_a = ID_INDEX(id_a);
    uint32_t page_a = idx_a >> SPARSE_SET_PAGE_SHIFT;
    uint32_t off_a = idx_a & SPARSE_SET_PAGE_MASK;
    
    uint32_t idx_b = ID_INDEX(id_b);
    uint32_t page_b = idx_b >> SPARSE_SET_PAGE_SHIFT;
    uint32_t off_b = idx_b & SPARSE_SET_PAGE_MASK;
    
    set->sparse[page_a][off_a] = b;
    set->sparse[page_b][off_b] = a; 
    
    if (set->stride > 0) {
        uint8_t *ptr_a = set->data + a * set->stride;
        uint8_t *ptr_b = set->data + b * set->stride;
        uint8_t buffer[64];
        uint32_t remaining = set->stride;
        uint32_t offset = 0;
        
        while (remaining > 0) {
            uint32_t chunk_size = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
            
            memcpy(buffer, ptr_a + offset, chunk_size);
            memcpy(ptr_a + offset, ptr_b + offset, chunk_size);
            memcpy(ptr_b + offset, buffer, chunk_size);
            
            remaining -= chunk_size;
            offset += chunk_size;
        }
    } 
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

bool sparse_set_set_stride(sparse_set_t *set, uint32_t stride) {
    if (set->size > 0 || set->data) return false; // Can only set stride when empty/init
    if (stride == 0) return true;
    
    set->stride = stride;
    set->data = (uint8_t*)calloc(set->dense_capacity, stride);
    return set->data != NULL;
}

void* sparse_set_get_data(const sparse_set_t *set, uint32_t pos) {
    if (pos >= set->size || !set->data) return NULL;
    return set->data + pos * set->stride;
}
