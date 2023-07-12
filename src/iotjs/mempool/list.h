#ifndef IOTJS_MEMPOOL_LIST_H
#define IOTJS_MEMPOOL_LIST_H
#include <stddef.h>
#include <iotjs/container/list.h>

typedef struct mempool
{
    // 塊大小
    size_t block;
    // 調用者期望分配的大小
    size_t len;
} iotjs_mep_value_t;

IOTJS_LIST_DEFINE(iotjs_mep_list_t, iotjs_mep_ele_t, iotjs_mep_value_t)

IOTJS_LIST_DEFINE_HEADER(iotjs_mep_list, iotjs_mep_list_t, iotjs_mep_ele_t)

#endif
