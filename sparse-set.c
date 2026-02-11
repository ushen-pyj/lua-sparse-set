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
    
    // 从0开始查找第一个空闲的index
    for (index = 0; index <= set->max_val; index++) {
        if (!sparse_set_contains(set, index)) {
            // 将索引添加到dense数组
            set->dense[set->size] = index;
            // 在sparse数组中记录位置
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
