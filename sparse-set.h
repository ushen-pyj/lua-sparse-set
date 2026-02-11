#ifndef SPARSE_SET_H
#define SPARSE_SET_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    uint32_t *sparse;   // sparse数组：通过索引(key)查找在dense中的位置
    uint32_t *dense;    // dense数组：存储实际的索引(key)
    uint32_t size;
    uint32_t capacity;
    uint32_t max_val;
} sparse_set_t;

// 迭代器结构
typedef struct {
    const sparse_set_t *set;  // 指向要迭代的set
    uint32_t current_pos;     // 当前迭代位置
} sparse_set_iter_t;

sparse_set_t* sparse_set_create(uint32_t max_size);
void sparse_set_destroy(sparse_set_t *set);

bool sparse_set_contains(const sparse_set_t *set, uint32_t index);
uint32_t sparse_set_add(sparse_set_t *set);  // 自动分配index并返回
bool sparse_set_remove(sparse_set_t *set, uint32_t index);
void sparse_set_clear(sparse_set_t *set);

uint32_t sparse_set_size(const sparse_set_t *set);
uint32_t sparse_set_get_index(const sparse_set_t *set, uint32_t pos);

// 迭代器接口
sparse_set_iter_t sparse_set_iter(const sparse_set_t *set);  // 创建迭代器
bool sparse_set_iter_next(sparse_set_iter_t *iter, uint32_t *out_index);  // 获取下一个元素

#endif // SPARSE_SET_H
