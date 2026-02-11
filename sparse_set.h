#ifndef SPARSE_SET_H
#define SPARSE_SET_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    uint32_t *sparse;
    uint32_t *dense;
    uint32_t size;
    uint32_t capacity;
    uint32_t max_val;
} sparse_set_t;

sparse_set_t* sparse_set_create(uint32_t max_val, uint32_t capacity);
void sparse_set_destroy(sparse_set_t *set);

bool sparse_set_contains(const sparse_set_t *set, uint32_t val);
bool sparse_set_add(sparse_set_t *set, uint32_t val);
bool sparse_set_remove(sparse_set_t *set, uint32_t val);
void sparse_set_clear(sparse_set_t *set);

uint32_t sparse_set_size(const sparse_set_t *set);
uint32_t sparse_set_get(const sparse_set_t *set, uint32_t index);

#endif // SPARSE_SET_H
