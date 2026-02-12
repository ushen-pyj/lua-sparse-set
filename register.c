#include "sparse-set.h"
#include <stdlib.h>
#include <string.h>

bool registry_init(registry_t *reg) {
    reg->generations = (uint32_t**)calloc(SPARSE_SET_DEFAULT_CAPACITY, sizeof(uint32_t*));
    reg->recycle = (uint32_t*)malloc(SPARSE_SET_DEFAULT_CAPACITY * sizeof(uint32_t));
    
    if (!reg->generations || !reg->recycle) {
        if (reg->generations) free(reg->generations);
        if (reg->recycle) free(reg->recycle);
        return false;
    }

    reg->generations_capacity = SPARSE_SET_DEFAULT_CAPACITY;
    reg->recycle_capacity = SPARSE_SET_DEFAULT_CAPACITY;
    reg->recycle_count = 0;
    reg->next_index = 0;
    return true;
}

void registry_deinit(registry_t *reg) {
    if (reg) {
        if (reg->generations) {
            for (uint32_t i = 0; i < reg->generations_capacity; i++) {
                if (reg->generations[i]) free(reg->generations[i]);
            }
            free(reg->generations);
        }
        if (reg->recycle) free(reg->recycle);
        reg->generations = NULL;
        reg->recycle = NULL;
    }
}

static bool registry_ensure_gen_page(registry_t *reg, uint32_t page_idx) {
    if (page_idx >= reg->generations_capacity) {
        uint32_t new_capacity = reg->generations_capacity;
        while (new_capacity <= page_idx) new_capacity *= 2;
        
        uint32_t **new_gens = (uint32_t**)realloc(reg->generations, new_capacity * sizeof(uint32_t*));
        if (!new_gens) return false;
        
        memset(new_gens + reg->generations_capacity, 0, (new_capacity - reg->generations_capacity) * sizeof(uint32_t*));
        reg->generations = new_gens;
        reg->generations_capacity = new_capacity;
    }

    if (!reg->generations[page_idx]) {
        reg->generations[page_idx] = (uint32_t*)calloc(SPARSE_SET_PAGE_SIZE, sizeof(uint32_t));
        if (!reg->generations[page_idx]) return false;
    }
    return true;
}

registry_t* registry_create() {
    registry_t *reg = (registry_t*)malloc(sizeof(registry_t));
    if (!reg) return NULL;
    if (!registry_init(reg)) {
        free(reg);
        return NULL;
    }
    return reg;
}

void registry_destroy(registry_t *reg) {
    if (reg) {
        registry_deinit(reg);
        free(reg);
    }
}

sparse_set_id_t registry_create_id(registry_t *reg) {
    uint32_t index;
    uint32_t version;

    if (reg->recycle_count > 0) {
        index = reg->recycle[--reg->recycle_count];
        uint32_t page_idx = index >> SPARSE_SET_PAGE_SHIFT;
        version = reg->generations[page_idx][index & SPARSE_SET_PAGE_MASK];
    } else {
        index = reg->next_index++;
        uint32_t page_idx = index >> SPARSE_SET_PAGE_SHIFT;
        if (!registry_ensure_gen_page(reg, page_idx)) {
            return ID_NULL;
        }
        version = 0;
    }
    
    return ID_MAKE(index, version);
}

void registry_recycle(registry_t *reg, sparse_set_id_t id) {
    if (!registry_valid(reg, id)) return;
    
    uint32_t index = ID_INDEX(id);
    uint32_t page_idx = index >> SPARSE_SET_PAGE_SHIFT;
    reg->generations[page_idx][index & SPARSE_SET_PAGE_MASK]++;
    
    if (reg->recycle_count >= reg->recycle_capacity) {
        uint32_t new_cap = reg->recycle_capacity * 2;
        uint32_t *new_rec = (uint32_t*)realloc(reg->recycle, new_cap * sizeof(uint32_t));
        if (!new_rec) return;
        reg->recycle = new_rec;
        reg->recycle_capacity = new_cap;
    }
    
    reg->recycle[reg->recycle_count++] = index;
}

bool registry_valid(const registry_t *reg, sparse_set_id_t id) {
    uint32_t index = ID_INDEX(id);
    uint32_t version = ID_VERSION(id);
    uint32_t page_idx = index >> SPARSE_SET_PAGE_SHIFT;
    
    if (page_idx >= reg->generations_capacity || !reg->generations[page_idx]) return false;
    return reg->generations[page_idx][index & SPARSE_SET_PAGE_MASK] == version;
}
