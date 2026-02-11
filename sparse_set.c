#include "sparse_set.h"
#include <string.h>

sparse_set_t* sparse_set_create(uint32_t max_size) {
    sparse_set_t *set = (sparse_set_t*)malloc(sizeof(sparse_set_t));
    if (!set) return NULL;

    set->sparse = (uint32_t*)malloc(max_size * sizeof(uint32_t));
    set->dense = (uint32_t*)malloc(max_size * sizeof(uint32_t));
    
    if (!set->sparse || !set->dense) {
        if (set->sparse) free(set->sparse);
        if (set->dense) free(set->dense);
        free(set);
        return NULL;
    }

    set->size = 0;
    set->capacity = max_size;
    set->max_val = max_size - 1;

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

uint32_t sparse_set_add(sparse_set_t *set) {
    // 如果已满，返回错误值
    if (set->size >= set->capacity) {
        return UINT32_MAX;
    }

    uint32_t index;
    
    // 首先尝试从dense数组后面查找可复用的空闲index
    // dense[size]位置保存着上一次被移除的index
    if (set->size < set->capacity) {
        // 检查dense数组在size位置及之后是否有之前使用过的index可以复用
        index = set->dense[set->size];
        
        // 验证这个index是否可用（未被使用且在有效范围内）
        if (index <= set->max_val && !sparse_set_contains(set, index)) {
            // 可以复用这个index
            set->sparse[index] = set->size;
            set->size++;
            return index;
        }
    }
    
    // 如果无法复用，从0开始查找第一个空闲的index
    for (index = 0; index <= set->max_val; index++) {
        if (!sparse_set_contains(set, index)) {
            set->dense[set->size] = index;
            set->sparse[index] = set->size;
            set->size++;
            return index;
        }
    }
    
    // 理论上不应该到达这里（因为前面已经检查了容量）
    return UINT32_MAX;
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
