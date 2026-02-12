#include "sparse-set.h"
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

    // 初始化dense数组，填满所有可用索引
    for (uint32_t i = 0; i < max_size; i++) {
        set->dense[i] = i;
    }

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

    // 直接从dense数组末尾获取下一个可用索引 (O(1))
    // dense数组中 [size...capacity-1] 的部分存储了所有未使用的索引
    uint32_t index = set->dense[set->size];
    
    // 在sparse数组中记录位置
    set->sparse[index] = set->size;
    set->size++;
    
    return index;
}

bool sparse_set_remove(sparse_set_t *set, uint32_t index) {
    if (!sparse_set_contains(set, index)) {
        return false;
    }

    uint32_t pos = set->sparse[index];
    uint32_t last_index = set->dense[set->size - 1];

    set->dense[pos] = last_index;
    set->sparse[last_index] = pos;
    
    // 将删除的索引放回dense数组的有效区末尾 (recycling)
    set->dense[set->size - 1] = index;
    
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
    return UINT32_MAX;
}

// 创建迭代器
sparse_set_iter_t sparse_set_iter(const sparse_set_t *set) {
    sparse_set_iter_t iter = {
        .set = set,
        .current_pos = 0
    };
    return iter;
}

// 获取下一个元素
// 返回true表示成功获取，false表示已经遍历完成
// out_index输出当前元素的索引
bool sparse_set_iter_next(sparse_set_iter_t *iter, uint32_t *out_index) {
    if (!iter || !iter->set || iter->current_pos >= iter->set->size) {
        return false;
    }
    
    if (out_index) {
        *out_index = iter->set->dense[iter->current_pos];
    }
    
    iter->current_pos++;
    return true;
}
