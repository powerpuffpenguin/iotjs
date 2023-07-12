#ifndef IOTJS_MEMPOOL_H
#define IOTJS_MEMPOOL_H
#include <stddef.h>
#include <iotjs/mempool/list.h>

typedef struct
{
    // 空閒的可分配節點
    iotjs_mep_list_t idle;
    // 已經被分配出去的節點
    iotjs_mep_list_t used;
    // 塊大小
    size_t block;
    // 最多緩存多少個塊
    size_t cache;
} iotjs_mep_alloctor_t;

typedef struct
{
    iotjs_mep_alloctor_t *alloctors;
    size_t len;
} iotjs_mep_t;

// 初始化內存池
void iotjs_mep_init(iotjs_mep_t *mep, iotjs_mep_alloctor_t *alloctors, size_t len);
// 銷毀內存池
void iotjs_mep_destroy(iotjs_mep_t *mep);

void *iotjs_mep_malloc(iotjs_mep_t *mep, size_t sz);
void *iotjs_mep_realloc(iotjs_mep_t *mep, void *ptr, size_t sz);
void iotjs_mep_free(iotjs_mep_t *mep, void *ptr);
#endif