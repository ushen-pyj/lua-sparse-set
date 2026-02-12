#include "sparse-set.h"
#include <stdlib.h>

bool registry_init(registry_t *reg, uint32_t max_size) {
    reg->generations = (uint32_t*)calloc(max_size, sizeof(uint32_t));
    reg->recycle = (uint32_t*)malloc(max_size * sizeof(uint32_t));
    
    if (!reg->generations || !reg->recycle) {
        if (reg->generations) free(reg->generations);
        if (reg->recycle) free(reg->recycle);
        return false;
    }

    reg->capacity = max_size;
    reg->recycle_count = 0;
    reg->next_index = 0;
    return true;
}

void registry_deinit(registry_t *reg) {
    if (reg) {
        if (reg->generations) free(reg->generations);
        if (reg->recycle) free(reg->recycle);
        reg->generations = NULL;
        reg->recycle = NULL;
    }
}

registry_t* registry_create(uint32_t max_size) {
    registry_t *reg = (registry_t*)malloc(sizeof(registry_t));
    if (!reg) return NULL;

    if (!registry_init(reg, max_size)) {
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
